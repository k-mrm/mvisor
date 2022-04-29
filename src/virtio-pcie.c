#include "pcie.h"
#include "virtio-pcie.h"
#include "log.h"

struct virtio_pci_common_cfg {
  /* About the whole device. */
  u32 device_feature_select; /* read-write */
  u32 device_feature; /* read-only for driver */
  u32 driver_feature_select; /* read-write */
  u32 driver_feature; /* read-write */
  u16 msix_config; /* read-write */
  u16 num_queues; /* read-only for driver */
  u8 device_status; /* read-write */
  u8 config_generation; /* read-only for driver */
  /* About a specific virtqueue. */
  u16 queue_select; /* read-write */
  u16 queue_size; /* read-write */
  u16 queue_msix_vector; /* read-write */
  u16 queue_enable; /* read-write */
  u16 queue_notify_off; /* read-only for driver */
  u64 queue_desc; /* read-write */
  u64 queue_driver; /* read-write */
  u64 queue_device; /* read-write */
};

static void virtio_net_init() {
  vmm_log("virtio_net_init\n");
}

static void virtio_rng_init() {
  vmm_log("virtio_rng_init\n");
}

void virtio_cap_scan(struct pci_config *cfg, struct virtio_pcie_cap *cap) {
  if(cap->cap_vndr != 0x9)
    vmm_warn("virtio-pci invalid vendor %p", cap->cap_vndr);

  vmm_log("cap vndr %p next %p\n", cap->cap_vndr, cap->cap_next);

  switch(cap->cfg_type) {
    case VIRTIO_PCI_CAP_COMMON_CFG: {
      vmm_log("\t%d %p\n", cap->bar, cap->offset);
      vmm_log("\tcap length: %p\n", cap->length);

      // vmm_log("common cfg %d %d %p\n", vtcfg->num_queues, vtcfg->queue_enable, vtcfg->device_status);

      break;
    }
  }

  if(cap->cap_next) {
    cap = (struct virtio_pcie_cap *)((char *)cfg + cap->cap_next);
    virtio_cap_scan(cfg, cap);
  }
}

int virtio_pci_dev_init(struct pci_config *cfg) {
  if(cfg->device_id < 0x1040)
    return -1;

  vmm_log("dev id %p revision id %p\n", cfg->device_id, cfg->revision_id);

  struct virtio_pcie_cap *cap = (struct virtio_pcie_cap *)((char *)cfg + cfg->cap_ptr);

  switch(cfg->device_id - 0x1040) {
    case 1:
      virtio_cap_scan(cfg, cap);
      virtio_net_init();
      break;
    case 4:
      virtio_rng_init();
      break;
    default:
      return -1;
  }

  return 0;
}