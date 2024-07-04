#ifndef __MEM_PART_H__
#define __MEM_PART_H__

// https://github.com/contiki-os/contiki/wiki/8051-Memory-Spaces

#ifdef BUILD_CODE
#define IDATA  __idata
#define XDATA  __xdata
#define AT0000 __at(0x0000) // 0x000   0
#define AT0008 __at(0x0008) // 0x008,  8
#define AT0010 __at(0x0010) // 0x010, 16
#define AT0040 __at(0x0040) // 0x040, 64
#define AT0050 __at(0x0050) // 0x050, 80
#define AT0080 __at(0x0080) // 0x080, 128
#define AT0090 __at(0x0090) // 0x090, 144
#define AT0100 __at(0x0100) // 0x100, 256
#define FLASH  __code
#else
#define IDATA
#define XDATA
#define AT0000
#define AT0008
#define AT0010
#define AT0040
#define AT0050
#define AT0080
#define AT0090
#define AT0100
#define FLASH
#endif

#endif
