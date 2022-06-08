#include "types.h"
#include "vm.h"
#include "vcpu.h"
#include "mm.h"
#include "kalloc.h"
#include "lib.h"
#include "memmap.h"
#include "printf.h"
#include "log.h"
#include "mmio.h"

struct vm vms[VM_MAX];

static struct vm *allocvm() {
  for(struct vm *vm = vms; vm < &vms[VM_MAX]; vm++) {
    if(vm->used == 0) {
      vm->used = 1;
      return vm;
    }
  }

  return NULL;
}

void pagetrap(struct vm *vm, u64 ipa, u64 size,
              int (*read_handler)(struct vcpu *, u64, u64 *, struct mmio_access *),
              int (*write_handler)(struct vcpu *, u64, u64, struct mmio_access *)) {
  u64 *vttbr = vm->vttbr;

  if(pagewalk(vttbr, ipa, 0))
    pageunmap(vttbr, ipa, size);

  if(mmio_reg_handler(vm, ipa, size, read_handler, write_handler) < 0)
    panic("?");

  tlb_flush();
}

void create_vm(struct vmconfig *vmcfg) {
  struct guest *guest = vmcfg->guest_img;
  struct guest *fdt = vmcfg->fdt_img;
  struct guest *initrd = vmcfg->initrd_img;

  vmm_log("create vm `%s`\n", guest->name);
  vmm_log("n vcpu: %d\n", vmcfg->nvcpu);
  vmm_log("allocated ram: %d byte\n", vmcfg->nallocate);
  vmm_log("img_start %p img_size %p byte\n", guest->start, guest->size);
  vmm_log("initrd_start %p initrd_size %p byte\n", initrd->start, initrd->size);

  if(guest->size > vmcfg->nallocate)
    panic("img_size > nallocate");
  if(vmcfg->nallocate % PAGESIZE != 0)
    panic("invalid mem size");
  if(vmcfg->nvcpu > VCPU_MAX)
    panic("too many vcpu");

  struct vm *vm = allocvm();

  strcpy(vm->name, guest->name);

  vm->fdt = 0x48400000;   /* fdt ipa */

  /* cpu0 */
  vm->vcpus[0] = new_vcpu(vm, 0, vmcfg->entrypoint);

  /* cpuN */
  for(int i = 1; i < vmcfg->nvcpu; i++)
    vm->vcpus[i] = new_vcpu(vm, i, 0);

  vm->nvcpu = vmcfg->nvcpu;

  u64 *vttbr = kalloc();
  if(!vttbr)
    panic("vttbr");

  u64 p, cpsize;
  for(p = 0; p < guest->size; p += PAGESIZE) {
    char *page = kalloc();
    if(!page)
      panic("img");

    if(guest->size - p > PAGESIZE)
      cpsize = PAGESIZE;
    else
      cpsize = guest->size - p;

    memcpy(page, (char *)guest->start+p, cpsize);
    pagemap(vttbr, vmcfg->entrypoint+p, (u64)page, PAGESIZE, S2PTE_NORMAL|S2PTE_RW);
  }

  for(; p < vmcfg->nallocate - 0x80000; p += PAGESIZE) {
    char *page = kalloc();
    if(!page)
      panic("ram");

    pagemap(vttbr, vmcfg->entrypoint+p, (u64)page, PAGESIZE, S2PTE_NORMAL|S2PTE_RW);
  }

  vmm_log("entrypoint+p %p\n", vmcfg->entrypoint+p);

  /* map fdt image */
  for(u64 d = 0; d < fdt->size; d += PAGESIZE) {
    char *page = kalloc();
    if(!page)
      panic("fdt");

    memcpy(page, (char *)fdt->start+d, PAGESIZE);
    pagemap(vttbr, vm->fdt+d, (u64)page, PAGESIZE, S2PTE_NORMAL|S2PTE_RW);
  }

  /* map initrd image */
  for(u64 f = 0; f < initrd->size; f += PAGESIZE) {
    char *page = kalloc();
    if(!page)
      panic("fdt");

    memcpy(page, (char *)initrd->start+f, PAGESIZE);
    pagemap(vttbr, 0x48000000+f, (u64)page, PAGESIZE, S2PTE_NORMAL|S2PTE_RW);
  }

  /* map peripheral */
  pagemap(vttbr, UARTBASE, UARTBASE, PAGESIZE, S2PTE_DEVICE|S2PTE_RW);
  pagemap(vttbr, GPIOBASE, GPIOBASE, PAGESIZE, S2PTE_DEVICE|S2PTE_RW);
  pagemap(vttbr, RTCBASE, RTCBASE, PAGESIZE, S2PTE_DEVICE|S2PTE_RW);
  pagemap(vttbr, PCIE_ECAM_BASE, PCIE_ECAM_BASE, 256*1024*1024,
          S2PTE_DEVICE|S2PTE_RW);
  pagemap(vttbr, PCIE_MMIO_BASE, PCIE_MMIO_BASE, 0x2eff0000, S2PTE_DEVICE|S2PTE_RW);
  pagemap(vttbr, PCIE_HIGH_MMIO_BASE, PCIE_HIGH_MMIO_BASE, 0x100000/*XXX*/,
          S2PTE_DEVICE|S2PTE_RW);

  vm->vttbr = vttbr;
  vm->pmap = NULL;
  vm->vgic = new_vgic(vm);

  virtio_mmio_init(vm);

  spinlock_init(&vm->lock);

  vcpu_ready(vm->vcpus[0]);
}
