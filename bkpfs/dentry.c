// SPDX-License-Identifier: GPL-2.0

#include "bkpfs.h"

/*
 * returns: -ERRNO if error (returned to user)
 *          0: tell VFS to invalidate dentry
 *          1: dentry is valid
 */
static int bkpfs_d_revalidate(struct dentry *dentry, unsigned int flags)
{
	struct path lower_path;
	struct dentry *lower_dentry;
	int err = 1;

	if (flags & LOOKUP_RCU)
		return -ECHILD;

	bkpfs_get_lower_path(dentry, &lower_path);
	lower_dentry = lower_path.dentry;
	if (!(lower_dentry->d_flags & DCACHE_OP_REVALIDATE))
		goto out;
	err = lower_dentry->d_op->d_revalidate(lower_dentry, flags);
out:
	bkpfs_put_lower_path(dentry, &lower_path);
	return err;
}

static void bkpfs_d_release(struct dentry *dentry)
{
	/* release and reset the lower paths */
	bkpfs_put_reset_lower_path(dentry);
	free_dentry_private_data(dentry);
}

const struct dentry_operations bkpfs_dops = {
	.d_revalidate	= bkpfs_d_revalidate,
	.d_release	= bkpfs_d_release,
};
