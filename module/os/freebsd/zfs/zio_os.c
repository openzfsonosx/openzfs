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
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 */

#include <sys/sysmacros.h>
#include <sys/zio.h>
#include <sys/zio_impl.h>
#include <sys/time.h>

void
zio_delay_interrupt(zio_t *zio)
{
	/*
	 * The timeout_generic() function isn't defined in userspace, so
	 * rather than trying to implement the function, the zio delay
	 * functionality has been disabled for userspace builds.
	 */

	/*
	 * If io_target_timestamp is zero, then no delay has been registered
	 * for this IO, thus jump to the end of this function and "skip" the
	 * delay; issuing it directly to the zio layer.
	 */
	if (zio->io_target_timestamp != 0) {
		hrtime_t now = gethrtime();

		if (now >= zio->io_target_timestamp) {
			/*
			 * This IO has already taken longer than the target
			 * delay to complete, so we don't want to delay it
			 * any longer; we "miss" the delay and issue it
			 * directly to the zio layer. This is likely due to
			 * the target latency being set to a value less than
			 * the underlying hardware can satisfy (e.g. delay
			 * set to 1ms, but the disks take 10ms to complete an
			 * IO request).
			 */

			DTRACE_PROBE2(zio__delay__miss, zio_t *, zio,
			    hrtime_t, now);

			zio_interrupt(zio);
		} else {
			hrtime_t diff = zio->io_target_timestamp - now;

			DTRACE_PROBE3(zio__delay__hit, zio_t *, zio,
			    hrtime_t, now, hrtime_t, diff);

			(void) timeout_generic(CALLOUT_NORMAL,
			    (void (*)(void *))zio_interrupt, zio, diff, 1, 0);
		}

		return;
	}

	DTRACE_PROBE1(zio__delay__skip, zio_t *, zio);
	zio_interrupt(zio);
}
