
#ifdef _KERNEL
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <machine/fpu.h>
#include <x86/x86_var.h>
#include <x86/specialreg.h>
#endif

#define	kfpu_init()		(0)
#define	kfpu_fini()		do {} while (0)
#ifdef _KERNEL
#define	kfpu_allowed()		1
#define	kfpu_initialize(tsk)	do {} while (0)

#define	kfpu_begin() {							\
	critical_enter();							\
	fpu_kern_enter(curthread, NULL, FPU_KERN_NOCTX); \
}

#define	kfpu_end()						\
{												   \
	fpu_kern_leave(curthread, NULL);				   \
	critical_exit();								   \
}
#else
#endif
/*
 * Check if OS supports AVX and AVX2 by checking XCR0
 * Only call this function if CPUID indicates that AVX feature is
 * supported by the CPU, otherwise it might be an illegal instruction.
 */
static inline uint64_t
xgetbv(uint32_t index)
{
	uint32_t eax, edx;
	/* xgetbv - instruction byte code */
	__asm__ __volatile__(".byte 0x0f; .byte 0x01; .byte 0xd0"
	    : "=a" (eax), "=d" (edx)
	    : "c" (index));

	return ((((uint64_t)edx)<<32) | (uint64_t)eax);
}


/*
 * Detect register set support
 */
static inline boolean_t
__simd_state_enabled(const uint64_t state)
{
	boolean_t has_osxsave;
	uint64_t xcr0;

#if defined(_KERNEL)

	has_osxsave = !!(cpu_feature2 & CPUID2_OSXSAVE);
#elif !defined(_KERNEL)
	has_osxsave = __cpuid_has_osxsave();
#endif

	if (!has_osxsave)
		return (B_FALSE);

	xcr0 = xgetbv(0);
	return ((xcr0 & state) == state);
}

#define	_XSTATE_SSE_AVX		(0x2 | 0x4)
#define	_XSTATE_AVX512		(0xE0 | _XSTATE_SSE_AVX)

#define	__ymm_enabled() __simd_state_enabled(_XSTATE_SSE_AVX)
#define	__zmm_enabled() __simd_state_enabled(_XSTATE_AVX512)


/*
 * Check if SSE instruction set is available
 */
static inline boolean_t
zfs_sse_available(void)
{
#if defined(_KERNEL)
	return !!(cpu_feature & CPUID_SSE);
#elif !defined(_KERNEL)
	return (__cpuid_has_sse());
#endif
}

/*
 * Check if SSE2 instruction set is available
 */
static inline boolean_t
zfs_sse2_available(void)
{
#if defined(_KERNEL)
	return !!(cpu_feature & CPUID_SSE2);
#elif !defined(_KERNEL)
	return (__cpuid_has_sse2());
#endif
}

/*
 * Check if SSE3 instruction set is available
 */
static inline boolean_t
zfs_sse3_available(void)
{
#if defined(_KERNEL)
	return !!(cpu_feature2 & CPUID2_SSE3);
#elif !defined(_KERNEL)
	return (__cpuid_has_sse3());
#endif
}

/*
 * Check if SSSE3 instruction set is available
 */
static inline boolean_t
zfs_ssse3_available(void)
{
#if defined(_KERNEL)
	return !!(cpu_feature2 & CPUID2_SSSE3);
#elif !defined(_KERNEL)
	return (__cpuid_has_ssse3());
#endif
}

/*
 * Check if SSE4.1 instruction set is available
 */
static inline boolean_t
zfs_sse4_1_available(void)
{
#if defined(_KERNEL)
#if defined(KERNEL_EXPORTS_X86_FPU)
	return (!!boot_cpu_has(X86_FEATURE_XMM4_1));
#else
	return (B_FALSE);
#endif
#elif !defined(_KERNEL)
	return (__cpuid_has_sse4_1());
#endif
}

/*
 * Check if SSE4.2 instruction set is available
 */
static inline boolean_t
zfs_sse4_2_available(void)
{
#if defined(_KERNEL)
#if defined(KERNEL_EXPORTS_X86_FPU)
	return (!!boot_cpu_has(X86_FEATURE_XMM4_2));
#else
	return (B_FALSE);
#endif
#elif !defined(_KERNEL)
	return (__cpuid_has_sse4_2());
#endif
}

/*
 * Check if AVX instruction set is available
 */
static inline boolean_t
zfs_avx_available(void)
{
	boolean_t has_avx;

#if defined(_KERNEL)
#ifdef __linux__
#if defined(KERNEL_EXPORTS_X86_FPU)
	has_avx = !!boot_cpu_has(X86_FEATURE_AVX);
#else
	has_avx = B_FALSE;
#endif
#elif defined(__FreeBSD__)
	has_avx = !!(cpu_feature2 & CPUID2_AVX);
#endif		
#elif !defined(_KERNEL)
	has_avx = __cpuid_has_avx();
#endif

	return (has_avx && __ymm_enabled());
}

/*
 * Check if AVX2 instruction set is available
 */
static inline boolean_t
zfs_avx2_available(void)
{
	boolean_t has_avx2;
#if defined(_KERNEL)
	has_avx2 = !!(cpu_stdext_feature & CPUID_STDEXT_AVX2);
#elif !defined(_KERNEL)
	has_avx2 = __cpuid_has_avx2();
#endif

	return (has_avx2 && __ymm_enabled());
}

/*
   * AVX-512 family of instruction sets:
 *
 * AVX512F	Foundation
 * AVX512CD	Conflict Detection Instructions
 * AVX512ER	Exponential and Reciprocal Instructions
 * AVX512PF	Prefetch Instructions
 *
 * AVX512BW	Byte and Word Instructions
 * AVX512DQ	Double-word and Quadword Instructions
 * AVX512VL	Vector Length Extensions
 *
 * AVX512IFMA	Integer Fused Multiply Add (Not supported by kernel 4.4)
 * AVX512VBMI	Vector Byte Manipulation Instructions
 */


/* Check if AVX512F instruction set is available */
static inline boolean_t
zfs_avx512f_available(void)
{
	boolean_t has_avx512 = B_FALSE;

#if defined(_KERNEL)
	has_avx512 = !!(cpu_stdext_feature & CPUID_STDEXT_AVX512F);
#elif !defined(_KERNEL)
	has_avx512 = __cpuid_has_avx512f();
#endif

	return (has_avx512 && __zmm_enabled());
}

/* Check if AVX512CD instruction set is available */
static inline boolean_t
zfs_avx512cd_available(void)
{
	boolean_t has_avx512 = B_FALSE;

#if defined(_KERNEL)
#if defined(X86_FEATURE_AVX512CD) && defined(KERNEL_EXPORTS_X86_FPU)
	has_avx512 = boot_cpu_has(X86_FEATURE_AVX512F) &&
	    boot_cpu_has(X86_FEATURE_AVX512CD);
#else
	has_avx512 = B_FALSE;
#endif
#elif !defined(_KERNEL)
	has_avx512 = __cpuid_has_avx512cd();
#endif

	return (has_avx512 && __zmm_enabled());
}

/* Check if AVX512ER instruction set is available */
static inline boolean_t
zfs_avx512er_available(void)
{
	boolean_t has_avx512 = B_FALSE;

#if defined(_KERNEL)
#if defined(X86_FEATURE_AVX512ER) && defined(KERNEL_EXPORTS_X86_FPU)
	has_avx512 = boot_cpu_has(X86_FEATURE_AVX512F) &&
	    boot_cpu_has(X86_FEATURE_AVX512ER);
#else
	has_avx512 = B_FALSE;
#endif
#elif !defined(_KERNEL)
	has_avx512 = __cpuid_has_avx512er();
#endif

	return (has_avx512 && __zmm_enabled());
}

/* Check if AVX512PF instruction set is available */
static inline boolean_t
zfs_avx512pf_available(void)
{
	boolean_t has_avx512 = B_FALSE;

#if defined(_KERNEL)
#if defined(X86_FEATURE_AVX512PF) && defined(KERNEL_EXPORTS_X86_FPU)
	has_avx512 = boot_cpu_has(X86_FEATURE_AVX512F) &&
	    boot_cpu_has(X86_FEATURE_AVX512PF);
#else
	has_avx512 = B_FALSE;
#endif
#elif !defined(_KERNEL)
	has_avx512 = __cpuid_has_avx512pf();
#endif

	return (has_avx512 && __zmm_enabled());
}

/* Check if AVX512BW instruction set is available */
static inline boolean_t
zfs_avx512bw_available(void)
{
	boolean_t has_avx512 = B_FALSE;

#if defined(_KERNEL)
	has_avx512 = !!(cpu_stdext_feature & CPUID_STDEXT_AVX512BW);
#elif !defined(_KERNEL)
	has_avx512 = __cpuid_has_avx512bw();
#endif

	return (has_avx512 && __zmm_enabled());
}

/* Check if AVX512DQ instruction set is available */
static inline boolean_t
zfs_avx512dq_available(void)
{
	boolean_t has_avx512 = B_FALSE;

#if defined(_KERNEL)
#if defined(X86_FEATURE_AVX512DQ) && defined(KERNEL_EXPORTS_X86_FPU)
	has_avx512 = boot_cpu_has(X86_FEATURE_AVX512F) &&
	    boot_cpu_has(X86_FEATURE_AVX512DQ);
#else
	has_avx512 = B_FALSE;
#endif
#elif !defined(_KERNEL)
	has_avx512 = __cpuid_has_avx512dq();
#endif

	return (has_avx512 && __zmm_enabled());
}

/* Check if AVX512VL instruction set is available */
static inline boolean_t
zfs_avx512vl_available(void)
{
	boolean_t has_avx512 = B_FALSE;

#if defined(_KERNEL)
#if defined(X86_FEATURE_AVX512VL) && defined(KERNEL_EXPORTS_X86_FPU)
	has_avx512 = boot_cpu_has(X86_FEATURE_AVX512F) &&
	    boot_cpu_has(X86_FEATURE_AVX512VL);
#else
	has_avx512 = B_FALSE;
#endif
#elif !defined(_KERNEL)
	has_avx512 = __cpuid_has_avx512vl();
#endif

	return (has_avx512 && __zmm_enabled());
}

/* Check if AVX512IFMA instruction set is available */
static inline boolean_t
zfs_avx512ifma_available(void)
{
	boolean_t has_avx512 = B_FALSE;

#if defined(_KERNEL)
#if defined(X86_FEATURE_AVX512IFMA) && defined(KERNEL_EXPORTS_X86_FPU)
	has_avx512 = boot_cpu_has(X86_FEATURE_AVX512F) &&
	    boot_cpu_has(X86_FEATURE_AVX512IFMA);
#else
	has_avx512 = B_FALSE;
#endif
#elif !defined(_KERNEL)
	has_avx512 = __cpuid_has_avx512ifma();
#endif

	return (has_avx512 && __zmm_enabled());
}

/* Check if AVX512VBMI instruction set is available */
static inline boolean_t
zfs_avx512vbmi_available(void)
{
	boolean_t has_avx512 = B_FALSE;

#if defined(_KERNEL)
#if defined(X86_FEATURE_AVX512VBMI) && defined(KERNEL_EXPORTS_X86_FPU)
	has_avx512 = boot_cpu_has(X86_FEATURE_AVX512F) &&
	    boot_cpu_has(X86_FEATURE_AVX512VBMI);
#else
	has_avx512 = B_FALSE;
#endif
#elif !defined(_KERNEL)
	has_avx512 = __cpuid_has_avx512f() &&
	    __cpuid_has_avx512vbmi();
#endif

	return (has_avx512 && __zmm_enabled());
}
