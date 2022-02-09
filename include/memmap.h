#ifndef MVMM_MEMMAP_H
#define MVMM_MEMMAP_H

#define UARTBASE    0x09000000
#define GICBASE     0x08000000

#define VMMBASE     0x40000000

#define PHYSIZE     (256*1024*1024)     /* 256 MB */
#define PHYEND      (VMMBASE+PHYSIZE)

#endif