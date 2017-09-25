// ;-----------------------------------------------------------------------------
// ;    This file is part of ds30 Loader.
// ;
// ;    ds30 Loader is free software: you can redistribute it and/or modify
// ;    it under the terms of the GNU General Public License as published by
// ;    the Free Software Foundation.
// ;
// ;    ds30 Loader is distributed in the hope that it will be useful
// ;    but WITHOUT ANY WARRANTY; without even the implied warranty of
// ;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// ;    GNU General Public License for more details.
// ;
// ;    You should have received a copy of the GNU General Public License
// ;    along with ds30 Loader. If not  see <http:;www.gnu.org/licenses/>.
// ;------------------------------------------------------------------------------

#include "boot_config.h"
#define PAGESIZER	8
#define ROWSIZEW	64
#ifdef  __PIC24FJ256GB106__
//#include      "p24FJ256GB106.inc"
#define         IS_24FJ                 1
#define         VALID_DEV               1
#define         FLASHSIZE               0x2AC00
#define         RAM_START               0x800
#define         RAM_SIZEB               16384
#define         DEVICEID        241
#define         EESIZEB         0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x1FFFL
#endif
#ifdef  __PIC24FJ128GB206__
//#include      "p24FJ128GB206.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x15800
#define         RAM_START               0x800
#define         RAM_SIZEB               98304
#define         DEVICEID                9
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ128GB210__
//#include      "p24FJ128GB210.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x15800
#define         RAM_START               0x800
#define         RAM_SIZEB               98304
#define         DEVICEID                17
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ256GB206__
//#include      "p24FJ256GB206.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x2AC00
#define         RAM_START               0x800
#define         RAM_SIZEB               98304
#define         DEVICEID                18
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ256GB210__
//#include      "p24FJ256GB210.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x2AC00
#define         RAM_START               0x800
#define         RAM_SIZEB               98304
#define         DEVICEID                19
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ256DA206__
//#include      "p24FJ256DA206.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x2AC00
#define         RAM_START               0x800
#define         RAM_SIZEB               98304
#define         DEVICEID                191
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ256DA210__
//#include      "p24FJ256DA210.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x2AC00
#define         RAM_START               0x800
#define         RAM_SIZEB               98304
#define         DEVICEID                192
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif

#ifdef  __PIC24FJ64GB106__
//#include      "p24FJ64GB106.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0xAC00
#define         RAM_START               0x800
#define         RAM_SIZEB               16384
#define         DEVICEID                217
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ64GB108__
//#include      "p24FJ64GB108.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0xAC00
#define         RAM_START               0x800
#define         RAM_SIZEB               16384
#define         DEVICEID                218
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ64GB110__
//#include      "p24FJ64GB110.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0xAC00
#define         RAM_START               0x800
#define         RAM_SIZEB               16384
#define         DEVICEID                219
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ128GB106__
//#include      "p24FJ128GB106.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x15800
#define         RAM_START               0x800
#define         RAM_SIZEB               16384
#define         DEVICEID                229
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ128GB108__
//#include      "p24FJ128GB108.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x15800
#define         RAM_START               0x800
#define         RAM_SIZEB               16384
#define         DEVICEID                230
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ128GB110__
//#include      "p24FJ128GB110.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x15800
#define         RAM_START               0x800
#define         RAM_SIZEB               16384
#define         DEVICEID                231
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ192GB106__
//#include      "p24FJ192GB106.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x20C00
#define         RAM_START               0x800
#define         RAM_SIZEB               16384
#define         DEVICEID                235
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ192GB108__
//#include      "p24FJ192GB108.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x20C00
#define         RAM_START               0x800
#define         RAM_SIZEB               16384
#define         DEVICEID                236
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ192GB110__
//#include      "p24FJ192GB110.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x20C00
#define         RAM_START               0x800
#define         RAM_SIZEB               16384
#define         DEVICEID                237
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ256GB108__
//#include      "p24FJ256GB108.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x2AC00
#define         RAM_START               0x800
#define         RAM_SIZEB               16384
#define         DEVICEID                242
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ256GB110__
//#include      "p24FJ256GB110.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x2AC00
#define         RAM_START               0x800
#define         RAM_SIZEB               16384
#define         DEVICEID                243
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ32GB002__
//#include      "p24FJ32GB002.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x5800
#define         RAM_START               0x800
#define         RAM_SIZEB               8192
#define         DEVICEID                244
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ32GB004__
//#include      "p24FJ32GB004.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x5800
#define         RAM_START               0x800
#define         RAM_SIZEB               8192
#define         DEVICEID                245
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ64GB002__
//#include      "p24FJ64GB002.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0xAC00
#define         RAM_START               0x800
#define         RAM_SIZEB               8192
#define         DEVICEID                246
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ64GB004__
//#include      "p24FJ64GB004.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0xAC00
#define         RAM_START               0x800
#define         RAM_SIZEB               8192
#define         DEVICEID                247
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ128DA106__
//#include      "p24FJ128DA106.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x15800
#define         RAM_START               0x800
#define         RAM_SIZEB               24576
#define         DEVICEID                250
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ128DA110__
//#include      "p24FJ128DA110.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x15800
#define         RAM_START               0x800
#define         RAM_SIZEB               24576
#define         DEVICEID                251
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ128DA206__
//#include      "p24FJ128DA206.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x15800
#define         RAM_START               0x800
#define         RAM_SIZEB               98304
#define         DEVICEID                252
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ128DA210__
//#include      "p24FJ128DA210.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x15800
#define         RAM_START               0x800
#define         RAM_SIZEB               98304
#define         DEVICEID                253
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ256DA106__
//#include      "p24FJ256DA106.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x2AC00
#define         RAM_START               0x800
#define         RAM_SIZEB               24576
#define         DEVICEID                254
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
#ifdef  __PIC24FJ256DA110__
//#include      "p24FJ256DA110.inc"
#define         IS_24FJ                         1
#define         VALID_DEV               1
#define         FLASHSIZE               0x2AC00
#define         RAM_START               0x800
#define         RAM_SIZEB               24576
#define         DEVICEID                255
#define         EESIZEB                 0
#define         HAS_UART2               1
#define         HAS_UART3               1
#define         HAS_UART4               1
#define         HAS_PPS                 1
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif

#ifndef USE_SECURE
#define BLPLP           2                       ;bootloader placement  pages from end
#define BLSIZEP 1                       ;bootloader size [pages] used by bootloader protection
#else
#define BLPLP           2                       ;bootloader placement  pages from end
#define BLSIZEP 2                       ;bootloader size [pages] used by bootloader protection
#define         BLSTARTADDR 0x400L
#define         BLENDADDR 0x23FFL
#endif
