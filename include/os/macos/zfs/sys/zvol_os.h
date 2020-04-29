/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
#ifndef _SYS_ZVOL_OS_h
#define _SYS_ZVOL_OS_h

extern int zvol_os_ioctl(dev_t, unsigned long, caddr_t,
    int isblk, cred_t *, int *rvalp);
extern int zvol_os_open(dev_t dev, int flag, int otyp, struct proc *p);
extern int zvol_os_close(dev_t dev, int flag, int otyp, struct proc *p);
extern int zvol_os_read(dev_t dev, struct uio *uiop, int p);
extern int zvol_os_write(dev_t dev, struct uio *uiop, int p);
extern void zvol_os_strategy(struct buf *bp);
extern int zvol_os_get_volume_blocksize(dev_t dev);

#endif
