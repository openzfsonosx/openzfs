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
 *
 * Copyright (C) 2013, 2020 Jorgen Lundman <lundman@lundman.net>
 *
 */

#define	KERNEL_MAP_NO_LOAD

#include <sys/debug.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <mach/mach_types.h>
#include <libkern/libkern.h>
#include <sys/mutex.h>
#include <sys/rwlock.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/taskq.h>
#include <kern/processor.h>
#include <sys/types.h>
#include <sys/sysctl.h>

#define	_task_user_
#include <IOKit/IOLib.h>

#include <zfs_config.h>
#include <sys/systeminfo.h>

extern int system_inshutdown;

static utsname_t utsname_static = { { 0 } };

unsigned int max_ncpus = 0;
uint64_t  total_memory = 0;
uint64_t  real_total_memory = 0;

// Size in bytes of the memory allocated in seg_kmem
extern uint64_t		segkmem_total_mem_allocated;

/* List of pointers to be filled in, to the real locations */
volatile unsigned int *REAL_vm_page_free_wanted = NULL;
volatile unsigned int *REAL_vm_page_free_count = NULL;
volatile unsigned int *REAL_vm_page_speculative_count = NULL;
volatile unsigned int *REAL_vm_page_free_min = NULL;
int *REAL_system_inshutdown = NULL;
char *REAL_hostname = NULL;
#ifdef DEBUG
OSKextLoadedKextSummaryHeader *REAL_gLoadedKextSummaries = NULL;
kernel_mach_header_t *REAL__mh_execute_header = NULL;
#endif

struct fileproc;

/* Functions */
struct vnode *(*REAL_rootvnode) = NULL;
vfs_context_t (*REAL_vfs_context_kernel)(void) = NULL;
int (*REAL_vnode_iocount)(struct vnode *vp) = NULL;
void (*REAL_cache_purgevfs)(mount_t mp) = NULL;
int (*REAL_VFS_ROOT)(mount_t mp, struct vnode **vpp, vfs_context_t ctx) = NULL;
errno_t (*REAL_VNOP_LOOKUP)(struct vnode *dvp, struct vnode **vpp,
		struct componentname *cnp, vfs_context_t ctx) = NULL;
int (*REAL_build_path)(struct vnode *vp, char *buff, int buflen, int *outlen,
		int flags, vfs_context_t ctx) = NULL;
addr64_t  (*REAL_kvtophys)(vm_offset_t va) = NULL;
kern_return_t (*REAL_kernel_memory_allocate)(vm_map_t map, void **addrp,
		vm_size_t size, vm_offset_t mask, int flags, int tag) = NULL;
void (*REAL_kx_qsort)(void *array, size_t nm, size_t member_size,
		int (*cmpf)(const void *, const void *)) = NULL;
int (*REAL_kauth_cred_getgroups)(kauth_cred_t _cred, gid_t *_groups, int *_groupcount) = NULL;

struct i386_cpu_info;
typedef struct i386_cpu_info i386_cpu_info_t;
i386_cpu_info_t *(*REAL_cpuid_info)(void) = NULL;

#ifdef __arm64__
int (*REAL_setjmp)(void *e) = NULL;
void (*longjmp)(void *e, int val) = NULL;
#endif

#if 0
int (*REAL_fp_drop)(struct proc *p, int fd, struct fileproc *fp, int locked) = NULL;
int (*REAL_fp_drop_written)(struct proc *p, int fd, struct fileproc *fp, int locked) = NULL;
int (*REAL_fp_lookup)(struct proc *p, int fd, struct fileproc **resultfp, int locked) = NULL;
int (*REAL_fo_read)(struct fileproc *fp, struct uio *uio, int flags, vfs_context_t ctx) = NULL;
int (*REAL_fo_write)(struct fileproc *fp, struct uio *uio, int flags, vfs_context_t ctx) = NULL;
#endif

int
(*REAL_fp_getfvp)(proc_t p, int fd, struct fileproc **resultfp,
    struct vnode **resultvp) = NULL;

utsname_t *
utsname(void)
{
	return (&utsname_static);
}

/*
 * Solaris delay is in ticks (hz) and Darwin uses microsecs
 * 1 HZ is 10 milliseconds
 */
void
osx_delay(int ticks)
{
	if (ticks < 2) {
		// IODelay spins and takes microseconds as an argument
		// don't spend more than 10msec spinning.
		IODelay(ticks * 10000);
		return;
	}

	// ticks are 10 msec units
	int64_t ticks_to_go = (int64_t) ticks * 10LL;
	// zfs_lbolt() is in 10 mec units
	int64_t start_tick = (int64_t) zfs_lbolt();
	int64_t end_tick = start_tick + (int64_t) ticks_to_go;

	do {
		IOSleep(ticks_to_go);
		int64_t cur_tick = (int64_t) zfs_lbolt();
		ticks_to_go = (end_tick - cur_tick);
	} while (ticks_to_go > 0);

}


uint32_t
zone_get_hostid(void *zone)
{
	size_t len;
	uint32_t myhostid = 0;

	len = sizeof (myhostid);
	sysctlbyname("kern.hostid", &myhostid, &len, NULL, 0);
	return (myhostid);
}

extern void *(*__ihook_malloc)(size_t size);
extern void (*__ihook_free)(void *);

int
ddi_copyin(const void *from, void *to, size_t len, int flags)
{
	int ret = 0;

	/* Fake ioctl() issued by kernel, 'from' is a kernel address */
	if (flags & FKIOCTL)
		bcopy(from, to, len);
	else
		ret = copyin((user_addr_t)from, (void *)to, len);

	return (ret);
}

int
ddi_copyout(const void *from, void *to, size_t len, int flags)
{
	int ret = 0;

	/* Fake ioctl() issued by kernel, 'from' is a kernel address */
	if (flags & FKIOCTL) {
		bcopy(from, to, len);
	} else {
		ret = copyout(from, (user_addr_t)to, len);
	}

	return (ret);
}

/*
 * Technically, this call does not exist in illumos, but we use it for
 * consistency.
 */
int
ddi_copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
{
	int ret;
	size_t local_done;

#undef copyinstr
	ret = copyinstr((user_addr_t)uaddr, kaddr, len, &local_done);
	if (done != NULL)
		*done = local_done;
	return (ret);
}

kern_return_t
spl_start(kmod_info_t *ki, void *d)
{
	printf("SPL: loading\n");

	int ncpus;
	size_t len = sizeof (ncpus);

	/*
	 * Boot load time is excessively early, so we have to wait
	 * until certain subsystems are available. Surely there is
	 * a more elegant way to do this wait?
	 */

	while (current_proc() == NULL) {
		printf("SPL: waiting for kernel init...\n");
		delay(hz>>1);
	}

	while (1) {
		len = sizeof (total_memory);
		sysctlbyname("hw.memsize", &total_memory, &len, NULL, 0);
		if (total_memory != 0) break;

		printf("SPL: waiting for sysctl...\n");
		delay(hz>>1);
	}

	/*
	 * We need to map a handful of functions and variables for ZFS to work.
	 * If any fail, we need to fail to load.
	 *
	 * It would be desirable to one day remove all of these.
	 */
	extern int spl_loadsymbols(void);
	int error;
	error = spl_loadsymbols();
	if (error != 0)
		return (error);

	sysctlbyname("hw.logicalcpu_max", &max_ncpus, &len, NULL, 0);
	if (!max_ncpus) max_ncpus = 1;

	/*
	 * Setting the total memory to physmem * 50% here, since kmem is
	 * not in charge of all memory and we need to leave some room for
	 * the OS X allocator. We internally add pressure if we step over it
	 */
	real_total_memory = total_memory;
	total_memory = total_memory * 50ULL / 100ULL;
	physmem = total_memory / PAGE_SIZE;

	len = sizeof (utsname_static.sysname);
	sysctlbyname("kern.ostype", &utsname_static.sysname, &len, NULL, 0);

	/*
	 * For some reason, (CTLFLAG_KERN is not set) looking up hostname
	 * returns 1. So we set it to uuid just to give it *something*.
	 * As it happens, ZFS sets the nodename on init.
	 */
	len = sizeof (utsname_static.nodename);
	sysctlbyname("kern.uuid", &utsname_static.nodename, &len, NULL, 0);

	len = sizeof (utsname_static.release);
	sysctlbyname("kern.osrelease", &utsname_static.release, &len, NULL, 0);

	len = sizeof (utsname_static.version);
	sysctlbyname("kern.version", &utsname_static.version, &len, NULL, 0);

	strlcpy(utsname_static.nodename, REAL_hostname,
	    sizeof (utsname_static.nodename));

	spl_mutex_subsystem_init();
	spl_kmem_init(total_memory);
	spl_vnode_init();
	spl_kmem_thread_init();
	spl_kmem_mp_init();

	return (KERN_SUCCESS);
}

kern_return_t
spl_stop(kmod_info_t *ki, void *d)
{
	spl_kmem_thread_fini();
	spl_vnode_fini();
	spl_taskq_fini();
	spl_rwlock_fini();
	spl_tsd_fini();
	spl_kmem_fini();
	spl_kstat_fini();
	spl_mutex_subsystem_fini();

	return (KERN_SUCCESS);
}



#include <mach/mach_types.h>
#include <mach-o/loader.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <mach-o/nlist.h>

#define DLOG(args...)   printf(args)

/* Exported: xnu/osfmk/mach/i386/vm_param.h */
#ifndef VM_MIN_KERNEL_ADDRESS
#define VM_MIN_KERNEL_ADDRESS           ((vm_offset_t) 0xffffff8000000000ULL)
#endif
#ifndef VM_MIN_KERNEL_AND_KEXT_ADDRESS
#define VM_MIN_KERNEL_AND_KEXT_ADDRESS  (VM_MIN_KERNEL_ADDRESS - 0x80000000ULL)
#endif

#define KERN_HIB_BASE                   ((vm_offset_t) 0xffffff8000100000ULL)
#define KERN_TEXT_BASE                  ((vm_offset_t) 0xffffff8000200000ULL)

typedef struct mach_header_64 mach_header_t;
typedef struct segment_command_64 segment_command_t;
typedef struct section_64 section_t;
typedef struct nlist_64 nlist_t;

#define LC_SGMT LC_SEGMENT_64
#define MH_MAGIC_ MH_MAGIC_64
#define load_cmd struct load_command

load_cmd *
macho_find_load_command(mach_header_t* header, uint32_t cmd)
{
	load_cmd *lc;
	vm_address_t cur = (vm_address_t)header + sizeof(mach_header_t);
	for (uint i = 0; i < header->ncmds; i++,cur += lc->cmdsize) {
		lc = (load_cmd*)cur;
		if(lc->cmd == cmd){
			return lc;
		}
	}
	return NULL;
}

segment_command_t* macho_find_segment(mach_header_t* header,const char *segname){
	segment_command_t *cur_seg_cmd;
	vm_address_t cur = (vm_address_t)header + sizeof(mach_header_t);
	for (uint i = 0; i < header->ncmds; i++,cur += cur_seg_cmd->cmdsize) {
		cur_seg_cmd = (segment_command_t*)cur;
		if(cur_seg_cmd->cmd == LC_SGMT){
			if(!strcmp(segname,cur_seg_cmd->segname)){
				return cur_seg_cmd;
			}
		}
	}
	return NULL;
}

section_t* macho_find_section(mach_header_t* header, const char *segname, const char *secname){
	segment_command_t *cur_seg_cmd;
	vm_address_t cur = (vm_address_t)header + sizeof(mach_header_t);
	for (uint i = 0; i < header->ncmds; i++,cur += cur_seg_cmd->cmdsize) {
		cur_seg_cmd = (segment_command_t*)cur;
		if(cur_seg_cmd->cmd == LC_SGMT){
			if(!strcmp(segname,cur_seg_cmd->segname)){
				for (uint j = 0; j < cur_seg_cmd->nsects; j++) {
					section_t *sect = (section_t *)(cur + sizeof(segment_command_t)) + j;
					if(!strcmp(secname, sect->sectname)){
						return sect;
					}
				}
			}
		}
	}
	return NULL;
}


void* macho_find_symbol(mach_header_t* header, const char *name)
{
	segment_command_t* first = (segment_command_t*) macho_find_load_command(header, LC_SGMT);
	segment_command_t* linkedit_segment = macho_find_segment(header, SEG_LINKEDIT);
	struct symtab_command* symtab_cmd = (struct symtab_command*)macho_find_load_command(header, LC_SYMTAB);

	vm_address_t vmaddr_slide = (vm_address_t)header - (vm_address_t)first->vmaddr;

	uintptr_t linkedit_base = (uintptr_t)vmaddr_slide + linkedit_segment->vmaddr - linkedit_segment->fileoff;
	nlist_t *symtab = (nlist_t *)(linkedit_base + symtab_cmd->symoff);
	char *strtab = (char *)(linkedit_base + symtab_cmd->stroff);

	for (int i = 0; i < symtab_cmd->nsyms; i++) {
		if (symtab[i].n_value && !strcmp(name,&strtab[symtab[i].n_un.n_strx])) {
			return (void*) (uint64_t) (symtab[i].n_value + vmaddr_slide);
		}
	}

	return NULL;
}

#define LOAD_SYMBOL(data, str, found, failed) \
	do { \
		(data) = macho_find_symbol(mh, (str));	\
		if ((data) == NULL) { \
			printf("%s: failed to locate '%s'\n", __func__, (str)); \
			(failed)++;	\
		} else \
			(found)++;	\
	} while (0)

int spl_loadsymbols(void)
{
    vm_offset_t vm_kern_slide;
    vm_address_t hib_base;
    vm_address_t kern_base;
    struct mach_header_64 *mh;
	int symbol_failed = 0;
	int symbol_found = 0;
#ifdef DEBUG
	int symbol_debug = 0;
#endif

	vm_offset_t func_address = (vm_offset_t) vm_kernel_unslide_or_perm_external;
	vm_offset_t func_address_unslid = 0;
	vm_kernel_unslide_or_perm_external(func_address, &func_address_unslid);
    vm_kern_slide = func_address - func_address_unslid;

	delay(hz);
	printf("Kernel slide: %lx\n", vm_kern_slide);

    hib_base = KERN_HIB_BASE + vm_kern_slide;
    kern_base = KERN_TEXT_BASE + vm_kern_slide /* + 0xc000 */;

	mh = (struct mach_header_64 *) kern_base;

	printf("Kernel base: %llx\n", (uint64_t)mh);
	LOAD_SYMBOL(REAL_vm_page_speculative_count, "_vm_page_speculative_count",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_vnode_iocount, "_vnode_iocount",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_cache_purgevfs, "_cache_purgevfs",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_kx_qsort, "_kx_qsort",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_vm_page_free_wanted, "_vm_page_free_wanted",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_vm_page_free_count, "_vm_page_free_count",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_vm_page_free_min, "_vm_page_free_min",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_rootvnode, "_rootvnode",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_system_inshutdown, "_system_inshutdown",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_VFS_ROOT, "_VFS_ROOT",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_build_path, "_build_path",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_kvtophys, "_kvtophys",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_kernel_memory_allocate, "_kernel_memory_allocate",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_hostname, "_hostname",
		symbol_found, symbol_failed);
#ifdef DEBUG
	LOAD_SYMBOL(REAL_gLoadedKextSummaries, "_gLoadedKextSummaries",
		symbol_debug, symbol_failed);
	LOAD_SYMBOL(REAL__mh_execute_header, "__mh_execute_header",
		symbol_debug, symbol_failed);
#endif
	LOAD_SYMBOL(REAL_VNOP_LOOKUP, "_VNOP_LOOKUP",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_cpuid_info, "_cpuid_info",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_vfs_context_kernel, "_vfs_context_kernel",
		symbol_found, symbol_failed);
	LOAD_SYMBOL(REAL_fp_getfvp, "_fp_getfvp",
		symbol_found, symbol_failed);

	printf("%s: Loaded %d, "
#ifdef DEBUG
		"debug %d "
#endif
		"failed %d.\n",
		__func__,
		symbol_found,
#ifdef DEBUG
		symbol_debug,
#endif
		symbol_failed);

	delay(hz);

	return symbol_failed;
}
