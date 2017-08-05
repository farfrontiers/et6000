/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 display driver
 * for Microsoft Windows 2000/XP.
 * Copyright (c) 2003-2005, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/
#ifndef _ET6000ACCEL2D_H_
#define _ET6000ACCEL2D_H_

#include "driver.h"


/*****************************************************************************/
typedef struct {
    uint16 src_left; /* guaranteed constrained to virtual width and height */
    uint16 src_top;
    uint16 dest_left;
    uint16 dest_top;
    uint16 width; /* 0 to N, where zero means one pixel, one means two pixels, etc. */
    uint16 height; /* 0 to M, where zero means one line, one means two lines, etc. */
} blit_params;
/*****************************************************************************/
typedef struct {
    uint16 left; /* guaranteed constrained to virtual width and height */
    uint16 top;
    uint16 right;
    uint16 bottom;
} fill_rect_params;
/*****************************************************************************/
#define CLIP_LIMIT 50  /* We'll take 800 bytes of stack space */

typedef struct {
    LONG c;
    RECTL arcl[CLIP_LIMIT]; /* Space for enumerating complex clipping */
} ClipEnum;
/*****************************************************************************/
void et6000aclInit(volatile UCHAR *mmRegs, UCHAR bpp);

void et6000aclWaitIdle(volatile UCHAR *mmRegs);

BOOL DrvBitBlt(SURFOBJ *destSurf,
               SURFOBJ *srcSurf,
               SURFOBJ *maskSurf,
               CLIPOBJ *clip,
               XLATEOBJ *xlate,
               RECTL *destArea,
               POINTL *srcUpperLeft,
               POINTL *maskUpperLeft,
               BRUSHOBJ *brush,
               POINTL *brushUpperLeft,
               ROP4 rop4);

BOOL DrvCopyBits(SURFOBJ*  destSurf,
                 SURFOBJ*  srcSurf,
                 CLIPOBJ*  clip,
                 XLATEOBJ* xlate,
                 RECTL*    destArea,
                 POINTL*   srcUpperLeft);
/*****************************************************************************/


#endif /* _ET6000ACCEL2D_H_ */
