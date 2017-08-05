/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 display driver
 * for Microsoft Windows 2000/XP.
 * Copyright (c) 2003-2005, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "driver.h"
#include "accel2d.h"


/***************************************************************************/
#define DEFAULT_FONT {16, 7, 0, 0, 700, 0, 0, 0, ANSI_CHARSET, \
    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,  \
    VARIABLE_PITCH | FF_DONTCARE, L"System"}

#define ANSIVAR_FONT {12, 9, 0, 0, 400, 0, 0, 0, ANSI_CHARSET, \
    OUT_DEFAULT_PRECIS, CLIP_STROKE_PRECIS, PROOF_QUALITY,     \
    VARIABLE_PITCH | FF_DONTCARE, L"MS Sans Serif"}

#define ANSIFIX_FONT {12, 9, 0, 0, 400, 0, 0, 0, ANSI_CHARSET, \
    OUT_DEFAULT_PRECIS, CLIP_STROKE_PRECIS, PROOF_QUALITY,     \
    FIXED_PITCH | FF_DONTCARE, L"Courier"}

DEVINFO et6000DefaultDevInfo = {
    0,            /* Graphics capabilities */
    DEFAULT_FONT, /* Default font description */
    ANSIVAR_FONT, /* ANSI variable font description */
    ANSIFIX_FONT, /* ANSI fixed font description */
    0,            /* Count of device fonts */
    0,            /* Preferred DIB format */
    0,            /* Width of color dither */
    0,            /* Height of color dither */
    0             /* Default palette to use for this device */
};
/***************************************************************************/
GDIINFO et6000DefaultGDIInfo =
{
    0x5000,                 // Major OS ver 5, minor OS ver 0, driver ver 0
    DT_RASDISPLAY,          // ulTechnology
    320,                    // ulHorzSize (filled in later)
    240,                    // ulVertSize (filled in later)
    0,                      // ulHorzRes (filled in later)
    0,                      // ulVertRes (filled in later)
    0,                      // cBitsPixel (filled in later)
    1,                      // cPlanes (filled in later)
    -1,                     // ulNumColors, not a palette managed
    0,                      // flRaster (DDI reserved field)
    96,                     // ulLogPixelsX (filled in later)
    96,                     // ulLogPixelsY (filled in later)
    TC_RA_ABLE,             // flTextCaps; TC_RA_ABLE is required for
                            // console applications (like FAR) to work
    8,                      // ulDACRed (filled in later)
    8,                      // ulDACGreen (filled in later)
    8,                      // ulDACBlue (filled in later)
    0x0024,                 // ulAspectX
    0x0024,                 // ulAspectY
    0x0033,                 // ulAspectXY (one-to-one aspect ratio)
    1,                      // xStyleStep
    1,                      // yStyleStep
    3,                      // denStyleStep -- Styles have a one-to-one aspect
                            // ratio, and every 'dot' is 3 pixels long
    { 0, 0 },               // ptlPhysOffset
    { 0, 0 },               // szlPhysSize
    0,                      // ulNumPalReg
    {                       // ciDevice
       { 6700, 3300, 0 },   //   Red
       { 2100, 7100, 0 },   //   Green
       { 1400,  800, 0 },   //   Blue
       { 1750, 3950, 0 },   //   Cyan
       { 4050, 2050, 0 },   //   Magenta
       { 4400, 5200, 0 },   //   Yellow
       { 3127, 3290, 0 },   //   AlignmentWhite
       20000,               //   RedGamma
       20000,               //   GreenGamma
       20000,               //   BlueGamma
       0, 0, 0, 0, 0, 0     //   No dye correction for raster displays
    },
    0,                      // ulDevicePelsDPI (for printers only)
    PRIMARY_ORDER_CBA,      // ulPrimaryOrder
    HT_PATSIZE_4x4_M,       // ulHTPatternSize
    0,                      // ulHTOutputFormat, filled later
    HT_FLAG_ADDITIVE_PRIMS, // flHTFlags
    0,                      // ulVRefresh
    1,                      // ulBltAlignment
    0,                      // ulPanningHorzRes
    0,                      // ulPanningVertRes
    0,                      // xPanningAlignment;
    0,                      // yPanningAlignment;
    0,                      // cxHTPat;
    0,                      // cyHTPat;
    0,                      // pHTPatA;
    0,                      // pHTPatB;
    0,                      // pHTPatC;
    0,                      // flShadeBlend;
    PPC_DEFAULT,            // ulPhysicalPixelCharacteristics;
    PPG_DEFAULT             // ulPhysicalPixelGamma;
};
/***************************************************************************/
/*
 * Determine the mode we should be in based on the DEVMODE passed in.
 * Query the miniport driver to get information needed to fill in
 * the devInfo and the gdiInfo.
 */
BOOL initPDev(ET6000PDev *ppdev,
              DEVMODEW *devMode,
              GDIINFO *gdiInfo,
              DEVINFO *devInfo)
{
    ULONG modesNum;
    PVIDEO_MODE_INFORMATION vmi, vmiSelected, vmiTemp;
    ULONG vmiStructSize;

    /* Get the modes information */
    modesNum = getModes(ppdev->hDriver, &vmi, &vmiStructSize);
    if (modesNum == 0)
        return FALSE;

    /* Find the requested mode in the list of available modes */
    vmiSelected = NULL;
    vmiTemp = vmi;
    while (modesNum > 0) {
        if ((vmiTemp->VisScreenWidth  == devMode->dmPelsWidth) &&
            (vmiTemp->VisScreenHeight == devMode->dmPelsHeight) &&
            (vmiTemp->BitsPerPlane * vmiTemp->NumberOfPlanes
                == devMode->dmBitsPerPel) &&
            (vmiTemp->Frequency  == devMode->dmDisplayFrequency))
        {
            vmiSelected = vmiTemp;
            break;
        }

        vmiTemp = (PVIDEO_MODE_INFORMATION)((ULONG)vmiTemp + vmiStructSize);
        modesNum--;
    }

    /* If no mode has been found, return an error */
    if (vmiSelected == NULL) {
        EngFreeMem(vmi);
        return FALSE;
    }

    ppdev->curMode = vmiSelected->ModeIndex;
    ppdev->screenWidth = vmiSelected->VisScreenWidth;
    ppdev->screenHeight = vmiSelected->VisScreenHeight;
    ppdev->bpp = (UCHAR)(vmiSelected->BitsPerPlane*vmiSelected->NumberOfPlanes);
    ppdev->screenStride = vmiSelected->ScreenStride;
    ppdev->redMask = vmiSelected->RedMask;
    ppdev->greenMask = vmiSelected->GreenMask;
    ppdev->blueMask = vmiSelected->BlueMask;

    *gdiInfo = et6000DefaultGDIInfo;
    gdiInfo->ulHorzSize = vmiSelected->XMillimeter;
    gdiInfo->ulVertSize = vmiSelected->YMillimeter;
    gdiInfo->ulHorzRes = ppdev->screenWidth;
    gdiInfo->ulVertRes = ppdev->screenHeight;
    gdiInfo->ulPanningHorzRes = ppdev->screenWidth;
    gdiInfo->ulPanningVertRes = ppdev->screenHeight;
    gdiInfo->cBitsPixel = vmiSelected->BitsPerPlane;
    gdiInfo->cPlanes = vmiSelected->NumberOfPlanes; /* must be 1 */
    gdiInfo->ulVRefresh = vmiSelected->Frequency;
    gdiInfo->ulLogPixelsX = devMode->dmLogPixels;
    gdiInfo->ulLogPixelsY = devMode->dmLogPixels;
    gdiInfo->ulDACRed   = vmiSelected->NumberRedBits;
    gdiInfo->ulDACGreen = vmiSelected->NumberGreenBits;
    gdiInfo->ulDACBlue  = vmiSelected->NumberBlueBits;

    *devInfo = et6000DefaultDevInfo;

    switch (ppdev->bpp) {
    case 16:
        gdiInfo->ulHTOutputFormat = HT_FORMAT_16BPP;
        devInfo->iDitherFormat = BMF_16BPP;
        break;
    case 24:
        gdiInfo->ulHTOutputFormat = HT_FORMAT_24BPP;
        devInfo->iDitherFormat = BMF_24BPP;
        break;
    }

    EngFreeMem(vmi);

    return TRUE;
}
/***************************************************************************/
/*
 * Enables the surface and maps the frame buffer into memory.
 */
BOOL initSurf(ET6000PDev *ppdev, BOOL firstTime)
{
    ULONG bytesReturned;
    VIDEO_MEMORY videoMemory;
    VIDEO_MEMORY_INFORMATION vmi;
    VIDEO_PUBLIC_ACCESS_RANGES pciConfigSpace;

    /* Set the current mode into the hardware */
    if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_SET_CURRENT_MODE,
             &(ppdev->curMode), sizeof(ULONG), NULL, 0, &bytesReturned))
    {
        return FALSE;
    }

    /* If this is the first time we enable the surface. */
    if (firstTime) {

        /* Map the adapter hardware memory. */
        
        videoMemory.RequestedVirtualAddress = NULL;

        if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                &videoMemory, sizeof(VIDEO_MEMORY), &vmi,
                sizeof(VIDEO_MEMORY_INFORMATION), &bytesReturned))
        {
            return FALSE;
        }

        ppdev->framebuffer = vmi.FrameBufferBase;
        ppdev->memory = vmi.VideoRamBase;
        ppdev->mmRegs = (PVOID)((ULONG)vmi.VideoRamBase + 0x003fff00);
        ppdev->emRegs = (PVOID)((ULONG)vmi.VideoRamBase + 0x003fe000);
        ppdev->memSize = vmi.VideoRamLength;

        /* Make sure we can access this video memory */
        *(volatile ULONG *)(ppdev->framebuffer) = 0xaa55aa55;
        if (*(volatile ULONG *)(ppdev->framebuffer) != 0xaa55aa55)
            return FALSE;

        /* Get the PCI configuration space base I/O address */

        if (EngDeviceIoControl(ppdev->hDriver,
                IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES,
                NULL, 0, &pciConfigSpace,
                sizeof(VIDEO_PUBLIC_ACCESS_RANGES), &bytesReturned))
        {
            return FALSE;
        }

        ppdev->pciConfigSpace = (USHORT)pciConfigSpace.VirtualAddress;
    }

    et6000aclInit(ppdev->mmRegs, ppdev->bpp / 8);

    return TRUE;
}
/***************************************************************************/
/*
 * Gets the list of modes supported by miniport driver;
 * returns the number of modes in the vmi buffer;
 * vmi buffer must be freed by the caller.
 */
ULONG getModes(HANDLE hDriver,
               VIDEO_MODE_INFORMATION **vmi,
               ULONG *vmiStructSize)
{
    VIDEO_NUM_MODES modes;
    ULONG i;

    /* Get the number of modes supported by the miniport driver */
    if (EngDeviceIoControl(hDriver, IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
                   NULL, 0, &modes, sizeof(VIDEO_NUM_MODES), &i))
    {
        return 0;
    }

    *vmiStructSize = modes.ModeInformationLength;

    /* Allocate the buffer for modes */
    *vmi = (PVIDEO_MODE_INFORMATION) EngAllocMem(0, modes.NumModes *
                   modes.ModeInformationLength, ET6000DD_ALLOC_TAG);

    if (*vmi == NULL)
        return 0;

    /* Query the modes from miniport driver */
    if (EngDeviceIoControl(hDriver, IOCTL_VIDEO_QUERY_AVAIL_MODES, NULL,
             0, *vmi, modes.NumModes * modes.ModeInformationLength, &i))
    {
        EngFreeMem(*vmi);
        *vmi = NULL;
        return 0;
    }

    return modes.NumModes;
}
/***************************************************************************/
