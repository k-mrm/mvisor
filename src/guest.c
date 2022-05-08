#include "types.h"
#include "guest.h"

extern char _binary_guest_xv6_kernel_img_start[];
extern char _binary_guest_xv6_kernel_img_size[];

struct guest hello = {
  .start = (u64)_binary_guest_xv6_kernel_img_start,
  .size = (u64)_binary_guest_xv6_kernel_img_size,
};
