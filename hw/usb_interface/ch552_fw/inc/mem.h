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
#define AT00C0 __at(0x00C0) // 0x0C0, 192
#define AT00C8 __at(0x00C8) // 0x0C8, 200
#define AT0110 __at(0x0110) // 0x110, 272
#define AT0148 __at(0x0148) // 0x148, 328
#define AT01C8 __at(0x01C8) // 0x1C8, 456
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
#define AT00C0
#define AT00C8
#define AT0110
#define AT0148
#define AT01C8
#define FLASH
#endif

#endif
