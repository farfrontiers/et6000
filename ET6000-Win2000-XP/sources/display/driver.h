/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 display driver
 * for Microsoft Windows 2000/XP.
 * Copyright (c) 2003-2005, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/
#ifndef _ET6000DRIVER_H_
#define _ET6000DRIVER_H_

#include <stdarg.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>
#include <devioctl.h>
#include <ntddvdeo.h>


/*****************************************************************************/
/* Set to 0 when compiling for Win2000, set to 1 when compiling for WinXP */
#define WINXP 1
/*****************************************************************************/
typedef struct _ET6000PDev {
    HANDLE  hDriver;       /* Handle to \Device\Screen */
    HDEV    hDev;          /* Engine's handle to PDEV */
    HSURF   hSurf;         /* Engine's handle to surface */
    HPALETTE hPalette;     /* Handle to the default palette for device */
    PVOID   framebuffer;   /* Pointer to the framebuffer */
    PVOID   memory;        /* Pointer to the mapped onboard memory */
    volatile void *mmRegs; /* Memory mapped registers */
    volatile void *emRegs; /* External mapped registers */
    USHORT  pciConfigSpace;/* PCI configuation space base I/O address */
    ULONG   memSize;       /* Amount of onboard memory of adapter */
    ULONG   screenWidth;   /* Visible screen width */
    ULONG   screenHeight;  /* Visible screen height */
    LONG    screenStride;  /* Bytes from one scan line to the next one */
    FLONG   redMask;       /* For bitfields device, red mask */
    FLONG   greenMask;     /* For bitfields device, green mask */
    FLONG   blueMask;      /* For bitfields device, blue mask */
    UCHAR   bpp;           /* Bits per pixel; 16 and 24 are only supported */
    ULONG   curMode;       /* Current mode of the miniport driver */
    FLONG   flHooks;
} ET6000PDev;
/*****************************************************************************/
ULONG getModes(HANDLE, PVIDEO_MODE_INFORMATION *, ULONG *);
BOOL initPDev(ET6000PDev *, PDEVMODEW, GDIINFO *, DEVINFO *);
BOOL initSurf(ET6000PDev *, BOOL);
/*****************************************************************************/
#define ET6000DD_ALLOC_TAG 0x36544544
#define DLL_NAME L"et6000dll"
#define DRIVER_EXTRA_SIZE 0
#define STANDARD_DEBUG_PREFIX "ET6000dd: "
/*****************************************************************************/
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef volatile uint8 vuint8;
typedef volatile uint16 vuint16;
typedef volatile uint32 vuint32;
/*****************************************************************************/


#endif /* _ET6000DRIVER_H_ */
