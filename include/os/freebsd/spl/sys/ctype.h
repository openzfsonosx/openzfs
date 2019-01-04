#ifndef _SPL_SYS_CTYPE_H_
#define	_SPL_SYS_CTYPE_H_
#include_next <sys/ctype.h>

#ifdef _KERNEL
#define	isalnum(ch)	(isalpha(ch) || isdigit(ch))
#define	iscntrl(C)      (uchar(C) <= 0x1f || uchar(C) == 0x7f)
#define	isgraph(C)      ((C) >= 0x21 && (C) <= 0x7E)
#define	ispunct(C)      (((C) >= 0x21 && (C) <= 0x2F) || \
    ((C) >= 0x3A && (C) <= 0x40) || \
    ((C) >= 0x5B && (C) <= 0x60) || \
    ((C) >= 0x7B && (C) <= 0x7E))

#endif

#endif
