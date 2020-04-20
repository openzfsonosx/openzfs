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
 * Copyright (C) 2013 Jorgen Lundman <lundman@lundman.net>
 * Copyright (C) 2014 Brendon Humphrey <brendon.humphrey@mac.com>
 *
 */

/*
 * Provides an implementation of kstat that is backed by OSX sysctls.
 */

#include <sys/kstat.h>
#include <sys/debug.h>
#include <sys/thread.h>
#include <sys/cmn_err.h>
#include <sys/time.h>
#include <sys/sysctl.h>

/*
 * We need to get dynamically allocated memory from the kernel allocator
 * (Our needs are small, we wont blow the zone_map).
 */
extern void *kalloc(vm_size_t size);
extern void kfree(void *data, vm_size_t size);

/*
 * Statically declared toplevel OID that all kstats
 * will hang off.
 */
struct sysctl_oid_list sysctl__kstat_children;
SYSCTL_DECL(_kstat);
SYSCTL_NODE( , OID_AUTO, kstat, CTLFLAG_RW, 0, "kstat tree");

/*
 * Sysctl node tree structure.
 *
 * These are wired into the OSX sysctl structure
 * and also stored a list/tree/whatever for easy
 * location and destruction at shutdown time.
 */
typedef struct sysctl_tree_node {
	char					tn_kstat_name[KSTAT_STRLEN];
	struct sysctl_oid_list 	tn_children;
	struct sysctl_oid		tn_oid;
	struct sysctl_tree_node	*tn_next;
} sysctl_tree_node_t;

/*
 * Each named kstats consists of one or more named
 * fields which are implemented as OIDs parented
 * off the kstat OID.
 *
 * To implement the kstat interface, we need to be able
 * to call the update() function on the kstat to
 * allow the owner to populate the kstat values from
 * internal data.
 *
 * To do this we need the address of the kstat_named_t
 * which contains the data value, and the owning kstat_t.
 *
 * OIDs allow a single void* user argument, so we will
 * use a structure that contains both values and
 * point to that.
 */
typedef struct sysctl_leaf {
	kstat_t				*l_ksp;
	kstat_named_t		*l_named;
	struct sysctl_oid   l_oid;                  /* kstats are backed by a sysctl */
	char                l_name[64];             /* Name of the related sysctl */
	int                 l_oid_registered;       /* != 0 if sysctl was registered */
} sysctl_leaf_t;

/*
 * Extended kstat structure -- for internal use only.
 */
typedef struct ekstat {
	kstat_t         		e_ks;		/* the kstat itself */
	size_t          		e_size;		/* total allocation size */
	kthread_t       		*e_owner;	/* thread holding this kstat */
	kcondvar_t				e_cv;		/* wait for owner == NULL */
	struct sysctl_oid_list	e_children; /* contains the named values from the kstat */
	struct sysctl_oid		e_oid;		/* the kstat is itself an OID */
	sysctl_leaf_t			*e_vals;	/* array of OIDs that implement the children */
	uint64_t				e_num_vals;	/* size of e_vals array */
} ekstat_t;

struct sysctl_tree_node		*tree_nodes = 0;
struct sysctl_oid 			*e_sysctl = 0;

static void kstat_set_string(char *dst, const char *src)
{
    bzero(dst, KSTAT_STRLEN);
    (void) strlcpy(dst, src, KSTAT_STRLEN);
}

static struct sysctl_oid*
get_oid_with_name(struct sysctl_oid_list* list, char *name)
{
	struct sysctl_oid *oidp;

	SLIST_FOREACH(oidp, list, oid_link) {
		if (strcmp(name, oidp->oid_name) == 0) {
			return oidp;
		}
	}

	return 0;
}

static void
init_oid_tree_node(struct sysctl_oid_list* parent, char *name, sysctl_tree_node_t* node)
{
	strlcpy(node->tn_kstat_name, name, KSTAT_STRLEN);

	node->tn_oid.oid_parent = parent;
	node->tn_oid.oid_link.sle_next = 0;
	node->tn_oid.oid_number = OID_AUTO;
	node->tn_oid.oid_arg2 = 0;
	node->tn_oid.oid_name = &node->tn_kstat_name[0];
	node->tn_oid.oid_descr = "";
	node->tn_oid.oid_version = SYSCTL_OID_VERSION;
	node->tn_oid.oid_refcnt = 0;
	node->tn_oid.oid_handler = 0;
	node->tn_oid.oid_kind = CTLTYPE_NODE|CTLFLAG_RW|CTLFLAG_OID2;
	node->tn_oid.oid_fmt = "N";
	node->tn_oid.oid_arg1 = (void*)(&node->tn_children);

	sysctl_register_oid(&node->tn_oid);

	node->tn_next = tree_nodes;
	tree_nodes = node;
}

static struct sysctl_oid_list*
get_kstat_parent(struct sysctl_oid_list* root, char *module_name, char* class_name)
{
	struct sysctl_oid *the_module = 0;
	struct sysctl_oid *the_class = 0;
	sysctl_tree_node_t *new_node = 0;
	struct sysctl_oid_list *container = root;

	/*
	 * Locate/create the module
	 */
	the_module = get_oid_with_name(root, module_name);

	if (!the_module) {
		new_node = kalloc(sizeof(sysctl_tree_node_t));
        bzero(new_node, sizeof(sysctl_tree_node_t));
		init_oid_tree_node(root, module_name, new_node);
		the_module = &new_node->tn_oid;
	}

	/*
	 * Locate/create the class
	 */
	container = the_module->oid_arg1;
	the_class = get_oid_with_name(container, class_name);

	if (!the_class) {
		new_node = kalloc(sizeof(sysctl_tree_node_t));
        bzero(new_node, sizeof(sysctl_tree_node_t));
		init_oid_tree_node(container, class_name, new_node);
		the_class = &new_node->tn_oid;
	}

	container = the_class->oid_arg1;
	return container;
}

static int kstat_handle_i64 SYSCTL_HANDLER_ARGS
{
    int error = 0;
	sysctl_leaf_t *params = (sysctl_leaf_t*)(arg1);
	kstat_named_t *named = params->l_named;
	kstat_t *ksp  = params->l_ksp;
	kmutex_t *lock = ksp->ks_lock;
	int lock_needs_release = 0;

	if (lock && !MUTEX_NOT_HELD(lock)) {
		mutex_enter(lock);
		lock_needs_release = 1;
	}

    if(!error && req->newptr) {
        /*
         * Write request - first read add current values for the kstat
         * (remember that is sysctl is likely only one of many
         *  values that make up the kstat).
         */
        if (ksp->ks_update) {
            ksp->ks_update(ksp, KSTAT_READ);
        }

        /* Copy the new value from user space */
	(void)copyin(req->newptr, &named->value.i64, sizeof(named->value.i64));

        /* and invoke the update operation */
        if (ksp->ks_update) {
            error = ksp->ks_update(ksp, KSTAT_WRITE);
        }
    } else {
        /*
         * Read request
         */
        if (ksp->ks_update) {
            ksp->ks_update(ksp, KSTAT_READ);
        }
        error = SYSCTL_OUT(req, &named->value.i64, sizeof(int64_t));
    }

	if (lock_needs_release) {
		mutex_exit(lock);
	}

    return error;
}

static int kstat_handle_ui64 SYSCTL_HANDLER_ARGS
{
	int error = 0;
	sysctl_leaf_t *params = (sysctl_leaf_t*)(arg1);
	kstat_named_t *named = params->l_named;
	kstat_t *ksp  = params->l_ksp;
	kmutex_t *lock = ksp->ks_lock;
	int lock_needs_release = 0;

	if (lock && !MUTEX_NOT_HELD(lock)) {
		mutex_enter(lock);
		lock_needs_release = 1;
	}
    if(!error && req->newptr) {
        /*
         * Write request - first read add current values for the kstat
         * (remember that is sysctl is likely only one of many
         *  values that make up the kstat).
         */
        if (ksp->ks_update) {
            ksp->ks_update(ksp, KSTAT_READ);
        }

        /* Copy the new value from user space */
	(void)copyin(req->newptr, &named->value.ui64, sizeof(named->value.ui64));

        /* and invoke the update operation */
        if (ksp->ks_update) {
            error = ksp->ks_update(ksp, KSTAT_WRITE);
        }
    } else {
        /*
         * Read request
         */
        if (ksp->ks_update) {
            ksp->ks_update(ksp, KSTAT_READ);
        }
        error = SYSCTL_OUT(req, &named->value.ui64, sizeof(uint64_t));
    }

	if (lock_needs_release) {
		mutex_exit(lock);
	}

    return error;
}

static int kstat_handle_string SYSCTL_HANDLER_ARGS
{
	int error = 0;
	sysctl_leaf_t *params = (sysctl_leaf_t*)(arg1);
	kstat_named_t *named = params->l_named;
	kstat_t *ksp  = params->l_ksp;
	kmutex_t *lock = ksp->ks_lock;
	int lock_needs_release = 0;

	if (lock && !MUTEX_NOT_HELD(lock)) {
		mutex_enter(lock);
		lock_needs_release = 1;
	}
    if(!error && req->newptr) {
        /*
         * Write request - first read add current values for the kstat
         * (remember that is sysctl is likely only one of many
         *  values that make up the kstat).
         */
        if (ksp->ks_update) {
            ksp->ks_update(ksp, KSTAT_READ);
        }

        /* Copy the new value from user space */
		/* This should use copyinstr when copying in string from userland
		 * Fix this before attempting to use type STRING with kstat */
        named->value.string.addr.ptr = (char *)(req->newptr);
		named->value.string.len = strlen((char *)(req->newptr))+1;

        /* and invoke the update operation */
        if (ksp->ks_update) {
            error = ksp->ks_update(ksp, KSTAT_WRITE);
        }
    } else {
        /*
         * Read request
         */
        if (ksp->ks_update) {
            ksp->ks_update(ksp, KSTAT_READ);
        }
        error = SYSCTL_OUT(req, named->value.string.addr.ptr,
						   named->value.string.len);
    }

	if (lock_needs_release) {
		mutex_exit(lock);
	}

    return error;
}

kstat_t *
kstat_create(char *ks_module, int ks_instance, char *ks_name, char *ks_class,
             uchar_t ks_type,
             ulong_t ks_ndata, uchar_t ks_flags)
{
    kstat_t *ksp = 0;
    ekstat_t *e = 0;
    size_t size = 0;

    /*
     * Allocate memory for the new kstat header.
     */
    size = sizeof (ekstat_t);
    e = (ekstat_t *)kalloc(size);
    bzero(e, size);
    if (e == NULL) {
        cmn_err(CE_NOTE, "kstat_create('%s', %d, '%s'): "
                "insufficient kernel memory",
                ks_module, ks_instance, ks_name);
        return (NULL);
    }
    e->e_size = size;

    cv_init(&e->e_cv, NULL, CV_DEFAULT, NULL);

    /*
     * Initialize as many fields as we can.  The caller may reset
     * ks_lock, ks_update, ks_private, and ks_snapshot as necessary.
     * Creators of virtual kstats may also reset ks_data.  It is
     * also up to the caller to initialize the kstat data section,
     * if necessary.  All initialization must be complete before
     * calling kstat_install().
     */
    ksp = &e->e_ks;

    ksp->ks_crtime          = gethrtime();
    kstat_set_string(ksp->ks_module, ks_module);
    ksp->ks_instance        = ks_instance;
    kstat_set_string(ksp->ks_name, ks_name);
    ksp->ks_type            = ks_type;
    kstat_set_string(ksp->ks_class, ks_class);
    ksp->ks_flags           = ks_flags | KSTAT_FLAG_INVALID;
    ksp->ks_ndata           = ks_ndata;
    ksp->ks_snaptime        = ksp->ks_crtime;
	ksp->ks_lock            = 0;

	/*
	 * Initialise the sysctl that represents this kstat
	 */
	e->e_children.slh_first = 0;

	e->e_oid.oid_parent = get_kstat_parent(&sysctl__kstat_children,
											ksp->ks_module, ksp->ks_class);
	e->e_oid.oid_link.sle_next = 0;
	e->e_oid.oid_number = OID_AUTO;
	e->e_oid.oid_arg2 = 0;
	e->e_oid.oid_name = ksp->ks_name;
	e->e_oid.oid_descr = "";
	e->e_oid.oid_version = SYSCTL_OID_VERSION;
	e->e_oid.oid_refcnt = 0;
	e->e_oid.oid_handler = 0;
	e->e_oid.oid_kind = CTLTYPE_NODE|CTLFLAG_RW|CTLFLAG_OID2;
	e->e_oid.oid_fmt = "N";
	e->e_oid.oid_arg1 = (void*)(&e->e_children);

	/* If VIRTUAL we allocate memory to store data */
	if (ks_flags & KSTAT_FLAG_VIRTUAL)
		ksp->ks_data    = NULL;
	else
		ksp->ks_data    = (void *)kmem_zalloc(
			sizeof(kstat_named_t) * ks_ndata, KM_SLEEP);


	sysctl_register_oid(&e->e_oid);

    return (ksp);
}

void
kstat_install(kstat_t *ksp)
{
	ekstat_t *e = (ekstat_t*)ksp;
	kstat_named_t *named_base = 0;
	sysctl_leaf_t *vals_base = 0;
	sysctl_leaf_t *params = 0;
	int oid_permissions = CTLFLAG_RD;

    if (ksp->ks_type == KSTAT_TYPE_NAMED) {

		if (ksp->ks_flags & KSTAT_FLAG_WRITABLE) {
			oid_permissions |= CTLFLAG_RW;
		}

		// Create the leaf node OID objects
		e->e_vals = (sysctl_leaf_t*)kalloc(ksp->ks_ndata * sizeof(sysctl_leaf_t));
        bzero(e->e_vals, ksp->ks_ndata * sizeof(sysctl_leaf_t));
		e->e_num_vals = ksp->ks_ndata;

        named_base = (kstat_named_t*)(ksp->ks_data);
		vals_base = e->e_vals;

        for (int i=0; i < ksp->ks_ndata; i++) {

            int oid_valid = 1;

            kstat_named_t *named = &named_base[i];
			sysctl_leaf_t *val = &vals_base[i];

            // Perform basic initialisation of the sysctl.
            //
            // The sysctl will be kstat.<module>.<class>.<name>.<data name>
            snprintf(val->l_name, KSTAT_STRLEN, "%s", named->name);

            val->l_oid.oid_parent = &e->e_children;
            val->l_oid.oid_link.sle_next = 0;
            val->l_oid.oid_number = OID_AUTO;
            val->l_oid.oid_arg2 = 0;
            val->l_oid.oid_name = val->l_name;
            val->l_oid.oid_descr = "";
            val->l_oid.oid_version = SYSCTL_OID_VERSION;
            val->l_oid.oid_refcnt = 0;

            // Based on the kstat type flags, provide location
            // of data item and associated type and handler
            // flags to the sysctl.
            switch (named->data_type) {
                case KSTAT_DATA_INT64:
					params = (sysctl_leaf_t*)kalloc(sizeof(sysctl_leaf_t));
					params->l_named = named;
					params->l_ksp = ksp;

                    val->l_oid.oid_handler = kstat_handle_i64;
                    val->l_oid.oid_kind = CTLTYPE_QUAD|oid_permissions|CTLFLAG_OID2;
                    val->l_oid.oid_fmt = "Q";
                    val->l_oid.oid_arg1 = (void*)params;
					params = 0;
                    break;
                case KSTAT_DATA_UINT64:
					params = (sysctl_leaf_t*)kalloc(sizeof(sysctl_leaf_t));
					params->l_named = named;
					params->l_ksp = ksp;

					val->l_oid.oid_handler = kstat_handle_ui64;
                    val->l_oid.oid_kind = CTLTYPE_QUAD|oid_permissions|CTLFLAG_OID2;
                    val->l_oid.oid_fmt = "Q";
					val->l_oid.oid_arg1 = (void*)params;
                    break;
                case KSTAT_DATA_INT32:
                    val->l_oid.oid_handler = sysctl_handle_int;
                    val->l_oid.oid_kind = CTLTYPE_INT|oid_permissions|CTLFLAG_OID2;
                    val->l_oid.oid_fmt = "I";
                    val->l_oid.oid_arg1 = &named->value.i32;
                    break;
                case KSTAT_DATA_UINT32:
                    val->l_oid.oid_handler = sysctl_handle_int;
                    val->l_oid.oid_kind = CTLTYPE_INT|oid_permissions|CTLFLAG_OID2;
                    val->l_oid.oid_fmt = "IU";
                    val->l_oid.oid_arg1 = &named->value.ui32;
                    break;
                case KSTAT_DATA_LONG:
                    val->l_oid.oid_handler = sysctl_handle_long;
                    val->l_oid.oid_kind = CTLTYPE_INT|oid_permissions|CTLFLAG_OID2;
                    val->l_oid.oid_fmt = "L";
                    val->l_oid.oid_arg1 = &named->value.l;
                    break;
                case KSTAT_DATA_ULONG:
                    val->l_oid.oid_handler = sysctl_handle_long;
                    val->l_oid.oid_kind = CTLTYPE_INT|oid_permissions|CTLFLAG_OID2;
                    val->l_oid.oid_fmt = "L";
                    val->l_oid.oid_arg1 = &named->value.ul;
                    break;
                case KSTAT_DATA_STRING:
					params = (sysctl_leaf_t*)kalloc(sizeof(sysctl_leaf_t));
					params->l_named = named;
					params->l_ksp = ksp;
                    val->l_oid.oid_handler = kstat_handle_string;
                    val->l_oid.oid_kind = CTLTYPE_STRING|oid_permissions|CTLFLAG_OID2;
                    val->l_oid.oid_fmt = "S";
                    val->l_oid.oid_arg1 = (void*)params;
                    break;

                case KSTAT_DATA_CHAR:
                default:
                    oid_valid = 0;
                    break;
            }

            // Finally publish the OID, provided that there were no issues initialising it.
            if (oid_valid) {
                sysctl_register_oid(&val->l_oid);
                val->l_oid_registered = 1;
            } else {
                val->l_oid_registered = 0;
            }
        }
	}

    ksp->ks_flags &= ~KSTAT_FLAG_INVALID;
}

static void
remove_child_sysctls(ekstat_t *e)
{
	kstat_t *ksp = &e->e_ks;
	kstat_named_t *named_base = (kstat_named_t*)(ksp->ks_data);
	sysctl_leaf_t *vals_base = e->e_vals;

	for (int i=0; i < ksp->ks_ndata; i++) {
		if (vals_base[i].l_oid_registered) {
			sysctl_unregister_oid(&vals_base[i].l_oid);
			vals_base[i].l_oid_registered = 0;
		}

		if (named_base[i].data_type == KSTAT_DATA_INT64 ||
			named_base[i].data_type == KSTAT_DATA_UINT64 ||
			named_base[i].data_type == KSTAT_DATA_STRING) {

			sysctl_leaf_t *leaf = (sysctl_leaf_t*)vals_base[i].l_oid.oid_arg1;
			kfree(leaf, sizeof(sysctl_leaf_t));
		}
	}
}

void
kstat_delete(kstat_t *ksp)
{
    ekstat_t *e = (ekstat_t *)ksp;
	kmutex_t *lock = ksp->ks_lock;
	int lock_needs_release = 0;

    // destroy the sysctl
    if (ksp->ks_type == KSTAT_TYPE_NAMED) {

		if (lock && MUTEX_NOT_HELD(lock)) {
			mutex_enter(lock);
			lock_needs_release = 1;
		}

		remove_child_sysctls(e);

		if (lock_needs_release) {
			mutex_exit(lock);
		}
    }

	sysctl_unregister_oid(&e->e_oid);

	if (e->e_vals) {
		kfree(e->e_vals, sizeof(sysctl_leaf_t) * e->e_num_vals);
	}

	if (!(ksp->ks_flags & KSTAT_FLAG_VIRTUAL))
		kmem_free(ksp->ks_data, sizeof(kstat_named_t) * ksp->ks_ndata);

    cv_destroy(&e->e_cv);
	kfree(e, e->e_size);
}

void
kstat_named_setstr(kstat_named_t *knp, const char *src)
{
	if (knp->data_type != KSTAT_DATA_STRING)
		panic("kstat_named_setstr('%p', '%p'): "
			  "named kstat is not of type KSTAT_DATA_STRING",
			  (void *)knp, (void *)src);

	KSTAT_NAMED_STR_PTR(knp) = (char *)src;
	if (src != NULL)
		KSTAT_NAMED_STR_BUFLEN(knp) = strlen(src) + 1;
	else
		KSTAT_NAMED_STR_BUFLEN(knp) = 0;
}

void
kstat_named_init(kstat_named_t *knp, const char *name, uchar_t data_type)
{
	kstat_set_string(knp->name, name);
	knp->data_type = data_type;

	if (data_type == KSTAT_DATA_STRING)
		kstat_named_setstr(knp, NULL);
}


void
kstat_waitq_enter(kstat_io_t *kiop)
{
}

void
kstat_waitq_exit(kstat_io_t *kiop)
{
}

void
kstat_runq_enter(kstat_io_t *kiop)
{
}

void
kstat_runq_exit(kstat_io_t *kiop)
{
}

void
__kstat_set_raw_ops(kstat_t *ksp,
                    int (*headers)(char *buf, size_t size),
                    int (*data)(char *buf, size_t size, void *data),
                    void *(*addr)(kstat_t *ksp, loff_t index))
{
}

void
spl_kstat_init()
{
    /*
	 * Create the kstat root OID
	 */
	sysctl_register_oid(&sysctl__kstat);
}

void
spl_kstat_fini()
{
	/*
	 * Destroy the kstat module/class/name tree
	 *
	 * Done in two passes, first unregisters all
	 * of the oids, second releases all the memory.
	 */

	sysctl_tree_node_t *iter = tree_nodes;
	while (iter) {
		sysctl_tree_node_t *tn = iter;
		iter = tn->tn_next;
		sysctl_unregister_oid(&tn->tn_oid);
	}

	while (tree_nodes) {
		sysctl_tree_node_t *tn = tree_nodes;
		tree_nodes = tn->tn_next;
		kfree(tn, sizeof(sysctl_tree_node_t));
	}

    /*
     * Destroy the root oid
     */
    sysctl_unregister_oid(&sysctl__kstat);
}
