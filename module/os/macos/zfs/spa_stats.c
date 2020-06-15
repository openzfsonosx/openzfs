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

#include <sys/zfs_context.h>
#include <sys/spa_impl.h>
#include <sys/vdev_impl.h>

void
spa_stats_init(spa_t *spa)
{

}

void
spa_stats_destroy(spa_t *spa)
{

}

void
spa_iostats_trim_add(spa_t *spa, trim_type_t type,
    uint64_t extents_written, uint64_t bytes_written,
    uint64_t extents_skipped, uint64_t bytes_skipped,
    uint64_t extents_failed, uint64_t bytes_failed)
{
}

void
spa_read_history_add(spa_t *spa, const zbookmark_phys_t *zb, uint32_t aflags)
{
}

void
spa_txg_history_add(spa_t *spa, uint64_t txg, hrtime_t birth_time)
{

}
/*
 * Set txg state completion time and increment current state.
 */
int
spa_txg_history_set(spa_t *spa, uint64_t txg, txg_state_t completed_state,
    hrtime_t completed_time)
{
	return (0);
}

txg_stat_t *
spa_txg_history_init_io(spa_t *spa, uint64_t txg, dsl_pool_t *dp)
{
	return (NULL);
}

void
spa_txg_history_fini_io(spa_t *spa, txg_stat_t *ts)
{

}

void
spa_tx_assign_add_nsecs(spa_t *spa, uint64_t nsecs)
{

}

void
spa_mmp_history_add(spa_t *spa, uint64_t txg, uint64_t timestamp,
    uint64_t mmp_delay, vdev_t *vd, int label, uint64_t mmp_node_id,
    int error)
{

}

int
spa_mmp_history_set(spa_t *spa, uint64_t mmp_node_id, int io_error,
    hrtime_t duration)
{
	return (0);
}

int
spa_mmp_history_set_skip(spa_t *spa, uint64_t mmp_node_id)
{
	return (0);
}
