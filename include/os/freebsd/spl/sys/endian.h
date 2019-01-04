#ifndef _SPL_SYS_ENDIAN_H_
#define	_SPL_SYS_ENDIAN_H_

#undef _MACHINE_ENDIAN_H_
#include_next<sys/endian.h>

#if BYTE_ORDER == LITTLE_ENDIAN
#undef _BIG_ENDIAN
#undef BIG_ENDIAN
#define	BIG_ENDIAN 4321
#endif

#endif /*  _SPL_SYS_ENDIAN_H_ */
