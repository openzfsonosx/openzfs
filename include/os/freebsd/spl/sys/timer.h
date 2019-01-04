
#ifndef _SPL_TIMER_H_
#define	_SPL_TIMER_H_ 
#define	ddi_time_after(a, b) ((a) > (b))
#define	ddi_time_after64(a, b) ((a) > (b))
#define	 ZFS_DELAY_RESOLUTION_NS 100 * 1000 /* 100 microseconds */
#define	usleep_range(wakeup, wakeupepsilon)				   \
	pause_sbt("usleep_range", nstosbt(wakeup), \
			  nstosbt(ZFS_DELAY_RESOLUTION_NS), C_ABSOLUTE)

#define	schedule() pause("schedule", 1)
	

#endif	
