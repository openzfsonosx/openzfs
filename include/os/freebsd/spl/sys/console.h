#ifndef	_SPL_CONSOLE_H
#define	_SPL_CONSOLE_H

static inline void
console_vprintf(const char *fmt, va_list args)
{
	vprintf(fmt, args);
}

static inline void
console_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	console_vprintf(fmt, args);
	va_end(args);
}

#endif /* _SPL_CONSOLE_H */
