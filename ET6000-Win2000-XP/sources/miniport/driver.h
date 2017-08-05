/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 video miniport driver
 * for Microsoft Windows 2000/XP.
 * Copyright (c) 2003-2005, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/
#ifndef _ET6000DRIVER_H_
#define _ET6000DRIVER_H_

#include <miniport.h>
#include <ntddvdeo.h>
#include <devioctl.h>
#include <video.h>
#include <dderror.h>


/*****************************************************************************/
/* Set to 0 when compiling for Win2000, set to 1 when compiling for WinXP */
#define WINXP 1
/*****************************************************************************/
enum et6000ChipType {none = 0, et6000 = 1, et6100 = 2, et6300 = 3};
/*****************************************************************************/
struct ET6000DeviceExtension {
    enum et6000ChipType chip;

    VOID *framebuffer;
    PHYSICAL_ADDRESS physFramebuffer;
    
    VOID *memory;
    PHYSICAL_ADDRESS physMemory; /* PCI Base Address 0 */

    USHORT pciConfigSpace;
    PHYSICAL_ADDRESS physPCIConfigSpace; /* PCI Base Address 1 */

    VOID *mmRegs; /* memory mapped registers */
    PHYSICAL_ADDRESS physMMRegs;

    VOID *emRegs; /* external mapped registers */
    PHYSICAL_ADDRESS physEMRegs;

    ULONG memSize; /* onboard memory amount */

    ULONG availModesNum; /* number of available modes */
    ULONG curMode; /* index of current mode */
    
    VIDEO_ACCESS_RANGE accessRanges[PCI_TYPE0_ADDRESSES];
};

/*****************************************************************************/
/* Must be unique 32-bit identifiers */
#define ET6000_DDC_MONITOR 0x100CDDC1
#define ET6000_NONDDC_MONITOR 0x100CDDC0
/*****************************************************************************/
extern VIDEO_MODE_INFORMATION *et6000Modes;
/*****************************************************************************/
/*
 * How many bytes of memory the accelerator (ACL) takes at the end
 * of the adapter onboard memory to perform some of its operations.
 * This must be the same value in both display and miniport drivers!
 */
#define ET6000_ACL_NEEDS_MEMORY 4
/*****************************************************************************/


#endif /* _ET6000DRIVER_H_ */
