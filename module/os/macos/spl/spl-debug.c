#include <sys/sysmacros.h>

#include <sys/kernel_map.h>

/* Debug log support enabled */
__attribute__((noinline)) int assfail(const char *str, const char *file,
	unsigned int line) __attribute__((optnone))
{
	return (1); /* Must return true for ASSERT macro */
}

const char *
spl_panicstr(void)
{
	return (NULL);
}

int
spl_system_inshutdown(void)
{
	return (*REAL_system_inshutdown);
}

#include <mach-o/loader.h>
typedef struct mach_header_64	kernel_mach_header_t;
#include <mach-o/nlist.h>
typedef struct nlist_64			kernel_nlist_t;

typedef struct segment_command_64 kernel_segment_command_t;

typedef struct _loaded_kext_summary {
	char		name[KMOD_MAX_NAME];
	uuid_t		uuid;
	uint64_t	address;
	uint64_t	size;
	uint64_t	version;
	uint32_t	loadTag;
	uint32_t	flags;
	uint64_t	reference_list;
} OSKextLoadedKextSummary;

typedef struct _loaded_kext_summary_header {
    uint32_t version;
    uint32_t entry_size;
    uint32_t numSummaries;
    uint32_t reserved; /* explicit alignment for gdb  */
    OSKextLoadedKextSummary summaries[0];
} OSKextLoadedKextSummaryHeader;

#ifdef DEBUG

extern OSKextLoadedKextSummaryHeader * gLoadedKextSummaries;

typedef struct _cframe_t {
	struct _cframe_t	*prev;
	uintptr_t			caller;
#if PRINT_ARGS_FROM_STACK_FRAME
	unsigned			args[0];
#endif
} cframe_t;

extern kernel_mach_header_t _mh_execute_header;
extern kmod_info_t * kmod; /* the list of modules */
#endif

extern addr64_t  kvtophys(vm_offset_t va);

static int
panic_print_macho_symbol_name(kernel_mach_header_t *mh, vm_address_t search,
    const char *module_name)
{
	kernel_nlist_t			*sym = NULL;
	struct load_command		*cmd;
	kernel_segment_command_t	*orig_ts = NULL, *orig_le = NULL;
	struct symtab_command		*orig_st = NULL;
	unsigned int			i;
	char				*strings, *bestsym = NULL;
	vm_address_t			bestaddr = 0, diff, curdiff;

	/*
	 * Assume that if it's loaded and linked into the kernel,
	 * it's a valid Mach-O
	 */
	cmd = (struct load_command *) &mh[1];
	for (i = 0; i < mh->ncmds; i++) {
		if (cmd->cmd == LC_SEGMENT_64) {
			kernel_segment_command_t *orig_sg =
			    (kernel_segment_command_t *) cmd;

			if (strncmp(SEG_TEXT, orig_sg->segname,
			    sizeof (orig_sg->segname)) == 0)
				orig_ts = orig_sg;
			else if (strncmp(SEG_LINKEDIT, orig_sg->segname,
			    sizeof (orig_sg->segname)) == 0)
				orig_le = orig_sg;
			/* pre-Lion i386 kexts have a single unnamed segment */
			else if (strncmp("", orig_sg->segname,
			    sizeof (orig_sg->segname)) == 0)
				orig_ts = orig_sg;
		} else if (cmd->cmd == LC_SYMTAB)
			orig_st = (struct symtab_command *) cmd;

		cmd = (struct load_command *) ((uintptr_t) cmd + cmd->cmdsize);
	}

	if ((orig_ts == NULL) || (orig_st == NULL) || (orig_le == NULL))
		return (0);

	if ((search < orig_ts->vmaddr) ||
	    (search >= orig_ts->vmaddr + orig_ts->vmsize)) {
		/* search out of range for this mach header */
		return (0);
	}

	sym = (kernel_nlist_t *)(uintptr_t)(orig_le->vmaddr +
	    orig_st->symoff - orig_le->fileoff);
	strings = (char *)(uintptr_t)(orig_le->vmaddr +
	    orig_st->stroff - orig_le->fileoff);
	diff = search;

	for (i = 0; i < orig_st->nsyms; i++) {
		if (sym[i].n_type & N_STAB) continue;

		if (sym[i].n_value <= search) {
			curdiff = search - (vm_address_t)sym[i].n_value;
			if (curdiff < diff) {
				diff = curdiff;
				bestaddr = sym[i].n_value;
				bestsym = strings + sym[i].n_un.n_strx;
			}
		}
	}

	if (bestsym != NULL) {
		if (diff != 0) {
			printf("%s : %s + 0x%lx", module_name, bestsym,
			    (unsigned long)diff);
		} else {
			printf("%s : %s", module_name, bestsym);
		}
		return (1);
	}
	return (0);
}


static void
panic_print_kmod_symbol_name(vm_address_t search)
{
	u_int i;

#ifdef DEBUG
	if (gLoadedKextSummaries == NULL)
		return;
	for (i = 0; i < gLoadedKextSummaries->numSummaries; ++i) {
		OSKextLoadedKextSummary *summary =
		    gLoadedKextSummaries->summaries + i;

		if ((search >= summary->address) &&
		    (search < (summary->address + summary->size))) {
			kernel_mach_header_t *header =
			    (kernel_mach_header_t *)(uintptr_t)summary->address;
			if (panic_print_macho_symbol_name(header, search,
			    summary->name) == 0) {
				printf("%s + %llu", summary->name,
				    (unsigned long)search - summary->address);
			}
			break;
		}
	}
#endif
}

#ifdef DEBUG
static void
panic_print_symbol_name(vm_address_t search)
{
	/* try searching in the kernel */
	if (panic_print_macho_symbol_name(&_mh_execute_header,
	    search, "mach_kernel") == 0) {
		/* that failed, now try to search for the right kext */
		panic_print_kmod_symbol_name(search);
	}
}
#endif

void
spl_backtrace(char *thesignal)
{
#ifdef DEBUG
	void *stackptr;

	printf("SPL: backtrace \"%s\"\n", thesignal);

#if defined(__i386__)
	__asm__ volatile("movl %%ebp, %0" : "=m" (stackptr));
#elif defined(__x86_64__)
	__asm__ volatile("movq %%rbp, %0" : "=m" (stackptr));
#endif

	int frame_index;
	int nframes = 16;
	cframe_t *frame = (cframe_t *)stackptr;

	for (frame_index = 0; frame_index < nframes; frame_index++) {
		vm_offset_t curframep = (vm_offset_t) frame;
		if (!curframep)
			break;
		if (curframep & 0x3) {
			printf("SPL: Unaligned frame\n");
			break;
		}
		if (!kvtophys(curframep) ||
		    !kvtophys(curframep + sizeof (cframe_t) - 1)) {
			printf("SPL: No mapping exists for frame pointer\n");
			break;
		}
		printf("SPL: %p : 0x%lx ", frame, frame->caller);
		panic_print_symbol_name((vm_address_t)frame->caller);
		printf("\n");
		frame = frame->prev;
	}
#endif
}

int
getpcstack(uintptr_t *pcstack, int pcstack_limit)
{
	int  depth = 0;
#ifdef DEBUG
	void *stackptr;

#if defined(__i386__)
	__asm__ volatile("movl %%ebp, %0" : "=m" (stackptr));
#elif defined(__x86_64__)
	__asm__ volatile("movq %%rbp, %0" : "=m" (stackptr));
#endif

	int frame_index;
	int nframes = pcstack_limit;
	cframe_t *frame = (cframe_t *)stackptr;

	for (frame_index = 0; frame_index < nframes; frame_index++) {
		vm_offset_t curframep = (vm_offset_t) frame;
		if (!curframep)
			break;
		if (curframep & 0x3) {
			break;
		}
		if (!kvtophys(curframep) ||
		    !kvtophys(curframep + sizeof (cframe_t) - 1)) {
			break;
		}
		pcstack[depth++] = frame->caller;
		frame = frame->prev;
	}
#endif
	return (depth);
}

void
print_symbol(uintptr_t symbol)
{
#ifdef DEBUG
	printf("SPL: ");
	panic_print_symbol_name((vm_address_t)(symbol));
	printf("\n");
#endif
}
