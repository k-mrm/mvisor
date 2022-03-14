#include "aarch64.h"
#include "gic.h"
#include "log.h"

/* gicv3 controller */

#define ich_hcr_el2   arm_sysreg(4, c12, c11, 0)
#define ich_vtr_el2   arm_sysreg(4, c12, c11, 1)
#define ich_vmcr_el2  arm_sysreg(4, c12, c11, 7)
#define ich_lr0_el2   arm_sysreg(4, c12, c12, 0)
#define ich_lr1_el2   arm_sysreg(4, c12, c12, 1)
#define ich_lr2_el2   arm_sysreg(4, c12, c12, 2)
#define ich_lr3_el2   arm_sysreg(4, c12, c12, 3)
#define ich_lr4_el2   arm_sysreg(4, c12, c12, 4)
#define ich_lr5_el2   arm_sysreg(4, c12, c12, 5)
#define ich_lr6_el2   arm_sysreg(4, c12, c12, 6)
#define ich_lr7_el2   arm_sysreg(4, c12, c12, 7)
#define ich_lr8_el2   arm_sysreg(4, c12, c13, 0)
#define ich_lr9_el2   arm_sysreg(4, c12, c13, 1)
#define ich_lr10_el2  arm_sysreg(4, c12, c13, 2)
#define ich_lr11_el2  arm_sysreg(4, c12, c13, 3)
#define ich_lr12_el2  arm_sysreg(4, c12, c13, 4)
#define ich_lr13_el2  arm_sysreg(4, c12, c13, 5)
#define ich_lr14_el2  arm_sysreg(4, c12, c13, 6)
#define ich_lr15_el2  arm_sysreg(4, c12, c13, 7)

#define icc_pmr_el1      arm_sysreg(0, c4, c6, 0)
#define icc_eoir0_el1    arm_sysreg(0, c12, c8, 1)
#define icc_dir_el1      arm_sysreg(0, c12, c11, 1)
#define icc_iar1_el1     arm_sysreg(0, c12, c12, 0)
#define icc_eoir1_el1    arm_sysreg(0, c12, c12, 1)
#define icc_ctlr_el1     arm_sysreg(0, c12, c12, 4)
#define icc_igrpen0_el1  arm_sysreg(0, c12, c12, 6)
#define icc_igrpen1_el1  arm_sysreg(0, c12, c12, 7)

#define ICC_CTLR_EOImode(m) ((m) << 1)

#define ICH_HCR_EN  (1<<0)

#define ICH_VMCR_VENG0  (1<<0)
#define ICH_VMCR_VENG1  (1<<1)

#define ICH_LR_VINTID(n)   ((n) & 0xffffffffL)
#define ICH_LR_PINTID(n)   (((n) & 0x3ffL) << 32)
#define ICH_LR_GROUP(n)    (((n) & 0x1L) << 60)
#define ICH_LR_HW          (1L << 61)
#define ICH_LR_STATE(n)    (((n) & 0x3L) << 62)
#define LR_INACTIVE  0L
#define LR_PENDING   1L
#define LR_ACTIVE    2L
#define LR_MASK      3L

static int gic_lr_max = 0;

u64 gic_read_lr(int n) {
  if(gic_lr_max <= n)
    panic("lr");

  u64 val;

  switch(n) {
    case 0:   read_sysreg(val, ich_lr0_el2); break;
    case 1:   read_sysreg(val, ich_lr1_el2); break;
    case 2:   read_sysreg(val, ich_lr2_el2); break;
    case 3:   read_sysreg(val, ich_lr3_el2); break;
    case 4:   read_sysreg(val, ich_lr4_el2); break;
    case 5:   read_sysreg(val, ich_lr5_el2); break;
    case 6:   read_sysreg(val, ich_lr6_el2); break;
    case 7:   read_sysreg(val, ich_lr7_el2); break;
    case 8:   read_sysreg(val, ich_lr8_el2); break;
    case 9:   read_sysreg(val, ich_lr9_el2); break;
    case 10:  read_sysreg(val, ich_lr10_el2); break;
    case 11:  read_sysreg(val, ich_lr11_el2); break;
    case 12:  read_sysreg(val, ich_lr12_el2); break;
    case 13:  read_sysreg(val, ich_lr13_el2); break;
    case 14:  read_sysreg(val, ich_lr14_el2); break;
    case 15:  read_sysreg(val, ich_lr15_el2); break;
    default:  panic("?");
  }

  return val;
}

void gic_write_lr(int n, u64 val) {
  if(gic_lr_max <= n)
    panic("lr");

  switch(n) {
    case 0:   write_sysreg(ich_lr0_el2, val); break;
    case 1:   write_sysreg(ich_lr1_el2, val); break;
    case 2:   write_sysreg(ich_lr2_el2, val); break;
    case 3:   write_sysreg(ich_lr3_el2, val); break;
    case 4:   write_sysreg(ich_lr4_el2, val); break;
    case 5:   write_sysreg(ich_lr5_el2, val); break;
    case 6:   write_sysreg(ich_lr6_el2, val); break;
    case 7:   write_sysreg(ich_lr7_el2, val); break;
    case 8:   write_sysreg(ich_lr8_el2, val); break;
    case 9:   write_sysreg(ich_lr9_el2, val); break;
    case 10:  write_sysreg(ich_lr10_el2, val); break;
    case 11:  write_sysreg(ich_lr11_el2, val); break;
    case 12:  write_sysreg(ich_lr12_el2, val); break;
    case 13:  write_sysreg(ich_lr13_el2, val); break;
    case 14:  write_sysreg(ich_lr14_el2, val); break;
    case 15:  write_sysreg(ich_lr15_el2, val); break;
    default:  panic("?");
  }
}

u64 gic_make_lr(u32 pirq, u32 virq, int grp) {
  return ICH_LR_STATE(LR_PENDING) | ICH_LR_HW | ICH_LR_GROUP(grp) | ICH_LR_PINTID(pirq) | ICH_LR_VINTID(virq);
}

u32 gic_read_iar() {
  u32 i;
  read_sysreg(i, icc_iar1_el1);
  return i;
}

void gic_eoi(u32 iar, int grp) {
  if(grp == 0)
    write_sysreg(icc_eoir0_el1, iar);
  else if(grp == 1)
    write_sysreg(icc_eoir1_el1, iar);
  else
    panic("?");
}

void gic_deactive_int(u32 irq) {
  write_sysreg(icc_dir_el1, irq);
}

static int gic_max_listregs() {
  u64 i;
  read_sysreg(i, ich_vtr_el2);
  return (i & 0x1f) + 1;
}

static void gicc_init(void) {
  write_sysreg(icc_igrpen0_el1, 0);
  write_sysreg(icc_igrpen1_el1, 0);

  write_sysreg(icc_pmr_el1, 0xff);
  write_sysreg(icc_ctlr_el1, ICC_CTLR_EOImode(1));

  write_sysreg(icc_igrpen0_el1, 1);
  write_sysreg(icc_igrpen1_el1, 1);

  isb();
}

static void gich_init(void) {
  write_sysreg(ich_vmcr_el2, ICH_VMCR_VENG0|ICH_VMCR_VENG1);
  write_sysreg(ich_hcr_el2, ICH_HCR_EN);

  gic_lr_max = gic_max_listregs();

  isb();
}

void gic_init(void) {
  vmm_log("gic init...\n");
  gicc_init();
  gich_init();
}