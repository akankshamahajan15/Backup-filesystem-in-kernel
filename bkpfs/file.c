// SPDX-License-Identifier: GPL-2.0

#include "bkpfs.h"

static ssize_t bkpfs_read(struct file *file, char __user *buf,
			  size_t count, loff_t *ppos)
{
	int err;
	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;

	lower_file = bkpfs_lower_file(file);
	err = vfs_read(lower_file, buf, count, ppos);
	/* update our inode atime upon a successful lower read */
	if (err >= 0)
		fsstack_copy_attr_atime(d_inode(dentry),
					file_inode(lower_file));

	return err;
}

static ssize_t bkpfs_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	int err = 0;

	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;

	lower_file = bkpfs_lower_file(file);
	err = vfs_write(lower_file, buf, count, ppos);
	/* update our inode times+sizes upon a successful lower write */
	if (err >= 0) {
		fsstack_copy_inode_size(d_inode(dentry),
					file_inode(lower_file));
		fsstack_copy_attr_times(d_inode(dentry),
					file_inode(lower_file));
	}

	return err;
}

struct bkpfs_getdents_callback {
	struct dir_context ctx;
	struct dir_context *caller;
};

/*
 * If file name starts with .bkp then filldir returns 0
 * without calling lower f/s to hide that file.
 */
static int bkpfs_filldir(struct dir_context *ctx, const char *lower_name,
			 int lower_namelen, loff_t offset, u64 ino,
			 unsigned int d_type)
{
	int rc = 0;
	struct bkpfs_getdents_callback *buf =
		container_of(ctx, struct bkpfs_getdents_callback, ctx);

	if (memcmp(lower_name, ".bkp", 4) == 0)
		return rc;

	buf->caller->pos = buf->ctx.pos;
	rc = !dir_emit(buf->caller, lower_name, lower_namelen, ino, d_type);
	return rc;
}

static int bkpfs_readdir(struct file *file, struct dir_context *ctx)
{
	int err;
	struct file *lower_file = NULL;
	struct dentry *dentry = file->f_path.dentry;

	struct bkpfs_getdents_callback buf = {
		.ctx.actor = bkpfs_filldir,
		.caller = ctx,
	};

	lower_file = bkpfs_lower_file(file);
	err = iterate_dir(lower_file, &buf.ctx);
	file->f_pos = lower_file->f_pos;
	ctx->pos = buf.ctx.pos;

	if (err >= 0)		/* copy the atime */
		fsstack_copy_attr_atime(d_inode(dentry),
					file_inode(lower_file));
	return err;
}

static long bkpfs_unlocked_ioctl(struct file *file, unsigned int cmd,
				 unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;
	void __user *p = (void __user *)arg;

	lower_file = bkpfs_lower_file(file);
	switch (cmd) {
	case FS_BKP_VERSION_LIST:
		if (!S_ISREG(file_inode(lower_file)->i_mode))
			return -EINVAL;
		err = bkpfs_list_versions(file, p);
		if (err < 0)
			goto out;
		break;
	case FS_BKP_VERSION_DELETE:
		if (!S_ISREG(file_inode(lower_file)->i_mode))
			return -EINVAL;
		err = bkpfs_delete_version(file, p);
		if (err < 0)
			goto out;
		break;
	case FS_BKP_VERSION_RESTORE:
		if (!S_ISREG(file_inode(lower_file)->i_mode))
			return -EINVAL;
		err = bkpfs_restore_version(file, p);
		if (err < 0)
			goto out;
		break;
	case FS_BKP_VERSION_OPEN_FILE:
		if (!S_ISREG(file_inode(lower_file)->i_mode))
			return -EINVAL;
		err =  bkpfs_open_version(file, p);
		if (err < 0)
			goto out;
		break;
	case FS_BKP_VERSION_CLOSE_FILE:
		if (!S_ISREG(file_inode(lower_file)->i_mode))
			return -EINVAL;
		err =  bkpfs_close_version(file, p);
		if (err < 0)
			goto out;
		break;
	default:
		/* XXX: use vfs_ioctl if/when VFS exports it */
		if (!lower_file || !lower_file->f_op)
			goto out;
		if (lower_file->f_op->unlocked_ioctl)
			err = lower_file->f_op->unlocked_ioctl(lower_file,
							       cmd, arg);
		/*
		 * some ioctls can change inode attributes
		 * (EXT2_IOC_SETFLAGS)
		 */
		if (!err)
			fsstack_copy_attr_all(file_inode(file),
					      file_inode(lower_file));
	}
out:
	return err;
}

#ifdef CONFIG_COMPAT
static long bkpfs_compat_ioctl(struct file *file, unsigned int cmd,
			       unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;

	lower_file = bkpfs_lower_file(file);

	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->compat_ioctl)
		err = lower_file->f_op->compat_ioctl(lower_file, cmd, arg);

out:
	return err;
}
#endif

static int bkpfs_mmap(struct file *file, struct vm_area_struct *vma)
{
	int err = 0;
	bool willwrite;
	struct file *lower_file;
	const struct vm_operations_struct *saved_vm_ops = NULL;

	/* this might be deferred to mmap's writepage */
	willwrite = ((vma->vm_flags | VM_SHARED | VM_WRITE) == vma->vm_flags);

	/*
	 * File systems which do not implement ->writepage may use
	 * generic_file_readonly_mmap as their ->mmap op.  If you call
	 * generic_file_readonly_mmap with VM_WRITE, you'd get an -EINVAL.
	 * But we cannot call the lower ->mmap op, so we can't tell that
	 * writeable mappings won't work.  Therefore, our only choice is to
	 * check if the lower file system supports the ->writepage, and if
	 * not, return EINVAL (the same error that
	 * generic_file_readonly_mmap returns in that case).
	 */
	lower_file = bkpfs_lower_file(file);
	if (willwrite && !lower_file->f_mapping->a_ops->writepage) {
		err = -EINVAL;
		pr_err("bkpfs: lower fs does not support writeable mmap\n");
		goto out;
	}

	/*
	 * find and save lower vm_ops.
	 *
	 * XXX: the VFS should have a cleaner way of finding the lower vm_ops
	 */
	if (!BKPFS_F(file)->lower_vm_ops) {
		err = lower_file->f_op->mmap(lower_file, vma);
		if (err) {
			pr_err("bkpfs: lower mmap failed %d\n", err);
			goto out;
		}
		saved_vm_ops = vma->vm_ops; /* save: came from lower ->mmap */
	}

	/*
	 * Next 3 lines are all I need from generic_file_mmap.  I definitely
	 * don't want its test for ->readpage which returns -ENOEXEC.
	 */
	file_accessed(file);
	vma->vm_ops = &bkpfs_vm_ops;

	file->f_mapping->a_ops = &bkpfs_aops; /* set our aops */
	if (!BKPFS_F(file)->lower_vm_ops) /* save for our ->fault */
		BKPFS_F(file)->lower_vm_ops = saved_vm_ops;

out:
	return err;
}

static int bkpfs_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct file *lower_file = NULL;
	struct path lower_path;
	size_t isize;
	struct version vr;

	/* don't open unhashed/deleted files */
	if (d_unhashed(file->f_path.dentry)) {
		err = -ENOENT;
		goto out_err;
	}

	if (memcmp(file->f_path.dentry->d_name.name, ".bkpfs_", 7) == 0)
		return -ENOENT;

	file->private_data =
		kzalloc(sizeof(struct bkpfs_file_info), GFP_KERNEL);
	if (!BKPFS_F(file)) {
		err = -ENOMEM;
		goto out_err;
	}

	/* open lower object and link bkpfs's file struct to lower's */
	bkpfs_get_lower_path(file->f_path.dentry, &lower_path);
	lower_file = dentry_open(&lower_path, file->f_flags, current_cred());
	path_put(&lower_path);
	if (IS_ERR(lower_file)) {
		err = PTR_ERR(lower_file);
		lower_file = bkpfs_lower_file(file);
		if (lower_file) {
			bkpfs_set_lower_file(file, NULL);
			fput(lower_file); /* fput calls dput for lower_dentry */
		}
	} else {
		isize = (file_inode(lower_file))->i_size;
		bkpfs_set_lower_file(file, lower_file);
	}

	if (err)
		kfree(BKPFS_F(file));

	/*
	 * If file is in write mode then backup is created.
	 * For size 0, its treated as first time, so xattr are set
	 * After that backup function is called to create backup file
	 */
	if  (err == 0 &&  S_ISREG(file_inode(lower_file)->i_mode) &&
	     (lower_file->f_mode & FMODE_WRITE)) {
		if (isize > 0) {
			err = vfs_getxattr(lower_file->f_path.dentry,
					   "user.version", (void *)&vr,
					   sizeof(struct version));
			err = bkpfs_unlink_version(file->f_path.dentry,
						   vr.last + 1);
			err = bkpfs_create_backup(file);
			if (err < 0)
				goto out_err;
		} else {
			vr.count = 0;
			vr.max = ((struct bkpfs_sb_info *)
				  (inode->i_sb->s_fs_info))->maxver;
			vr.first = 0;
			vr.last = 0;
			err = vfs_setxattr(lower_file->f_path.dentry,
					   "user.version", (void *)&vr,
					   sizeof(struct version), 0);
		}
	}

	if (err == 0)
		fsstack_copy_attr_all(inode, bkpfs_lower_inode(inode));
out_err:
	return err;
}

static int bkpfs_flush(struct file *file, fl_owner_t id)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = bkpfs_lower_file(file);
	if (lower_file && lower_file->f_op && lower_file->f_op->flush) {
		filemap_write_and_wait(file->f_mapping);
		err = lower_file->f_op->flush(lower_file, id);
	}

	return err;
}

/* release all lower object references & free the file info structure */
static int bkpfs_file_release(struct inode *inode, struct file *file)
{
	struct file *lower_file;

	lower_file = bkpfs_lower_file(file);
	if (lower_file) {
		bkpfs_set_lower_file(file, NULL);
		fput(lower_file);
	}

	kfree(BKPFS_F(file));
	return 0;
}

static int bkpfs_fsync(struct file *file, loff_t start, loff_t end,
		       int datasync)
{
	int err;
	struct file *lower_file;
	struct path lower_path;
	struct dentry *dentry = file->f_path.dentry;

	err = __generic_file_fsync(file, start, end, datasync);
	if (err)
		goto out;
	lower_file = bkpfs_lower_file(file);
	bkpfs_get_lower_path(dentry, &lower_path);
	err = vfs_fsync_range(lower_file, start, end, datasync);
	bkpfs_put_lower_path(dentry, &lower_path);
out:
	return err;
}

static int bkpfs_fasync(int fd, struct file *file, int flag)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = bkpfs_lower_file(file);
	if (lower_file->f_op && lower_file->f_op->fasync)
		err = lower_file->f_op->fasync(fd, lower_file, flag);

	return err;
}

/*
 * Wrapfs cannot use generic_file_llseek as ->llseek, because it would
 * only set the offset of the upper file.  So we have to implement our
 * own method to set both the upper and lower file offsets
 * consistently.
 */
static loff_t bkpfs_file_llseek(struct file *file, loff_t offset, int whence)
{
	int err;
	struct file *lower_file;

	err = generic_file_llseek(file, offset, whence);
	if (err < 0)
		goto out;

	lower_file = bkpfs_lower_file(file);
	err = generic_file_llseek(lower_file, offset, whence);

out:
	return err;
}

/*
 * Wrapfs read_iter, redirect modified iocb to lower read_iter
 */
ssize_t
bkpfs_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = bkpfs_lower_file(file);
	if (!lower_file->f_op->read_iter) {
		err = -EINVAL;
		goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->read_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode atime as needed */
	if (err >= 0 || err == -EIOCBQUEUED)
		fsstack_copy_attr_atime(d_inode(file->f_path.dentry),
					file_inode(lower_file));
out:
	return err;
}

/*
 * Wrapfs write_iter, redirect modified iocb to lower write_iter
 */
ssize_t
bkpfs_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = bkpfs_lower_file(file);
	if (!lower_file->f_op->write_iter) {
		err = -EINVAL;
		goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->write_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
	/* update upper inode times/sizes as needed */
	if (err >= 0 || err == -EIOCBQUEUED) {
		fsstack_copy_inode_size(d_inode(file->f_path.dentry),
					file_inode(lower_file));
		fsstack_copy_attr_times(d_inode(file->f_path.dentry),
					file_inode(lower_file));
	}
out:
	return err;
}

const struct file_operations bkpfs_main_fops = {
	.llseek		= generic_file_llseek,
	.read		= bkpfs_read,
	.write		= bkpfs_write,
	.unlocked_ioctl	= bkpfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= bkpfs_compat_ioctl,
#endif
	.mmap		= bkpfs_mmap,
	.open		= bkpfs_open,
	.flush		= bkpfs_flush,
	.release	= bkpfs_file_release,
	.fsync		= bkpfs_fsync,
	.fasync		= bkpfs_fasync,
	.read_iter	= bkpfs_read_iter,
	.write_iter	= bkpfs_write_iter,
};

/* trimmed directory options */
const struct file_operations bkpfs_dir_fops = {
	.llseek		= bkpfs_file_llseek,
	.read		= generic_read_dir,
	.iterate	= bkpfs_readdir,
	.unlocked_ioctl	= bkpfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= bkpfs_compat_ioctl,
#endif
	.open		= bkpfs_open,
	.release	= bkpfs_file_release,
	.flush		= bkpfs_flush,
	.fsync		= bkpfs_fsync,
	.fasync		= bkpfs_fasync,
};
