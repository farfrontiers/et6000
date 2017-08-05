/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 video miniport driver
 * for Microsoft Windows 2000/XP.
 * Copyright (c) 2003-2005, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "setmode.h"


#if USE_INT10

/*
 * ATTENTION: Here we set the graphics modes using INT 10h service of
 * VideoPortInt10 function.
 */

/*****************************************************************************/
ULONG et6000SetRefreshRate(struct ET6000DeviceExtension *de,
                           ULONG mode)
{
    VIDEO_X86_BIOS_ARGUMENTS ba;
    VIDEO_MODE_INFORMATION *mi = &et6000Modes[mode];
    ULONG status;

    VideoPortZeroMemory(&ba, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    ba.Eax = 0x00001202; /* AH=0x12, AL=0x02 */

    switch (mi->VisScreenWidth) {
    case 640: /* 640x480 */
        ba.Ebx = 0x000000f1; break;
    case 800: /* 800x600 */
        ba.Ebx = 0x000001f1; break;
    case 1024: /* 1024x768 */
        ba.Ebx = 0x000002f1; break;
    case 1280: /* 1280x1024 */
        ba.Ebx = 0x000003f1; break;
    case 1152: /* 1152x864 */
        ba.Ebx = 0x000004f1; break;
    default:
        ba.Ebx = 0x000001f1; break;
    }

    ba.Ecx = mi->Frequency;

    status = VideoPortInt10(de, &ba);

    if ((status == NO_ERROR) &&
        (((ba.Eax & 0x000000ff) != 0x12) || /* AL must be 0x12 if success */
          (ba.Ecx != mi->Frequency)))
        status = ERROR_INVALID_PARAMETER;

    return status;
}
/*****************************************************************************/
ULONG et6000SetGraphicsMode(struct ET6000DeviceExtension *de,
                            ULONG mode)
{
    VIDEO_X86_BIOS_ARGUMENTS ba;
    VIDEO_MODE_INFORMATION *mi = &et6000Modes[mode];
    ULONG status;

    et6000EnableLinearMemoryMapping(de->pciConfigSpace);

    VideoPortZeroMemory(&ba, sizeof(VIDEO_X86_BIOS_ARGUMENTS));

    switch (mi->BitsPerPlane) {
    case 24: {
        ba.Eax = 0x000010f0; /* AH=0x10, AL=0xf0 */
        switch (mi->VisScreenWidth) {
        case 640:
            ba.Ebx = 0x00002eff; break;
        case 800:
            ba.Ebx = 0x000030ff; break;
        case 1024:
            ba.Ebx = 0x000038ff; break;
        case 1280:
            ba.Ebx = 0x00003fff; break;
        default:
            ba.Ebx = 0x00002eff; break;
        }
        status = VideoPortInt10(de, &ba);
        if ((ba.Eax & 0x0000ffff) != 0x0010) /* AH must be 0x00, AL=0x10 */
            status = ERROR_INVALID_PARAMETER;
        break;
    }

    case 16: {
        if (mi->NumberGreenBits == 5)
            ioSet8(de->pciConfigSpace+0x58, 0xfd, 0x00); /* 16bpp is 5:5:5 */
        else
            ioSet8(de->pciConfigSpace+0x58, 0xfd, 0x02); /* 16bpp is 5:6:5 */

        ba.Eax = 0x000010f0; /* AH=0x10, AL=0xf0 */
        switch (mi->VisScreenWidth) {
        case 640:
            ba.Ebx = 0x0000002e; break;
        case 800:
            ba.Ebx = 0x00000030; break;
        case 1024:
            ba.Ebx = 0x00000038; break;
        case 1280:
            ba.Ebx = 0x0000003f; break;
        default:
            ba.Ebx = 0x0000002e; break;
        }
        status = VideoPortInt10(de, &ba);
        if ((ba.Eax & 0x0000ffff) != 0x0010) /* AH must be 0x00, AL=0x10 */
            status = ERROR_INVALID_PARAMETER;
        break;
    }
    }

    et6000EnableLinearMemoryMapping(de->pciConfigSpace);

    if (status != NO_ERROR)
        return status;

    status = et6000SetRefreshRate(de, mode);

    if (status == NO_ERROR)
        de->curMode = mode;

    return status;
}
/*****************************************************************************/
ULONG et6000SetMode(struct ET6000DeviceExtension *de, ULONG mode) {
    mode &= ~(VIDEO_MODE_NO_ZERO_MEMORY | VIDEO_MODE_MAP_MEM_LINEAR);
    return et6000SetGraphicsMode(de, mode);
}
/*****************************************************************************/

#endif /* USE_INT10 */
