/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 video miniport driver
 * for Microsoft Windows 2000/XP.
 * Copyright (c) 2003-2005, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/
#ifndef _ET6000SETMODE_H_
#define _ET6000SETMODE_H_

#include "driver.h"
#include "bits.h"


/*****************************************************************************/
/*
 * If not 0 then VideoPortInt10 calls are used to set the screen mode. If 0
 * then direct programming of VGA registers is used to set the screen mode.
 */
#define USE_INT10 1
/*****************************************************************************/
__inline void et6000EnableLinearMemoryMapping(USHORT pciConfigSpace)
{
    /*
     * Relocate memory via PCI Base Address 0; don't enable MMU;
     * enable memory mapped registers; enable system linear memory mapping.
     */
    ioSet8(pciConfigSpace+0x40, 0xf0, 0x0b);
}
/*****************************************************************************/

#if USE_INT10

/*****************************************************************************/
__inline ULONG et6000GetOnboardMemorySize(struct ET6000DeviceExtension *de)
{
    VIDEO_X86_BIOS_ARGUMENTS ba;
    ULONG memSize;

    ba.Eax = 0x00001203; /* AH=0x12, AL=0x03 */
    ba.Ebx = 0x000000f2; /* BL=0xf2 */
    ba.Ecx = 0;
    ba.Edx = 0;
    ba.Esi = 0;
    ba.Edi = 0;
    ba.Ebp = 0;

    if ((VideoPortInt10(de, &ba) == NO_ERROR)
        && ((ba.Eax & 0x000000ff) == 0x12)) /* AL must be 0x12 */
    {
        /* BX contains the video memory size in 64Kbytes */
        memSize = (ba.Ebx & 0x0000ffff) * 64*1024;
    }
    else {
        memSize = 4*1024*1024; /* Maximum can be */
    }

    if (memSize == 2621440) { /* If 2.5Mb detected */
        UCHAR pci40 = ioGet8(de->pciConfigSpace+0x40);
        et6000EnableLinearMemoryMapping(de->pciConfigSpace);

        /* Check whether the memory beyond 2.25Mb really exists */
        *(volatile ULONG *)((ULONG)de->memory + 2359296) = 0xaa55aa55;
        if (*(volatile ULONG *)((ULONG)de->memory + 2359296) != 0xaa55aa55)
            memSize = 2359296; /* It's 2.25Mb */

        ioSet8(de->pciConfigSpace+0x40, 0x00, pci40); /* Restore */
    }

    return memSize;
}
/*****************************************************************************/

#else /* USE_INT10 */

/*****************************************************************************/
__inline ULONG  et6000GetOnboardMemorySize(struct ET6000DeviceExtension *de)
{
    ULONG memSize = 0;
    USHORT pciConfigSpace = de->pciConfigSpace;
    VOID *memory = de->memory;

    ioSet8(0x3d8, 0x00, 0xa0); /* Set the KEY for color modes */
    ioSet8(0x3b8, 0x00, 0xa0); /* Set the KEY for monochrome modes */

    switch (ioGet8(0x3C2) & 0x03) {
    case 0x00: /* onboard memory is of DRAM type */
        memSize = 1024*1024 * ((ioGet8(pciConfigSpace + 0x45) & 0x03) + 1);
        break;
    case 0x03: /* onboard memory is of MDRAM type */
        memSize = /* number*8 of 32kb banks per channel */
            ((ioGet8(pciConfigSpace + 0x47) & 0x07) + 1) * 8 * 32*1024;
        if (ioGet8(pciConfigSpace + 0x45) & 0x04) /* If 2 channels */
            memSize *= 2;
        break;
    default:  /* onboard memory is of unknown type */
        memSize = 4196*1024; /* Let it be of maximum possible size */
    }

    /*
     * This algorithm would fail to recongize 2.25Mb of onboard
     * memory - it would detect 2.5Mb instead. It needs to be fixed.
     */
    if (memSize == 2621440) { /* If 2.5Mb detected */
        UCHAR pci40 = ioGet8(pciConfigSpace+0x40);
        et6000EnableLinearMemoryMapping(pciConfigSpace);

        /* Check whether the memory beyond 2.25Mb really exists */
        *(volatile ULONG *)((ULONG)memory + 2359296) = 0xaa55aa55;
        if (*(volatile ULONG *)((ULONG)memory + 2359296) != 0xaa55aa55)
            memSize = 2359296; /* It's 2.25Mb */

        ioSet8(pciConfigSpace+0x40, 0x00, pci40); /* Restore */
    }

    return memSize;
}
/*****************************************************************************/

#endif /* USE_INT10 */

/*****************************************************************************/
ULONG et6000SetMode(struct ET6000DeviceExtension *de, ULONG mode);
VOID et6000SetTextMode(struct ET6000DeviceExtension *de);
/*****************************************************************************/


#endif /* _ET6000SETMODE_H_ */
