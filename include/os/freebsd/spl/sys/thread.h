#ifndef _SPL_THREAD_H_
#define	_SPL_THREAD_H_

#define	getcomm() curthread->td_name
#define	getpid() curthread->td_tid
#endif
