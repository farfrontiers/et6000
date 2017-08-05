/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 display driver
 * for Microsoft Windows 2000/XP.
 * Copyright (c) 2003-2005, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "driver.h"
#include "accel2d.h"


/***************************************************************************/
DHPDEV DrvEnablePDEV(
    DEVMODEW *pdm,
    PWSTR    pwszLogAddress,  /* ignored by DD */
    ULONG    cPat,            /* ignored by DD */
    HSURF    *phsurfPatterns, /* ignored by DD */
    ULONG    devCapsLen, /* Length of memory pointed to by devCaps */
    ULONG    *devCaps,   /* Pointer to GDIINFO structure */
    ULONG    devInfoLen, /* Length of memory pointed to by devInfo */
    DEVINFO  *devInfo,
    HDEV     hdev, /* GDI-supplied handle to the device, used for some GDI callbacks */
    PWSTR    pwszDeviceName,
    HANDLE   hDriver) /* Identifies the kernel-mode driver that supports the device */
{
    GDIINFO gdiInfoTemp;
    DEVINFO devInfoTemp;
    ET6000PDev *ppdev = NULL;

    /* Allocate a physical device structure */
    ppdev = (ET6000PDev *)EngAllocMem(FL_ZERO_MEMORY,
             sizeof(ET6000PDev), ET6000DD_ALLOC_TAG);

    if (ppdev == NULL)
        return (DHPDEV)0;

    ppdev->hDriver = hDriver;
    ppdev->flHooks = HOOK_SYNCHRONIZE | HOOK_BITBLT | HOOK_COPYBITS;

    /*
     * Get the current screen mode information.
     * Set up device caps and devinfo.
     */
    if (!initPDev(ppdev, pdm, &gdiInfoTemp, &devInfoTemp)) {
        goto failure;
    }

    /* Create the default palette */
    ppdev->hPalette = devInfoTemp.hpalDefault =
        EngCreatePalette(PAL_BITFIELDS, 0, NULL, ppdev->redMask,
                             ppdev->greenMask, ppdev->blueMask);

    if (ppdev->hPalette == (HPALETTE)0)
        goto failure;

    /* Copy the devinfo into the engine buffer. */
    memcpy(devInfo, &devInfoTemp, min(sizeof(DEVINFO), devInfoLen));

    /*
     * Set the devCaps with gdiInfoTemp we have prepared
     * to the list of caps for this pdev.
     */
    memcpy(devCaps, &gdiInfoTemp, min(sizeof(GDIINFO), devCapsLen));

    return (DHPDEV)ppdev;

failure:
    EngFreeMem(ppdev);
    return (DHPDEV)0;
}
/***************************************************************************/
VOID DrvCompletePDEV(DHPDEV dhpdev,
                     HDEV hdev)
{
    ((ET6000PDev *)dhpdev)->hDev = hdev;
}
/***************************************************************************/
VOID DrvDisablePDEV(DHPDEV dhpdev)
{
#define ppdev ((ET6000PDev *)dhpdev)

   /* Delete the default palette */
    if (ppdev->hPalette) {
        EngDeletePalette(ppdev->hPalette);
        ppdev->hPalette = (HPALETTE)0;
    }

    EngFreeMem(dhpdev);

#undef ppdev
}
/***************************************************************************/
HSURF DrvEnableSurface(DHPDEV dhpdev)
{
#define ppdev ((ET6000PDev *)dhpdev)
    HSURF hSurf;
    SIZEL sizel;
    ULONG formatCompat;

    /* Create engine bitmap around frame buffer. */
    if (!initSurf(ppdev, TRUE)) {
        return FALSE;
    }

    sizel.cx = ppdev->screenWidth;
    sizel.cy = ppdev->screenHeight;

    switch (ppdev->bpp) {
    case 16:
        formatCompat = BMF_16BPP;
        break;
    case 24:
        formatCompat = BMF_24BPP;
        break;
    }

    hSurf = (HSURF)EngCreateDeviceSurface((DHSURF)ppdev, sizel, formatCompat);

    if (hSurf == (HSURF)0)
        return FALSE;

    if (!EngModifySurface(hSurf, ppdev->hDev, ppdev->flHooks,
                          MS_NOTSYSTEMMEMORY, (DHSURF)ppdev,
                          ppdev->framebuffer, ppdev->screenStride, NULL))
    {
        return FALSE;
    }

    ppdev->hSurf = hSurf;

    return hSurf;

#undef ppdev
}
/***************************************************************************/
VOID DrvDisableSurface(DHPDEV dhpdev)
{
#define ppdev ((ET6000PDev *)dhpdev)
    VIDEO_MEMORY videoMemory;
    ULONG i;

    EngDeleteSurface(ppdev->hSurf);

    videoMemory.RequestedVirtualAddress = ppdev->framebuffer;

    /* Unmap the adapter hardware memory */
    EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                     &videoMemory, sizeof(VIDEO_MEMORY), NULL, 0, &i);

    ppdev->hSurf = (HSURF)0;

#undef ppdev
}
/***************************************************************************/
BOOL DrvAssertMode(DHPDEV dhpdev,
                   BOOL enable)
{
#define ppdev ((ET6000PDev *)dhpdev)
    ULONG bytesReturned;
    PVOID framebuffer;

    if (enable) {

        /* The screen must be reenabled, reinitialize the device to clean state. */
        framebuffer = ppdev->framebuffer;

        if (!initSurf(ppdev, TRUE))
            return FALSE;

        if (framebuffer != ppdev->framebuffer) {
            if (!EngModifySurface(ppdev->hSurf, ppdev->hDev, ppdev->flHooks,
                    MS_NOTSYSTEMMEMORY, (DHSURF)ppdev, ppdev->framebuffer,
                    ppdev->screenStride, NULL))
            {
                return FALSE;
            }
        }

        return TRUE;
    }
    else {
        /*
         * We must give up the display.
         * Call the kernel driver to reset the device to a known state.
         */
        if (EngDeviceIoControl(ppdev->hDriver, IOCTL_VIDEO_RESET_DEVICE,
                                      NULL, 0, NULL, 0, &bytesReturned))
            return FALSE;
        else
            return TRUE;
    }

#undef ppdev
}
/***************************************************************************/
ULONG DrvGetModes(HANDLE hDriver,
                  ULONG pdmLen,
                  DEVMODEW *pdm)
{
    ULONG modesNum;
    ULONG retVal;
    PVIDEO_MODE_INFORMATION vmi, vmiTemp;
    ULONG dmNum = pdmLen / (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    ULONG vmiStructSize;

    modesNum = getModes(hDriver, &vmi, &vmiStructSize);

    if (modesNum == 0)
        return 0;

    if (pdm == NULL) {
        retVal = modesNum * (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    }
    else {
        /*
         * Now copy the information for the supported
         * modes back into the output buffer.
         */

        retVal = 0;
        vmiTemp = vmi;

        while ((dmNum > 0) && (modesNum > 0)) {

            memset(pdm, 0, sizeof(DEVMODEW));

            /* Set the name of the device to the name of the DLL. */
            memcpy(pdm->dmDeviceName, DLL_NAME, sizeof(DLL_NAME));

            pdm->dmSpecVersion = DM_SPECVERSION;
            pdm->dmDriverVersion = DM_SPECVERSION;
            pdm->dmSize = sizeof(DEVMODEW);
            pdm->dmDriverExtra = DRIVER_EXTRA_SIZE;

            pdm->dmBitsPerPel = vmiTemp->NumberOfPlanes * vmiTemp->BitsPerPlane;
            pdm->dmPelsWidth = vmiTemp->VisScreenWidth;
            pdm->dmPelsHeight = vmiTemp->VisScreenHeight;
            pdm->dmDisplayFrequency = vmiTemp->Frequency;
            pdm->dmDisplayFlags = 0;

            pdm->dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                                    DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS;

            /* Go to the next DEVMODE entry in the buffer. */
            pdm = (LPDEVMODEW)((ULONG)pdm+sizeof(DEVMODEW)+DRIVER_EXTRA_SIZE);
            dmNum--;

            vmiTemp = (PVIDEO_MODE_INFORMATION)((ULONG)vmiTemp+vmiStructSize);
            modesNum--;

            retVal += (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
        }
    }

    EngFreeMem(vmi);

    return retVal;
}
/***************************************************************************/
VOID DrvSynchronize(DHPDEV dhpdev,
                    RECTL *prcl)
{
    et6000aclWaitIdle(((ET6000PDev *)dhpdev)->mmRegs);
}
/***************************************************************************/
VOID DrvDisableDriver(VOID)
{
    return;
}
/***************************************************************************/
#if WINXP
ULONG DrvResetDevice(DHPDEV dhpdev,
                     PVOID Reserved)
{
    return DRD_ERROR;
}
#endif /* WINXP */
/***************************************************************************/
DRVFN et6000drvfn[] = {
    {INDEX_DrvAssertMode,            (PFN) DrvAssertMode        },
    {INDEX_DrvCompletePDEV,          (PFN) DrvCompletePDEV      },
    {INDEX_DrvDisablePDEV,           (PFN) DrvDisablePDEV       },
    {INDEX_DrvDisableDriver,         (PFN) DrvDisableDriver     },
    {INDEX_DrvDisableSurface,        (PFN) DrvDisableSurface    },
    {INDEX_DrvEnablePDEV,            (PFN) DrvEnablePDEV        },
    {INDEX_DrvEnableSurface,         (PFN) DrvEnableSurface     },
    {INDEX_DrvSynchronize,           (PFN) DrvSynchronize       },
    {INDEX_DrvGetModes,              (PFN) DrvGetModes          },
    {INDEX_DrvBitBlt,                (PFN) DrvBitBlt            },
    {INDEX_DrvCopyBits,              (PFN) DrvCopyBits          },
//    {INDEX_DrvEnableDirectDraw,      (PFN) DrvEnableDirectDraw  },
//    {INDEX_DrvDisableDirectDraw,     (PFN) DrvDisableDirectDraw },
//    {INDEX_DrvGetDirectDrawInfo,     (PFN) DrvGetDirectDrawInfo },
#if WINXP
    {INDEX_DrvResetDevice,           (PFN) DrvResetDevice       }
#endif /* WINXP */
};

#define ET6000DD_IMPLEMENTED_FUNCTIONS_NUMBER \
    (sizeof(et6000drvfn) / sizeof(et6000drvfn[0]))
/***************************************************************************/
BOOL DrvEnableDriver(ULONG iEngineVersion,
                     ULONG cj,
                     PDRVENABLEDATA pded)
{
    pded->iDriverVersion = DDI_DRIVER_VERSION_NT5;
    pded->c = ET6000DD_IMPLEMENTED_FUNCTIONS_NUMBER;
    pded->pdrvfn = et6000drvfn;

    return TRUE;
}
/***************************************************************************/
