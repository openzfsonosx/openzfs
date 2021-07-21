/*
 * test POSIX style timers:
 *
 * clang -o timer_test timer_test.c
 *      -I lib/libspl/include/os/macos/ -I lib/libspl/include
 */

#include <sys/stdtypes.h>
#include <stdbool.h>
#include <mach/boolean.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

void
_timer_notify(union sigval sv)
{
	printf("timer called: %p\n", sv.sival_ptr);
}


int
main(int argc, char **argv)
{

	struct sigevent sev;
	struct itimerspec its;
	hrtime_t delta;
	timer_t tid;

	printf("setup\n");

	delta = SEC2NSEC(2);

	its.it_value.tv_sec = delta / 1000000000;
	its.it_value.tv_nsec = delta % 1000000000;
	its.it_interval.tv_sec = its.it_value.tv_sec;
	its.it_interval.tv_nsec = its.it_value.tv_nsec;

	sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = _timer_notify;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value.sival_ptr = (void *)0x1234567801234567;

	timer_create(CLOCK_REALTIME, &sev, &tid);
	timer_settime(tid, 0, &its, NULL);

	printf("sleeping\n");
	sleep(3);
	printf("suspend\n");

	struct itimerspec its2 = { 0 };
	timer_settime(tid, 0, &its2, NULL);

	sleep(3);

	printf("resuming\n");
	timer_settime(tid, 0, &its, NULL);

	sleep(3);
	timer_delete(tid);

	printf("quit\n");
}
