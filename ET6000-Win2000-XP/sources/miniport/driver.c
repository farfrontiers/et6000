/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 video miniport driver
 * for Microsoft Windows 2000/XP.
 * Copyright (c) 2003-2005, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/

#include "driver.h"
#include "setmode.h"
#include "acl.h"


/* RAMDACs frequencies: ET6000 = 135MHz, ET6100 = 175MHz, ET6300 = 220MHz */

/*****************************************************************************/
struct ET6000Resolutions {
    USHORT width;
    USHORT height;
} et6000Res[] = { {640, 480}, {800, 600}, {1024, 768}, /*{1152, 864},*/ {1280, 1024} };

#define ET6000_RESOLUTIONS_NUMBER \
    (sizeof(et6000Res) / sizeof(et6000Res[0]))
/*****************************************************************************/
struct ET6000ColorModes {
    UCHAR bpp;
    UCHAR redBitsNum;
    UCHAR greenBitsNum;
    UCHAR blueBitsNum;
    ULONG redMask;
    ULONG greenMask;
    ULONG blueMask;
} et6000ColorModes[] = {
    {16, 5, 5, 5, 0x00007c00, 0x000003e0, 0x0000001f}, /* 5:5:5 */
    {16, 5, 6, 5, 0x0000f800, 0x000007e0, 0x0000001f}, /* 5:6:5 */
    {24, 8, 8, 8, 0x00ff0000, 0x0000ff00, 0x000000ff}  /* 8:8:8 */
};

#define ET6000_COLOR_MODES_NUMBER \
    (sizeof(et6000ColorModes) / sizeof(et6000ColorModes[0]))
/*****************************************************************************/
UCHAR et6000RefreshRates[] = {56, 60, 70, 72, 75, 85, 90, 100, 110, 120};

#define ET6000_REFRESH_RATES_NUMBER \
    (sizeof(et6000RefreshRates) / sizeof(et6000RefreshRates[0]))
/*****************************************************************************/
VIDEO_MODE_INFORMATION *et6000Modes = NULL;
/*****************************************************************************/
/*
 * Construct the modes structures.
 */
BOOLEAN et6000ConstructModes(struct ET6000DeviceExtension *de)
{
    USHORT i,j,k,l;
    ULONG fbMaxSize; /* maximum size of framebuffer */

    de->availModesNum = ET6000_RESOLUTIONS_NUMBER *
        ET6000_COLOR_MODES_NUMBER * ET6000_REFRESH_RATES_NUMBER;

#if WINXP
    et6000Modes = VideoPortAllocatePool(de, VpNonPagedPool,
        de->availModesNum * sizeof(VIDEO_MODE_INFORMATION), 0xDD6000DD);
#else /* WINXP */
    VideoPortAllocateBuffer(de,
        de->availModesNum * sizeof(VIDEO_MODE_INFORMATION), &et6000Modes);
#endif /* WINXP */

    if (et6000Modes == NULL)
        return FALSE;

    /* Framebuffer must not overlap with the memory mapped registers */
    if (de->memSize > 0x3fe000)
        fbMaxSize = 0x3fe000;
    else
        fbMaxSize = de->memSize;

    fbMaxSize -= ET6000_ACL_NEEDS_MEMORY;

    l = 0;
    for (i = 0; i < ET6000_RESOLUTIONS_NUMBER; i++) {
      for (j = 0; j < ET6000_COLOR_MODES_NUMBER; j++) {
        for (k = 0; k < ET6000_REFRESH_RATES_NUMBER; k++)
        {
          /* Check whether this mode is valid with current amount of memory. */
          if ((ULONG)(et6000Res[i].width * et6000Res[i].height *
              et6000ColorModes[j].bpp / 8) > fbMaxSize)
          {
              /* This mode is not valid, skip it. */
              de->availModesNum--;
          }
          else {
              /* This mode is valid. */
              et6000Modes[l].Length = sizeof(VIDEO_MODE_INFORMATION);
              et6000Modes[l].ModeIndex = l;
              et6000Modes[l].VisScreenWidth = et6000Res[i].width;
              et6000Modes[l].VisScreenHeight = et6000Res[i].height;
              et6000Modes[l].ScreenStride =
                  et6000Res[i].width * et6000ColorModes[j].bpp / 8;
              et6000Modes[l].NumberOfPlanes = 1;
              et6000Modes[l].BitsPerPlane = et6000ColorModes[j].bpp;
              et6000Modes[l].Frequency = et6000RefreshRates[k];
              et6000Modes[l].XMillimeter = 320;
              et6000Modes[l].YMillimeter = 240;
              et6000Modes[l].NumberRedBits = et6000ColorModes[j].redBitsNum;
              et6000Modes[l].NumberGreenBits = et6000ColorModes[j].greenBitsNum;
              et6000Modes[l].NumberBlueBits = et6000ColorModes[j].blueBitsNum;
              et6000Modes[l].RedMask = et6000ColorModes[j].redMask;
              et6000Modes[l].GreenMask = et6000ColorModes[j].greenMask;
              et6000Modes[l].BlueMask = et6000ColorModes[j].blueMask;
              et6000Modes[l].AttributeFlags =
                  VIDEO_MODE_COLOR | VIDEO_MODE_GRAPHICS;
              et6000Modes[l].VideoMemoryBitmapWidth = et6000Res[i].width;
              et6000Modes[l].VideoMemoryBitmapHeight = et6000Res[i].height;
              et6000Modes[l].DriverSpecificAttributeFlags = 0;
              l++;
          }
        }
      }
    }

    return TRUE;
}
/*****************************************************************************/
struct {
    USHORT vendorID;
    USHORT deviceID;
    enum et6000ChipType chip;
    PWSTR adapterName;
    ULONG adapterNameLength;
} supportedPCIChips[] =
{
    {0x100c, 0x3208, et6000,
        L"Tseng Labs Inc ET6000/ET6100 graphics and multimedia engine",
        sizeof(L"Tseng Labs Inc ET6000/ET6100 graphics and multimedia engine")},
    {0x100c, 0x4702, et6300,
        L"Tseng Labs Inc ET6300 graphics and multimedia engine",
        sizeof(L"Tseng Labs Inc ET6300 graphics and multimedia engine")}
};

#define ET6000_SUPPORTED_PCI_CHIPS_NUMBER \
    ( sizeof(supportedPCIChips) / sizeof(supportedPCIChips[0]) )

struct {
    PWSTR name;
    ULONG lenght;
} et6000ChipNames[] =
{ { L"none", 8 }, { L"ET6000", 12 }, { L"ET6100", 12 }, { L"ET6300", 12 }};
/*****************************************************************************/
VP_STATUS et6000FindAdapter(
    struct ET6000DeviceExtension *de,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ci,
    PUCHAR Again)
{
    PCI_COMMON_CONFIG pciData;
    ULONG i, nameIndex;

    if (ci->Length < sizeof(VIDEO_PORT_CONFIG_INFO)) {
        return ERROR_INVALID_PARAMETER;
    }

    /* We support only PCI devices */
    if (ci->AdapterInterfaceType != PCIBus)
        return ERROR_DEV_NOT_EXIST;

    VideoPortGetBusData(de, PCIConfiguration, 0, &pciData, 0,
                                      PCI_COMMON_HDR_LENGTH);

    /* Find out whether this adapter is what we support */
    de->chip = none;
    for (i = 0; i < ET6000_SUPPORTED_PCI_CHIPS_NUMBER; i++) {
        if ((supportedPCIChips[i].vendorID == pciData.VendorID) &&
            (supportedPCIChips[i].deviceID == pciData.DeviceID))
        {
            /* We support this adapter */
            de->chip = supportedPCIChips[i].chip;

            /* ET6100 has a RevisionID >= 0x70 */
            if ((de->chip == et6000) && (pciData.RevisionID >= 0x70))
                de->chip = et6100;

            nameIndex = i;
        }
    }
    if (de->chip == none)
        return ERROR_DEV_NOT_EXIST; /* We don't support this adapter */

    pciData.Command |= PCI_ENABLE_MEMORY_SPACE | PCI_ENABLE_IO_SPACE;
    pciData.u.type0.ROMBaseAddress |= 0x0000001;


    if (VideoPortSetBusData(de, PCIConfiguration, 0, &pciData,
                              0, PCI_COMMON_HDR_LENGTH) == 0)
        return ERROR_DEV_NOT_EXIST;

    VideoPortGetBusData(de, PCIConfiguration, 0, &pciData, 0,
                                      PCI_COMMON_HDR_LENGTH);

    /* Get the physical memory areas the adapter uses */
    VideoPortZeroMemory(&de->accessRanges, sizeof(de->accessRanges));

    if (VideoPortGetAccessRanges(de, 0, NULL, PCI_TYPE0_ADDRESSES,
        de->accessRanges, NULL, NULL, NULL) != NO_ERROR)
            return ERROR_DEV_NOT_EXIST;

    /*
     * Store the PCI header I/O space address.
     */
    de->physPCIConfigSpace = de->accessRanges[1].RangeStart;
    de->pciConfigSpace = (USHORT) de->physPCIConfigSpace.LowPart;

    /* Map the video memory physical addresses into kernel virtual space */
    de->physMemory = de->accessRanges[0].RangeStart;
    de->memory = VideoPortGetDeviceBase(de, de->physMemory,
        de->accessRanges[0].RangeLength,
        VIDEO_MEMORY_SPACE_MEMORY /* | VIDEO_MEMORY_SPACE_P6CACHE; */ );
    if (de->memory == NULL)
        return ERROR_INVALID_PARAMETER;

    /* Framebuffer */
    de->physFramebuffer = de->physMemory;
    de->framebuffer = de->memory;

    /* Memory mapped registers */
    de->physMMRegs.LowPart = de->physMemory.LowPart + 0x3fff00;
    de->physMMRegs.HighPart = 0;
    de->mmRegs = (PVOID)((ULONG)de->memory + 0x003fff00);

    /* External mapped registers */
    de->physEMRegs.LowPart = de->physMemory.LowPart + 0x3fe000;
    de->physEMRegs.HighPart = 0;
    de->emRegs = (PVOID)((ULONG)de->memory + 0x003fe000);

    /* Fill the ci structure */
    ci->BusInterruptLevel = 0;
    ci->BusInterruptVector = 0;
///    ci->InterruptMode = LevelSensitive; /* or Latched?! */
    ci->NumEmulatorAccessEntries = 0;
    ci->EmulatorAccessEntries = NULL;
    ci->EmulatorAccessEntriesContext = 0;
    ci->VdmPhysicalVideoMemoryAddress.LowPart = 0x000A0000;
    ci->VdmPhysicalVideoMemoryAddress.HighPart = 0x00000000;
    ci->VdmPhysicalVideoMemoryLength = 0x00020000;
    ci->HardwareStateSize = 0;
    ci->InterruptShareable = 1; /* Can share interrupts */

    /*
     * Save the information to the registry so it can be used by
     * configuration programs - such as the display applet.
     */
    VideoPortSetRegistryParameters(de, L"HardwareInformation.ChipType",
        et6000ChipNames[de->chip].name, et6000ChipNames[de->chip].lenght);

    VideoPortSetRegistryParameters(de, L"HardwareInformation.AdapterString",
                                   supportedPCIChips[nameIndex].adapterName,
                            supportedPCIChips[nameIndex].adapterNameLength);

    return NO_ERROR;
}
/*****************************************************************************/
BOOLEAN et6000Initialize(struct ET6000DeviceExtension *de)
{
    de->memSize = et6000GetOnboardMemorySize(de);

    VideoPortSetRegistryParameters(de, L"HardwareInformation.MemorySize",
                                            &de->memSize, sizeof(ULONG));

    /*
     * Clear any pending interrupts and disable interrupts. Driver
     * currently does not use interrupts and unlikely will in future.
     */
    et6000aclReadInterruptClear(de->mmRegs);
    et6000aclWriteInterruptClear(de->mmRegs);
    et6000aclMasterInterruptDisable(de->mmRegs);

    if (!et6000ConstructModes(de))
        return FALSE;

    return TRUE;
}
/*****************************************************************************/
BOOLEAN et6000Interrupt(struct ET6000DeviceExtension *de)
{
    switch (et6000aclInterruptCause(de->mmRegs)) {
    case ET6000_ACL_INT_CAUSE_NONE:
        return FALSE;
    case ET6000_ACL_INT_CAUSE_READ:
        et6000aclReadInterruptClear(de->mmRegs);
        break;
    case ET6000_ACL_INT_CAUSE_WRITE:
        et6000aclWriteInterruptClear(de->mmRegs);
        break;
    case ET6000_ACL_INT_CAUSE_BOTH: /* Can it be at all? */
        et6000aclReadInterruptClear(de->mmRegs);
        et6000aclWriteInterruptClear(de->mmRegs);
        break;
    }

    return TRUE;
}
/*****************************************************************************/
BOOLEAN et6000StartIO(
    struct ET6000DeviceExtension *de,
    PVIDEO_REQUEST_PACKET rp)
{
    VP_STATUS status;

    switch (rp->IoControlCode)
    {
    case IOCTL_VIDEO_MAP_VIDEO_MEMORY: {
        ULONG inIOSpace;
        ULONG bytesMapped;
#define ibuf ((PVIDEO_MEMORY)rp->InputBuffer)
#define obuf ((PVIDEO_MEMORY_INFORMATION)rp->OutputBuffer)

        rp->StatusBlock->Information = sizeof(VIDEO_MEMORY_INFORMATION);

        if ((rp->OutputBufferLength < sizeof(VIDEO_MEMORY_INFORMATION))
            || (rp->InputBufferLength < sizeof(VIDEO_MEMORY)))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        obuf->VideoRamLength = de->memSize;
        inIOSpace = VIDEO_MEMORY_SPACE_MEMORY /*|VIDEO_MEMORY_SPACE_P6CACHE;*/;
        obuf->VideoRamBase = ibuf->RequestedVirtualAddress;

        /* Always map 4Mb - from onboard RAM to memory mapped registers */
        bytesMapped = 4*1024*1024;
        status = VideoPortMapMemory(de, de->physMemory,
             &bytesMapped, &inIOSpace, &(obuf->VideoRamBase));

        if (status == NO_ERROR) {
            obuf->FrameBufferBase = obuf->VideoRamBase;
            obuf->FrameBufferLength = obuf->VideoRamLength;
        }
        else {
            rp->StatusBlock->Information = 0;
        }

        break;
#undef ibuf
#undef obuf
    }

    case IOCTL_VIDEO_UNMAP_VIDEO_MEMORY: {
        if (rp->InputBufferLength < sizeof(VIDEO_MEMORY)) {
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        status = VideoPortUnmapMemory(de,
            ((PVIDEO_MEMORY)(rp->InputBuffer))->RequestedVirtualAddress, 0);

        break;
    }

    case IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES: {
#define obuf ((PVIDEO_NUM_MODES)rp->OutputBuffer)

        rp->StatusBlock->Information = sizeof(VIDEO_NUM_MODES);

        if (rp->OutputBufferLength < sizeof(VIDEO_NUM_MODES)) {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
            obuf->NumModes = de->availModesNum;
            obuf->ModeInformationLength = sizeof(VIDEO_MODE_INFORMATION);
            status = NO_ERROR;
        }

        break;
#undef obuf
    }

    case IOCTL_VIDEO_QUERY_AVAIL_MODES: {
        if (rp->OutputBufferLength <
            de->availModesNum * sizeof(VIDEO_MODE_INFORMATION))
        {
            status = ERROR_INSUFFICIENT_BUFFER;
            rp->StatusBlock->Information = 0;
        }
        else {
            PVIDEO_MODE_INFORMATION modes = (PVIDEO_MODE_INFORMATION)rp->OutputBuffer;
            ULONG i;

            for (i = 0; i < de->availModesNum; i++) {
                *modes = et6000Modes[i];
                modes++;
            }
            status = NO_ERROR;
            rp->StatusBlock->Information =
                de->availModesNum * sizeof(VIDEO_MODE_INFORMATION);
        }
        break;
    }

    case IOCTL_VIDEO_QUERY_CURRENT_MODE: {
        if (rp->OutputBufferLength < sizeof(VIDEO_MODE_INFORMATION)) {
            rp->StatusBlock->Information = 0;
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
            *((PVIDEO_MODE_INFORMATION)rp->OutputBuffer)
                = et6000Modes[de->curMode];
            rp->StatusBlock->Information = sizeof(VIDEO_MODE_INFORMATION);
            status = NO_ERROR;
        }
        break;
    }

    case IOCTL_VIDEO_SET_CURRENT_MODE: {
        if (rp->InputBufferLength < sizeof(VIDEO_MODE)) {
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
            status = et6000SetMode(de,
                ((PVIDEO_MODE)rp->InputBuffer)->RequestedMode);

            if (status == NO_ERROR)
                de->curMode = ((PVIDEO_MODE)rp->InputBuffer)->RequestedMode
                    & ~(VIDEO_MODE_NO_ZERO_MEMORY|VIDEO_MODE_MAP_MEM_LINEAR);
        }
        break;
    }

    case IOCTL_VIDEO_RESET_DEVICE: {
        VIDEO_X86_BIOS_ARGUMENTS ba;

        /* Do an Int10 to mode 3 will put the VGA to a known state. */
        VideoPortZeroMemory(&ba, sizeof(VIDEO_X86_BIOS_ARGUMENTS));
        ba.Eax = 0x00000003; /* AL=0x03 */
        status = VideoPortInt10(de, &ba);
        break;
    }

    case IOCTL_VIDEO_GET_CHILD_STATE: {
#if WINXP
        *((ULONG *)rp->OutputBuffer) = VIDEO_CHILD_ACTIVE;
#else
        *((ULONG *)rp->OutputBuffer) = TRUE;
#endif
        rp->StatusBlock->Information = sizeof(ULONG);
        status = NO_ERROR;
        break;
    }

    case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES: {
#define obuf ((VIDEO_PUBLIC_ACCESS_RANGES *)rp->OutputBuffer)

        if (rp->OutputBufferLength < sizeof(VIDEO_PUBLIC_ACCESS_RANGES)) {
            rp->StatusBlock->Information = 0;
            status = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
            /* Return the PCI configuration space base I/O address */
            obuf->InIoSpace = VIDEO_MEMORY_SPACE_IO;
            obuf->MappedInIoSpace = 1;
            obuf->VirtualAddress = (PVOID)de->pciConfigSpace;
            rp->StatusBlock->Information = sizeof(VIDEO_PUBLIC_ACCESS_RANGES);
            status = NO_ERROR;        
        }

        break;
#undef obuf
    }

    case IOCTL_VIDEO_SHARE_VIDEO_MEMORY: {
        PVIDEO_SHARE_MEMORY shareMemory;
        PVIDEO_SHARE_MEMORY_INFORMATION shareMemoryInfo;
        PHYSICAL_ADDRESS shareAddress;
        PVOID virtualAddress;
        ULONG sharedViewSize;
        ULONG inIoSpace;

        if ((rp->OutputBufferLength < sizeof(VIDEO_SHARE_MEMORY_INFORMATION))
            || (rp->InputBufferLength < sizeof(VIDEO_MEMORY)) )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        shareMemory = rp->InputBuffer;

        if ((shareMemory->ViewOffset > de->memSize) ||
            ((shareMemory->ViewOffset + shareMemory->ViewSize) >
                  de->memSize))
        {
            status = ERROR_INVALID_PARAMETER;
            break;
        }

        rp->StatusBlock->Information = sizeof(VIDEO_SHARE_MEMORY_INFORMATION);

        /*
         * Beware: the input buffer and the output buffer are the same buffer,
         * and therefore data should not be copied from one to the other.
         */

        virtualAddress = shareMemory->ProcessHandle;
        sharedViewSize = shareMemory->ViewSize;

        /* We are ignoring ViewOffset */

        shareAddress.QuadPart = de->physMemory.QuadPart;

        /*
         * Enable USWC. We only do it for the frame buffer.
         * Memory mapped registers usually can not be mapped USWC.
         */
        inIoSpace = VIDEO_MEMORY_SPACE_MEMORY | VIDEO_MEMORY_SPACE_P6CACHE;
///        | VIDEO_MEMORY_SPACE_USER_MODE;///???

        status = VideoPortMapMemory(de, shareAddress, &sharedViewSize,
                                    &inIoSpace, &virtualAddress);

        shareMemoryInfo = rp->OutputBuffer;
        shareMemoryInfo->SharedViewOffset = shareMemory->ViewOffset;
        shareMemoryInfo->VirtualAddress = virtualAddress;
        shareMemoryInfo->SharedViewSize = sharedViewSize;
        break;
    }

    case IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY: {
        PVIDEO_SHARE_MEMORY sm;

        if (rp->InputBufferLength < sizeof(VIDEO_SHARE_MEMORY)) {
            status = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        sm = rp->InputBuffer;
        status = VideoPortUnmapMemory(de, sm->RequestedVirtualAddress,
                                      sm->ProcessHandle);
        break;
    }

    default:
        status = ERROR_INVALID_FUNCTION;
        rp->StatusBlock->Information = 0;
        break;
    }

    rp->StatusBlock->Status = status;
    return TRUE;
}
/*****************************************************************************/
BOOLEAN et6000ResetHW(
    struct ET6000DeviceExtension *de,
    ULONG Columns,
    ULONG Rows)
{
    /* Return false so the HAL does an INT10 mode 3 */
    return FALSE;
}
/*****************************************************************************/
VP_STATUS et6000SetPowerState(
    struct ET6000DeviceExtension *de,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    return NO_ERROR;
}
/*****************************************************************************/
VP_STATUS et6000GetPowerState(
    struct ET6000DeviceExtension *de,
    ULONG HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl)
{
    return ERROR_DEVICE_REINITIALIZATION_NEEDED;
}
/*****************************************************************************/
#if 0

/* Underimplemented */

VOID et6000I2CWriteClockLine(struct ET6000DeviceExtension *de, UCHAR data) {
    ioSet8(de->pciConfigSpace+0x4e, 0xfd, 0x02); /* tristate SCL pin */
    ioSet8(de->pciConfigSpace+0x4e, 0xdf, data<<5);
    ioSet8(de->pciConfigSpace+0x4e, 0xfd, 0x00); /* enable SCL pin */
}
VOID et6000I2CWriteDataLine(struct ET6000DeviceExtension *de, UCHAR data) {
    ioSet8(de->pciConfigSpace+0x4e, 0xfe, 0x01); /* tristate SDA pin */
    ioSet8(de->pciConfigSpace+0x4e, 0xef, data<<4);
    ioSet8(de->pciConfigSpace+0x4e, 0xfe, 0x00); /* enable SDA pin */
}
BOOLEAN et6000I2CReadClockLine(struct ET6000DeviceExtension *de) {
//    ioSet8(de->pciConfigSpace+0x4e, 0xfd, 0x00); /* enable SCL pin */
//    ioSet8(de->pciConfigSpace+0x4e, 0xfd, 0x02); /* tristate SCL pin */
    return (ioGet8(de->pciConfigSpace+0x4e) & 0xf7) >> 3;
}

BOOLEAN et6000I2CReadDataLine(struct ET6000DeviceExtension *de) {
//    ioSet8(de->pciConfigSpace+0x4e, 0xfe, 0x00); /* enable SDA pin */
//    ioSet8(de->pciConfigSpace+0x4e, 0xfe, 0x01); /* tristate SDA pin */
    return (ioGet8(de->pciConfigSpace+0x4e) & 0xfb) >> 2;
}

#if WINXP
DDC_CONTROL et6000DDCControl = {
    sizeof(DDC_CONTROL),
    {
        et6000I2CWriteClockLine,
        et6000I2CWriteDataLine,
        et6000I2CReadClockLine,
        et6000I2CReadDataLine,
    },
    0
};
#else /* WINXP */
/* Function that waits for a vertical sync to occur on the monitor */
VOID et6000I2CWaitVSyncActive(struct ET6000DeviceExtension *de) {
    /* If we are in VSync, wait for it to end */
    while ((ioGet8(0x3da) & 0x08) == 0x08);
    /* Wait for start of VSync */
    while ((ioGet8(0x3da) & 0x08) == 0x00);
}

I2C_FNC_TABLE et6000DDCControl = {
    sizeof(I2C_FNC_TABLE),
    et6000I2CWriteClockLine,
    et6000I2CWriteDataLine,
    et6000I2CReadClockLine,
    et6000I2CReadDataLine,
    et6000I2CWaitVSyncActive,
    0
};
#endif /* WINXP */

#endif /* 0 */
/*****************************************************************************/
VP_STATUS et6000GetVideoChildDescriptor(
    struct ET6000DeviceExtension *de,
    PVIDEO_CHILD_ENUM_INFO childEnumInfo,
    PVIDEO_CHILD_TYPE videoChildType,
    PUCHAR pChildDescriptor,
    PULONG pUId,
    PULONG pUnused)
{
    switch (childEnumInfo->ChildIndex) {
    case 0:
        /*
         * Case 0 is used to enumerate devices found by the ACPI firmware.
         * We do not currently support ACPI devices.
         */
        return ERROR_NO_MORE_DEVICES;

    case 1:
        /* Treat index 1 as the monitor */
        *videoChildType = Monitor;


        if(FALSE
            /*VideoPortDDCMonitorHelper(de, &et6000DDCControl,
                pChildDescriptor, childEnumInfo->ChildDescriptorSize)*/)
        {
            /* Found a DDC monitor, its EDID is in pChildDescriptor */
            *pUId = ET6000_DDC_MONITOR;
        }
        else {
            /* Assume monitor is non-DDC */
            *pUId = ET6000_NONDDC_MONITOR;
        }
        return ERROR_MORE_DATA;

    default:
        return ERROR_NO_MORE_DEVICES;
    }
}
/*****************************************************************************/
VIDEO_ACCESS_RANGE et6000LegacyResourceList[] =
{
    {{0x000C0000,0x00000000}, 0x00008000, 0, 1, 1, 0}, /* ROM location */
    {{0x000A0000,0x00000000}, 0x00020000, 0, 1, 1, 0}, /* Frame buffer */
    {{0x000003B0,0x00000000}, 0x0000000C, 1, 1, 1, 0}, /* VGA regs */
    {{0x000003C0,0x00000000}, 0x00000020, 1, 1, 1, 0}  /* VGA regs */
};

#define ET6000_LEGACY_RESOURCE_ENTRIES \
    ( sizeof(et6000LegacyResourceList) / sizeof(et6000LegacyResourceList[0]) )
/*****************************************************************************/
ULONG DriverEntry(
    PVOID Context1,
    PVOID Context2)
{
    VIDEO_HW_INITIALIZATION_DATA hwInitData;
    VP_STATUS initStatus;

    /* Zero out the structure. */
    VideoPortZeroMemory(&hwInitData, sizeof(VIDEO_HW_INITIALIZATION_DATA));

    /* Set entry points. */
    hwInitData.HwInitDataSize = sizeof(VIDEO_HW_INITIALIZATION_DATA);
    hwInitData.AdapterInterfaceType = 0; /* could be set to PCIBus */
    hwInitData.HwFindAdapter = et6000FindAdapter;
    hwInitData.HwInitialize = et6000Initialize;
    hwInitData.HwInterrupt = et6000Interrupt;
    hwInitData.HwStartIO = et6000StartIO;
    hwInitData.HwDeviceExtensionSize = sizeof(struct ET6000DeviceExtension);
    hwInitData.StartingDeviceNumber = 0;
    hwInitData.HwResetHw = et6000ResetHW;
    hwInitData.HwTimer = NULL;
    hwInitData.HwStartDma = NULL;
    hwInitData.HwSetPowerState = et6000SetPowerState;
    hwInitData.HwGetPowerState = et6000GetPowerState;
    hwInitData.HwGetVideoChildDescriptor = et6000GetVideoChildDescriptor;
    hwInitData.HwQueryInterface = NULL;
    hwInitData.HwChildDeviceExtensionSize = 0;
    hwInitData.HwLegacyResourceList = et6000LegacyResourceList;
    hwInitData.HwLegacyResourceCount = ET6000_LEGACY_RESOURCE_ENTRIES;
    hwInitData.HwGetLegacyResources = NULL;
    hwInitData.AllowEarlyEnumeration = FALSE;

    initStatus = VideoPortInitialize(Context1, Context2, &hwInitData, NULL);

#if WINXP
    if (initStatus!=NO_ERROR) {
        hwInitData.HwInitDataSize = SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA;
        initStatus = VideoPortInitialize(Context1,Context2,&hwInitData,NULL);
    }
#endif /* WINXP */

    return initStatus;
}
/*****************************************************************************/
