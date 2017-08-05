/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 display driver
 * for Microsoft Windows 2000/XP.
 * Copyright (c) 2003-2005, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "accel2d.h"


/*****************************************************************************/
/*
 * Set bits in a byte pointed by addr; mask must contain 0s at the bits
 * positions to be set and must contain 1s at all other bits; val must
 * contain the values of bits to be set.
 */
__inline void set8(volatile char *addr, char mask, char val)
{
    if (mask == 0)
        *addr = val;
    else
        *addr = (*addr & mask) | (val & ~mask);
}
/*****************************************************************************/
__inline char get8(volatile char *addr)
{
    return *addr;
}
/*****************************************************************************/
__inline void et6000aclTerminate(volatile UCHAR *mmRegs) {
    set8(mmRegs+0x31, 0xef, 0x10); /* let ACL to operate */
    et6000aclWaitIdle(mmRegs);
    set8(mmRegs+0x30, 0, 0x00);
    set8(mmRegs+0x30, 0, 0x01);
    et6000aclWaitIdle(mmRegs);
    set8(mmRegs+0x30, 0, 0x00);
    set8(mmRegs+0x30, 0, 0x10);
    et6000aclWaitIdle(mmRegs);
    set8(mmRegs+0x30, 0, 0x00);
}
/*****************************************************************************/
/*
 * bpp must be bytes per pixel, not bits!
 */
void et6000aclInit(volatile UCHAR *mmRegs, UCHAR bpp) {

    et6000aclTerminate(mmRegs);

    set8(mmRegs+0x31, 0xef, 0x10); /* let ACL to operate */
    set8(mmRegs+0x32, 0x99, 0x00); /* maximize the performance */
    set8(mmRegs+0x8e, 0xcf, (bpp - 1) << 4); /* set pixel color depth */
    set8(mmRegs+0x91, 0x80, 0x00); /* maximize the performance */
    set8(mmRegs+0x9d, 0x00, 0x00); /* maximize the performance */
}
/*****************************************************************************/
/*
 * Wait until ACL becomes idle.
 */
void et6000aclWaitIdle(volatile UCHAR *mmRegs) {
    while ((get8(mmRegs+0x36) & 0x02) == 0x02);
}
/*****************************************************************************/
/*
 * Wait until ACL queue becomes not full.
 */
__inline void et6000aclWaitQueueNotFull(volatile UCHAR *mmRegs) {
    while ((get8(mmRegs+0x36) & 0x01) == 0x01);
}
/*****************************************************************************/
/*
 * Move the specified rectangular region from one location in
 * the frame buffer to another.
 */
void screenToScreenBlit(blit_params *params,
                        uint16 screenWidth,
                        uint8 bpp, /* bytes(!) per pixel */
                        vuint8 *mmRegs)
{
    uint8 bltDir;
    uint16 src_left, src_top, dest_left, dest_top, width, height;
    uint32 srcAddr, destAddr;

    et6000aclWaitQueueNotFull(mmRegs);

    set8(mmRegs+0x92, 0x80, 0x77); /* no source wrap */
    set8(mmRegs+0x9c, 0x00, 0x33); /* mask=1 always, always use FGR */
    set8(mmRegs+0x9f, 0x00, 0xcc); /* FGR ROP = copy of source */

    /* Set the source Y offset */
    *((vuint16 *)(mmRegs+0x8a)) = screenWidth * bpp - 1;

    /* Set the destination Y offset */
    *((vuint16 *)(mmRegs+0x8c)) = screenWidth * bpp - 1;

    src_left = params->src_left;
    src_top = params->src_top;
    dest_left = params->dest_left;
    dest_top = params->dest_top;
    width = params->width;
    height = params->height;

    /* Set the direction and opcode(BitBLT) register */
    bltDir = 0x00;
    if (src_left < dest_left) bltDir |= 0x01;
    if (src_top < dest_top) bltDir |= 0x02;
    set8(mmRegs+0x8f, 0x3c, bltDir);

    /* Set the X count register */
    *((vuint16 *)(mmRegs+0x98)) = (width + 1) * bpp - 1;

    /* Set the Y count register */
    *((vuint16 *)(mmRegs+0x9a)) = height;

    switch (bltDir & 0x03) {
    case 0x00:
        srcAddr = (src_top * screenWidth + src_left) * bpp;
        destAddr = (dest_top * screenWidth + dest_left) * bpp;
        break;

    case 0x01:
        srcAddr =  (src_top * screenWidth + src_left + width) * bpp + bpp-1;
        destAddr = (dest_top * screenWidth + dest_left + width) * bpp + bpp-1;
        break;

    case 0x02:
        srcAddr = ((src_top + height)*screenWidth + src_left) * bpp;
        destAddr = ((dest_top + height)*screenWidth + dest_left) * bpp;
        break;

    case 0x03:
        srcAddr = ((src_top + height)*screenWidth + src_left + width) * bpp + bpp-1;
        destAddr = ((dest_top + height)*screenWidth + dest_left + width) * bpp + bpp-1;
        break;
    }

    /* Set the source address */
    *((vuint32 *)(mmRegs+0x84)) = srcAddr;

    /*
     * Set the destination address -
     * this action starts the BitBLT operation.
     */
    *((vuint32 *)(mmRegs+0xa0)) = destAddr;
}
/*****************************************************************************/
/*
 * Fill the specified rectangular regions with the specified color.
 */
void fillRectangle(fill_rect_params *params,
                   uint32 color,
                   uint16 screenWidth,
                   uint16 screenHeight,
                   uint8 bpp, /* bytes(!) per pixel */
                   vuint8 *mmRegs,
                   uint32 memSize,
                   PVOID memory,
                   PVOID framebuffer)
{
    uint16 left, top, right, bottom;
    uint32 srcAddr;
    uint8 i;

    /*
     * Normally WaitQueueNotFull should be required & enough, but in reality
     * this is somewhy sometimes not enough for pixel depth of 3 bytes.
     */
    if (bpp == 2)
        et6000aclWaitQueueNotFull(mmRegs);
    else
        et6000aclWaitIdle(mmRegs);

    /*
     * We'll put the color at 4 bytes just after the framebuffer. The srcAddr
     * must be 4 bytes aligned (and is always for standard resolutions).
     */
    srcAddr = (ULONG)framebuffer - (ULONG)memory +
        screenWidth * screenHeight * bpp;

    switch(bpp) {
        case 2:
            set8(mmRegs+0x92, 0x80, 0x02); /* 4x1 source wrap */
            for (i = 0; i < 2; i++) /* copy the color to source address */
                ((vuint16 *)((uint32)memory + srcAddr))[i] = (uint16)color;
            break;
        case 3:
            set8(mmRegs+0x92, 0x80, 0x0a); /* 3x1 source wrap */
            for (i = 0; i < 3; i++) /* copy the color to source address */
                ((vuint8 *)((uint32)memory + srcAddr))[i] = ((uint8 *)&color)[i];
            break;
    }

    set8(mmRegs+0x9c, 0x00, 0x33); /* mask=1 always, always use FGR */
    set8(mmRegs+0x9f, 0x00, 0xcc); /* FGR ROP = copy of source */

    /* Set the source Y offset */
    *((vuint16 *)(mmRegs+0x8a)) = screenWidth * bpp - 1;
    /* Set the destination Y offset */
    *((vuint16 *)(mmRegs+0x8c)) = screenWidth * bpp - 1;

    /* Set the direction and opcode(trapezoid) register (primary edge) */
    set8(mmRegs+0x8f, 0x18, 0x40);
    /* Set the secondary edge register */
    set8(mmRegs+0x93, 0x1a, 0x00);

    /* Set the primary delta minor register */
    *((vuint16 *)(mmRegs+0xac)) = 0;
    /* Set the secondary delta minor register */
    *((vuint16 *)(mmRegs+0xb4)) = 0;

    left = params->left;
    top = params->top;
    right = params->right;
    bottom = params->bottom;

    /* Set the X count register */
    *((vuint16 *)(mmRegs+0x98)) = (right-left+1)*bpp - 1;
    /* Set the Y count register */
    *((vuint16 *)(mmRegs+0x9a)) = bottom-top;

    /* Set the primary delta major register */
    *((vuint16 *)(mmRegs+0xae)) = bottom-top;

    /* Set the secondary delta major register */
    *((vuint16 *)(mmRegs+0xb6)) = bottom-top;

    /* Set the source address */
    *((vuint32 *)(mmRegs+0x84)) = srcAddr;

    /*
     * Set the destination address -
     * this action starts the trapezoid operation.
     */
    *((vuint32 *)(mmRegs+0xa0)) = (top * screenWidth + left) * bpp;
}
/*****************************************************************************/
/*
 * Check the intersection of two input rectangles (rect1 and rect2) and set
 * the intersection result in rectResult.
 * Returns TRUE if rect1 and rect2 intersect, the intersection will be in
 * rectResult. Returns FALSE if they don't intersect, rectResult is undefined.
 */
static BOOL intersect(RECTL* rect1,
                      RECTL* rect2,
                      RECTL* rectResult)
{
    rectResult->left = max(rect1->left, rect2->left);
    rectResult->right = min(rect1->right, rect2->right);

    /* Check if there is an intersection horizontally */
    if ( rectResult->left < rectResult->right ) {
        rectResult->top = max(rect1->top, rect2->top);
        rectResult->bottom = min(rect1->bottom, rect2->bottom);

        /* Check if there is an intersection vertically */
        if (rectResult->top < rectResult->bottom)
            return TRUE;
    }

    /* Return FALSE if there is no intersection */
    return FALSE;
}
/*****************************************************************************/
/*
 * This routine takes a list of rectangles from 'rect' and clips them
 * in-place to the rectangle 'clip'. The input rectangles don't
 * have to intersect 'clip'; the return value will reflect the
 * number of input rectangles that did intersect, and the intersecting
 * rectangles will be densely packed.
 */
static LONG intersect2(RECTL* clip,
                       RECTL* rect,
                       LONG rectNum)
{
    LONG isNum; /* number of intersections */
    RECTL* rectOut;

    isNum = 0;
    rectOut = rect; /* Put the result in place as the input */

    for (; rectNum != 0; rect++, rectNum--)
    {
        rectOut->left  = max(rect->left,  clip->left);
        rectOut->right = min(rect->right, clip->right);

        if ( rectOut->left < rectOut->right )
        {
            /*
             * Find intersection, horizontally, between current
             * rectangle and the clipping rectangle.
             */
            rectOut->top    = max(rect->top,    clip->top);
            rectOut->bottom = min(rect->bottom, clip->bottom);

            if ( rectOut->top < rectOut->bottom )
            {
                /*
                 * Find intersection, vertically, between current rectangle
                 * and the clipping rectangle. Put this rectangle in the
                 * result list and increment the counter. Ready for next input
                 */
                rectOut++;
                isNum++;
            }
        }
    } /* loop through all the input rectangles */

    return isNum;
}
/*****************************************************************************/
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
               ROP4 rop4)
{
    blit_params blit;
    fill_rect_params fill;
    ET6000PDev *ppdev;

    ppdev = (ET6000PDev *)(destSurf->dhpdev);

    if (ppdev != NULL) {

        if (                   
           (rop4 == 0xCCCC) && /* Copy the source */
           (maskSurf == NULL) &&
           ((xlate == NULL) || ((xlate != NULL) &&
               (xlate->flXlate & XO_TRIVIAL))) &&
           (destSurf == srcSurf) &&
           (((uint32)destSurf->pvBits >= (uint32)ppdev->memory) &&
               ((uint32)destSurf->pvBits < (uint32)ppdev->memory+0x400000))
           )
        {
            if ((clip == NULL) || (clip->iDComplexity == DC_TRIVIAL)) {
                blit.src_left = (uint16)srcUpperLeft->x;
                blit.src_top = (uint16)srcUpperLeft->y;
                blit.dest_left = (uint16)destArea->left;
                blit.dest_top = (uint16)destArea->top;
                blit.width = (uint16)(destArea->right - destArea->left - 1);
                blit.height = (uint16)(destArea->bottom - destArea->top - 1);
            }
            else
            if (clip->iDComplexity == DC_RECT) {
                RECTL is;

                if (intersect(destArea, &clip->rclBounds, &is)) {
                    blit.src_left = (uint16)(srcUpperLeft->x +
                        (is.left - destArea->left));
                    blit.src_top = (uint16)(srcUpperLeft->y +
                        (is.top - destArea->top));
                    blit.dest_left = (uint16)is.left;
                    blit.dest_top = (uint16)is.top;
                    blit.width = (uint16)(is.right - is.left - 1);
                    blit.height = (uint16)(is.bottom - is.top - 1);
                }
                else
                    goto renderBySoftware;
            }
            else {
                ClipEnum ce;
                LONG c, i;
                BOOL more;
                ULONG enumDir = CD_ANY;

                if(srcUpperLeft == NULL)
                    goto renderBySoftware;

                /*
                 * NOTE: we can safely shift by 16 because the
                 * surface stride will never be greater the 2--16
                 */
                if(((destArea->top - srcUpperLeft->y) << 16)
                    + (destArea->left - srcUpperLeft->x) > 0)
                    enumDir = CD_LEFTUP;
                else
                    enumDir = CD_RIGHTDOWN;

                CLIPOBJ_cEnumStart(clip, FALSE, CT_RECTANGLES, enumDir, 0);

                do {
                    more = CLIPOBJ_bEnum(clip, sizeof(ce), (ULONG*) &ce);

                    c = intersect2(destArea, ce.arcl, ce.c);

                    for (i = 0; i < c; i++) {
                        blit.src_left = (uint16)(srcUpperLeft->x +
                            (ce.arcl[i].left - destArea->left));
                        blit.src_top = (uint16)(srcUpperLeft->y +
                            (ce.arcl[i].top - destArea->top));
                        blit.dest_left = (uint16)ce.arcl[i].left;
                        blit.dest_top = (uint16)ce.arcl[i].top;
                        blit.width = (uint16)(ce.arcl[i].right -
                            ce.arcl[i].left - 1);
                        blit.height = (uint16)(ce.arcl[i].bottom -
                            ce.arcl[i].top - 1);

                        screenToScreenBlit(&blit, (uint16)ppdev->screenWidth,
                                              ppdev->bpp / 8, ppdev->mmRegs);
                    }
                } while (more);

                return TRUE;
            }

            screenToScreenBlit(&blit, (uint16)ppdev->screenWidth,
                                  ppdev->bpp / 8, ppdev->mmRegs);

            return TRUE;
        }


        if (
           (rop4 == 0xF0F0) && /* Copy the pattern */
           (srcSurf == NULL) &&
           (maskSurf == NULL) &&
           ((xlate == NULL) || ((xlate != NULL) &&
               (xlate->flXlate & XO_TRIVIAL))) &&
           (((uint32)destSurf->pvBits >= (uint32)ppdev->memory) &&
               ((uint32)destSurf->pvBits < (uint32)ppdev->memory+0x400000)) &&
           (brush->iSolidColor != 0xffffffff)
           )
        {
            if ((clip == NULL) || (clip->iDComplexity == DC_TRIVIAL)) {
                fill.left = (uint16)destArea->left;
                fill.top = (uint16)destArea->top;
                fill.right = (uint16)(destArea->right - 1);
                fill.bottom = (uint16)(destArea->bottom - 1);
            }
            else
            if (clip->iDComplexity == DC_RECT) {
                RECTL is;

                if (intersect(destArea, &clip->rclBounds, &is)) {
                    fill.left = (uint16)is.left;
                    fill.top = (uint16)is.top;
                    fill.right = (uint16)(is.right - 1);
                    fill.bottom = (uint16)(is.bottom - 1);
                }
                else
                    goto renderBySoftware;
            }
            else {
                ClipEnum ce;
                LONG c, i;
                BOOL more;

                CLIPOBJ_cEnumStart(clip, FALSE, CT_RECTANGLES, CD_ANY, 0);

                do {
                    more = CLIPOBJ_bEnum(clip, sizeof(ce), (ULONG*) &ce);

                    c = intersect2(destArea, ce.arcl, ce.c);

                    for (i = 0; i < c; i++) {
                        fill.left = (uint16)ce.arcl[i].left;
                        fill.top = (uint16)ce.arcl[i].top;
                        fill.right = (uint16)(ce.arcl[i].right - 1);
                        fill.bottom = (uint16)(ce.arcl[i].bottom - 1);

                        fillRectangle(&fill, brush->iSolidColor,
                            (uint16)ppdev->screenWidth,
                            (uint16)ppdev->screenHeight,
                            ppdev->bpp / 8, ppdev->mmRegs, ppdev->memSize,
                            ppdev->memory, ppdev->framebuffer);
                    }
                } while (more);

                return TRUE;
            }

            fillRectangle(&fill, brush->iSolidColor,
                (uint16)ppdev->screenWidth, (uint16)ppdev->screenHeight,
                ppdev->bpp / 8, ppdev->mmRegs, ppdev->memSize,
                ppdev->memory, ppdev->framebuffer);

            return TRUE;
        }
    }

renderBySoftware:

    return EngBitBlt(destSurf, srcSurf, maskSurf, clip, xlate, destArea,
              srcUpperLeft, maskUpperLeft, brush, brushUpperLeft, rop4);
}
/*****************************************************************************/
BOOL DrvCopyBits(SURFOBJ*  destSurf,
                 SURFOBJ*  srcSurf,
                 CLIPOBJ*  clip,
                 XLATEOBJ* xlate,
                 RECTL*    destArea,
                 POINTL*   srcUpperLeft)
{
    return DrvBitBlt(destSurf, srcSurf, NULL, clip, xlate, destArea,
                            srcUpperLeft, NULL, NULL, NULL, 0xCCCC);
}
/*****************************************************************************/
