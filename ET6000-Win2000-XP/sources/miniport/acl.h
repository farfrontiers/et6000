/*****************************************************************************\
 * Tseng Labs ET6000, ET6100 and ET6300 video miniport driver
 * for Microsoft Windows 2000/XP.
 * Copyright (c) 2003-2005, Evgeniy Vladimirovich Bobkov.
\*****************************************************************************/
#ifndef _ET6000ACL_H_
#define _ET6000ACL_H_

#include "driver.h"
#include "bits.h"


/*****************************************************************************/
__inline void et6000aclMasterInterruptEnable(void *base) {
    set8(&((char *)base)[0x34], 0x7f, 0x80);
}

__inline void et6000aclMasterInterruptDisable(void *base) {
    set8(&((char *)base)[0x34], 0x7f, 0x00);
}

__inline void et6000aclReadInterruptEnable(void *base) {
    set8(&((char *)base)[0x34], 0xfd, 0x02);
}

__inline void et6000aclReadInterruptDisable(void *base) {
    set8(&((char *)base)[0x34], 0xfd, 0x00);
}

__inline void et6000aclWriteInterruptEnable(void *base) {
    set8(&((char *)base)[0x34], 0xfe, 0x01);
}

__inline void et6000aclWriteInterruptDisable(void *base) {
    set8(&((char *)base)[0x34], 0xfe, 0x00);
}

__inline void et6000aclReadInterruptClear(void *base) {
    set8(&((char *)base)[0x35], 0xfd, 0x02);
}

__inline void et6000aclWriteInterruptClear(void *base) {
    set8(&((char *)base)[0x34], 0xfe, 0x00);
}

#define ET6000_ACL_INT_CAUSE_NONE 0
#define ET6000_ACL_INT_CAUSE_READ 2
#define ET6000_ACL_INT_CAUSE_WRITE 1
#define ET6000_ACL_INT_CAUSE_BOTH 3

__inline char et6000aclInterruptCause(void *base) {
    return ((volatile char *)base)[0x35] & 0x03;
}
/*****************************************************************************/


#endif /* _ET6000ACL_H_ */
