// SPDX-License-Identifier: GPL-2.0

#include "bkpfs.h"

/*
 * This function get the int value and converts it into string
 * and populate that sting in str passed as argument.
 *
 * Returns len of string on success, else 0.
 */
int get_string(char **str, int V)
{
	int rem = V;
	int len = 0;
	int  i;

	while (rem) {
		rem = rem / 10;
		len++;
	}
	i = len - 1;
	*str = kmalloc(len + 1, GFP_KERNEL);

	while (V) {
		(*str)[i] = (V % 10) + '0';
		V = V / 10;
		i--;
	}
	(*str)[len] = '\0';

	return len;
}

/*
 * This function checks the version passed as string from userland
 * and copies that to kernel buffer.
 * struct xstr contains len of the string and the string itself that
 * cab be NEW/OLD/ALL/VERSION NUM
 */
int get_xstr(void __user *p, struct file *file, struct xstr **x_str)
{
	int err = 0;

	if (unlikely(!access_ok(VERIFY_READ, p, sizeof(struct xstr))))
		return -EFAULT;
	if (unlikely(!access_ok(VERIFY_READ, p, sizeof(struct xstr) +
				((struct xstr *)p)->len)))
		return -EFAULT;
	*x_str = kmalloc(sizeof(struct xstr) + ((struct xstr *)p)->len,
			 GFP_KERNEL);
	if (!(*x_str))
		return -ENOMEM;

	if (copy_from_user(*x_str, p, sizeof(struct xstr) +
			   ((struct xstr *)p)->len)) {
		err = -EINVAL;
		goto out_free_xstr;
	}

out_free_xstr:
	return err;
}

int bkpfs_set_version_xattr(struct dentry *dentry, struct version *vr)
{
	int err;

	err = vfs_setxattr(dentry, "user.version", (void *)vr,
			   sizeof(struct version), 0);
	return err;
}

int bkpfs_get_version_xattr(struct dentry *dentry, struct version *vr)
{
	int err;

	err = vfs_getxattr(dentry, "user.version", vr,
			   sizeof(struct version));
	return err;
}

/*
 * This function gets the version V and create negative
 * dentry for the backup file and  does the lookup of that
 * negative dentry.
 *
 * Returns positive dentry on success, else error
 */
struct dentry *bkpfs_get_bkdentry(struct dentry *dentry, int V)
{
	struct dentry *ret_dentry = NULL, *parent, *bk_dentry;
	char *newname, *version;
	struct qstr this;
	int len, i = 0, v_len;

	parent = dget_parent(dentry);

	v_len = get_string(&version, V);
	len = strlen(dentry->d_name.name);

	newname = kmalloc(len + v_len + 6, GFP_KERNEL);
	if (!newname) {
		ret_dentry = ERR_PTR(-ENOMEM);
		goto out_free_dentries;
	}

	/* creates backup name as .bkps<count>_<originalfile> */
	memcpy(newname, ".bkp", 4);
	i = 4;
	memcpy(newname + i, version, v_len);
	i = i + v_len;
	memcpy(newname + i, "_", 1);
	i  = i + 1;
	memcpy(newname + i, dentry->d_name.name, len + 1);

	this.name = newname;
	this.len = len + 8;
	this.hash = full_name_hash(parent, this.name, this.len);

	bk_dentry = d_alloc(parent, &this);
	ret_dentry = bkpfs_lookup(parent->d_inode, bk_dentry, 0);
	if (IS_ERR(ret_dentry))
		goto out_free_newname;

	ret_dentry = bk_dentry;

out_free_newname:
	kfree(newname);
out_free_dentries:
	dput(parent);
	return ret_dentry;
}

/*
 * This function copies the data from lower_infile to lower_outfile.
 *
 * Returns 0 on success, else error.
 */
int bkpfs_copy_data(struct file *lower_infile, struct file *lower_outfile)
{
	int err = 0;
	loff_t bytes_read, bytes_wrote;
	loff_t in_off = 0, out_off = 0;
	fmode_t temp_read, temp_write;
	void *data;

	data = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!data)
		return err;

	temp_read = lower_infile->f_mode;
	lower_infile->f_mode |= FMODE_READ;
	lower_infile->f_mode |= FMODE_CAN_READ;

	temp_write = lower_outfile->f_mode;
	lower_outfile->f_mode |= FMODE_WRITE;
	lower_outfile->f_mode |= FMODE_CAN_WRITE;

	while ((bytes_read = kernel_read(lower_infile, data,
					 PAGE_SIZE, &in_off))) {
		bytes_wrote = kernel_write(lower_outfile, data, bytes_read,
					   &out_off);
		if (bytes_wrote < 0)
			goto out_free_data;
	}
out_free_data:
	kfree(data);

	if (bytes_read < 0)
		err = bytes_read;
	if (bytes_wrote < 0)
		err = bytes_wrote;

	lower_infile->f_mode = temp_read;
	lower_outfile->f_mode = temp_write;

	return err;
}

/*
 * This functions takes a file name and creates its backup version.
 * It copies the content of file to new backup file with incremented
 * version and update the extended attributes of the original file.
 * If version exceeds, it deletes the oldest version and then creates
 * a new one.
 *
 * Returns 0 on success, else error.
 */
int bkpfs_create_backup(struct file *file)
{
	int err = 0;
	struct dentry *dentry, *bk_dentry, *parent;
	struct dentry *lower_bk_dentry, *lower_parent_dentry;
	struct file *lower_bk_file, *lower_file;
	struct path lower_bk_path;
	struct version vr;
	struct iattr attr;

	dentry = file->f_path.dentry;
	dget(dentry);
	parent = dget_parent(dentry);
	lower_file = bkpfs_lower_file(file);

	err = bkpfs_get_version_xattr(lower_file->f_path.dentry, &vr);
	if (err < 0)
		goto out_free_dentry;

	/* If count exceeds the maximum allowed backup versions then delete
	 *  the oldest version and create a now
	 */
	if (vr.count == vr.max) {
		err = bkpfs_unlink_version(dentry, vr.first);
		if (err < 0)
			goto out_free_dentry;
		vr.first++;
		vr.count--;
	}

	if (vr.count == 0)
		vr.first = vr.first + 1;
	vr.last++;
	vr.count++;

	/* create a negative dentry and do lookup*/
	bk_dentry = bkpfs_get_bkdentry(dentry, vr.last);
	if (IS_ERR(bk_dentry)) {
		err = PTR_ERR(bk_dentry);
		goto out_bk_dentry;
	}

	bkpfs_get_lower_path(bk_dentry, &lower_bk_path);
	lower_bk_dentry = lower_bk_path.dentry;
	lower_parent_dentry = lock_parent(lower_bk_dentry);

	/* create inode of backup file */
	err = vfs_create(d_inode(lower_parent_dentry), lower_bk_dentry,
			 0777, true);
	if (err)
		goto out_release_lock;

	err = bkpfs_interpose(bk_dentry, parent->d_inode->i_sb, &lower_bk_path);

out_release_lock:
	unlock_dir(lower_parent_dentry);

	if (err)
		goto out_lower_path;

	/* open the backup file */
	lower_bk_file = dentry_open(&lower_bk_path, lower_file->f_flags,
				    current_cred());
	if (IS_ERR(lower_bk_file)) {
		err = PTR_ERR(lower_bk_file);
		goto out_lower_path;
	}

	err = bkpfs_set_version_xattr(lower_file->f_path.dentry, &vr);
	if (err < 0)
		goto out_lower_path;

	/*
	 * copy the data from original file to backup file and
	 * update the attr of both upper and lower inodes
	 */
	err = bkpfs_copy_data(lower_file, lower_bk_file);
	if (err < 0) {
		pr_err("Error in copying data from file to backup file\n");
	} else {
		attr.ia_valid = ATTR_SIZE;
		attr.ia_size = file_inode(lower_file)->i_size;

		inode_lock(file_inode(lower_bk_file));
		notify_change(lower_bk_dentry, &attr, NULL);
		inode_unlock(file_inode(lower_bk_file));

		fsstack_copy_attr_all(bk_dentry->d_inode,
				      file_inode(lower_bk_file));
		fsstack_copy_inode_size(bk_dentry->d_inode,
					file_inode(lower_bk_file));
		fsstack_copy_attr_times(parent->d_inode,
					bkpfs_lower_inode(parent->d_inode));
		fsstack_copy_inode_size(parent->d_inode,
					d_inode(lower_parent_dentry));
	}

	fput(lower_bk_file);
out_lower_path:
	path_put(&lower_bk_path);

out_bk_dentry:
	dput(bk_dentry);

out_free_dentry:
	dput(dentry);
	dput(parent);
	return err;
}

/*
 * This function populates the user string of struct xstr passed through p
 * with either "version start_count to end_count" or
 * "No backup exists".
 *
 * Returns 0 on success, else error.
 */
int bkpfs_list_versions(struct file *file, void __user *p)
{
	int err = 0, i = 0, len_firstv, len_lastv, len;
	struct xstr *x_str;
	struct version vr;
	struct file *lower_file;
	char *firstv, *lastv;

	lower_file = bkpfs_lower_file(file);

	err = bkpfs_get_version_xattr(lower_file->f_path.dentry, &vr);
	if (err < 0)
		return err;

	if (unlikely(!access_ok(VERIFY_WRITE, p, sizeof(struct xstr))))
		return -EFAULT;
	if (unlikely(!access_ok(VERIFY_WRITE, p, sizeof(struct xstr) +
				((struct xstr *)p)->len)))
		return -EFAULT;

	x_str = kmalloc(sizeof(struct xstr) + ((struct xstr *)p)->len,
			GFP_KERNEL);
	if (!x_str)
		return -ENOMEM;

	/* If length passed is less than needed, error out with -EMSGSIZE */
	if (vr.count == 0) {
		len = 16;
		if (len > ((struct xstr *)p)->len + 1) {
			err = -EMSGSIZE;
			goto out_free_list_xstr;
		}
		/* populates user buffer as "No backup exist" */
		memcpy(x_str->str, "No backup exist", 16);
	} else if (vr.count == 1) {
		len_firstv = get_string(&firstv, vr.first);
		len = 10 + len_firstv;
		if (len > ((struct xstr *)p)->len + 1) {
			err = -EMSGSIZE;
			goto out_free_list_xstr;
		}
		/* populates user buffer as "version: version_num */
		memcpy(x_str->str, "version: ", 9);
		i = i + 9;
		memcpy(x_str->str + i, firstv, len_firstv + 1);
		kfree(firstv);
	} else {
		len_firstv = get_string(&firstv, vr.first);
		len_lastv = get_string(&lastv, vr.last);
		len = 14 + len_firstv + len_lastv;
		if (len > ((struct xstr *)p)->len + 1) {
			err = -EMSGSIZE;
			goto out_free_list_xstr;
		}
		/* populates user buffer as "version: first to version last */
		memcpy(x_str->str, "version: ", 9);
		i  = i + 9;
		memcpy(x_str->str + i, firstv, len_firstv);
		i = i + len_firstv;
		memcpy(x_str->str + i, " to ", 4);
		i = i + 4;
		memcpy(x_str->str + i, lastv, len_lastv + 1);
		kfree(firstv);
		kfree(lastv);
	}

	if (copy_to_user(((struct xstr *)p)->str, x_str->str, len)) {
		err = -EINVAL;
		goto out_free_list_xstr;
	}

out_free_list_xstr:
	kfree(x_str);
	return err;
}

/*
 * This function finds the dentry of backup file with version V passed
 * and then deletes that file and updates the extended attributes of
 * that file.
 *
 * Returns 0 on success, else err
 */
int bkpfs_unlink_version(struct dentry *dentry, int V)
{
	int err = 0;
	struct dentry *bk_dentry, *parent, *lower_bk_dentry, *lower_dir_dentry;
	struct inode *lower_dir_inode;
	struct path lower_bk_path;

	parent = dget_parent(dentry);

	/* get the backup dentry */
	bk_dentry = bkpfs_get_bkdentry(dentry, V);
	if (IS_ERR(bk_dentry)) {
		err = PTR_ERR(bk_dentry);
		goto out_free_bk_dentry;
	}

	lower_dir_inode = bkpfs_lower_inode(parent->d_inode);
	bkpfs_get_lower_path(bk_dentry, &lower_bk_path);
	lower_bk_dentry = lower_bk_path.dentry;
	dget(lower_bk_dentry);
	lower_dir_dentry = lock_parent(lower_bk_dentry);

	/* delete the backup dentry */
	err = vfs_unlink(lower_dir_inode, lower_bk_dentry, NULL);
	if (err == -EBUSY && lower_bk_dentry->d_flags & DCACHE_NFSFS_RENAMED)
		err = 0;
	if (err)
		goto out_free_lock;

	fsstack_copy_attr_times(parent->d_inode, lower_dir_inode);
	fsstack_copy_inode_size(parent->d_inode, lower_dir_inode);
	set_nlink(d_inode(bk_dentry),
		  bkpfs_lower_inode(d_inode(bk_dentry))->i_nlink);
	d_inode(bk_dentry)->i_ctime = parent->d_inode->i_ctime;
	d_drop(bk_dentry);
out_free_lock:
	unlock_dir(lower_dir_dentry);
	dput(lower_bk_dentry);
	bkpfs_put_lower_path(bk_dentry, &lower_bk_path);
out_free_bk_dentry:
	dput(bk_dentry);
	dput(parent);
	return err;
}

/*
 * This function gets the argument from the user string passed through
 * struct xstr. On the basis of that argument it calls bkpfs_unlink_version
 * to delete that version.
 *
 * Returns 0 on success, else error.
 */
int bkpfs_delete_version(struct file *file, void __user *p)
{
	int err = 0;
	struct file *lower_file;
	struct version vr;
	struct xstr *x_str = NULL;
	struct dentry *dentry;

	dentry = file->f_path.dentry;
	dget(dentry);
	lower_file = bkpfs_lower_file(file);

	err = bkpfs_get_version_xattr(lower_file->f_path.dentry, &vr);
	if (err < 0)
		goto out;

	if (vr.count == 0) {
		err = -ENODATA;
		goto out;
	}

	err = get_xstr(p, file, &x_str);
	if (err < 0)
		return err;

	/*
	 * set version number from the string passed as NEW/OLD/ALL
	 * and unlink that version of backup accordingly
	 */
	if (strcmp(x_str->str, "NEW") == 0) {
		err = bkpfs_unlink_version(dentry, vr.last);
		if (err < 0)
			goto out;
		vr.last--;
		vr.count--;
	} else if (strcmp(x_str->str, "OLD") == 0) {
		err = bkpfs_unlink_version(dentry, vr.first);
		vr.count--;
		vr.first++;
		if (err < 0)
			goto out;
	} else if (strcmp(x_str->str, "ALL") == 0) {
		while (vr.count) {
			err = bkpfs_unlink_version(dentry, vr.first);
			vr.count--;
			vr.first++;
			if (err < 0)
				goto out;
		}
		vr.first = 0;
		vr.last = 0;
	} else {
		err = -ENODATA;
		goto out;
	}

	if (vr.count == 0) {
		vr.first = 0;
		vr.last = 0;
	}

	err = bkpfs_set_version_xattr(lower_file->f_path.dentry, &vr);
out:
	dput(dentry);
	kfree(x_str);
	return err;
}

/*
 * This function restore the version passed by copying the data
 * from backup file of that version to original file.
 *
 * Returns 0 on success, else error
 */
int bkpfs_restore_version(struct file *file, void __user  *p)
{
	int err = 0, version_num;
	struct file *lower_file, *lower_bk_file;
	struct dentry *bk_dentry, *dentry;
	struct path lower_bk_path;
	struct version vr;
	struct xstr *x_str = NULL;
	struct iattr attr;

	err = get_xstr(p, file, &x_str);
	if (err < 0)
		return err;

	lower_file = bkpfs_lower_file(file);
	dentry = file->f_path.dentry;
	dget(dentry);

	err = bkpfs_get_version_xattr(lower_file->f_path.dentry, &vr);
	if (err < 0)
		goto out_restore_dentry;

	/* set version number from the string passed as NEW/OLD/VERSION_NO */
	if (strcmp("NEW", x_str->str) == 0)
		version_num = vr.last;
	else if (strcmp("OLD", x_str->str) == 0)
		version_num = vr.first;
	else {
		err = kstrtoint(x_str->str, 10, &version_num);
		if (err < 0)
			goto out_restore_dentry;
	}

	if (version_num > vr.last || version_num < vr.first ||
	    vr.count == 0) {
		err = -ENODATA;
		goto out_restore_dentry;
	}

	/* get the dentry of backup file to restore */
	bk_dentry = bkpfs_get_bkdentry(dentry, version_num);
	if (IS_ERR(bk_dentry)) {
		err = PTR_ERR(bk_dentry);
		goto out_restore_bk_dentry;
	}

	/* open the backup file through dentry_open */
	bkpfs_get_lower_path(bk_dentry, &lower_bk_path);
	lower_bk_file = dentry_open(&lower_bk_path, file->f_flags,
				    current_cred());
	path_put(&lower_bk_path);

	if (IS_ERR(lower_bk_file)) {
		err = PTR_ERR(lower_bk_file);
		goto out_restore_bk_dentry;
	}

	/*
	 * copy the data from original file to backup file and
	 * update the attr of both upper and lower inodes
	 */
	err = bkpfs_copy_data(lower_bk_file, lower_file);
	if (err == 0) {
		attr.ia_valid = ATTR_SIZE;
		attr.ia_size = file_inode(lower_bk_file)->i_size;

		inode_lock(file_inode(lower_file));
		notify_change(lower_file->f_path.dentry, &attr, NULL);
		inode_unlock(file_inode(lower_file));

		fsstack_copy_attr_all(file_inode(file),
				      file_inode(lower_file));
		fsstack_copy_inode_size(file_inode(file),
					file_inode(lower_file));
	}

	fput(lower_bk_file);
out_restore_bk_dentry:
	dput(bk_dentry);
out_restore_dentry:
	dput(dentry);
	kfree(x_str);
	return err;
}

/*
 * This function opens the backup file with version passed
 * through argument by user.
 *
 * Returns fd on succes, else err.
 */
int bkpfs_open_version(struct file *file, void __user *p)
{
	int fd, err = 0, version_num;
	struct dentry *bk_dentry, *dentry;
	struct file *lower_bk_file, *lower_file;
	struct path lower_bk_path;
	struct xstr *x_str;
	struct version vr;

	err = get_xstr(p, file, &x_str);
	if (err < 0)
		return err;

	lower_file = bkpfs_lower_file(file);

	err = bkpfs_get_version_xattr(lower_file->f_path.dentry, &vr);
	if (err < 0)
		return err;

	/* set version number from the string passed as NEW/OLD/VERSION_NO */
	if (strcmp("NEW", x_str->str) == 0) {
		version_num = vr.last;
	} else if (strcmp("OLD", x_str->str) == 0) {
		version_num = vr.first;
	} else {
		err = kstrtoint(x_str->str, 10, &version_num);
		if (err < 0)
			return err;
	}

	if (version_num < vr.first || version_num > vr.last || vr.count == 0)
		return -EINVAL;

	dentry = file->f_path.dentry;
	dget(dentry);

	/* get the dentry of the backup file to open */
	bk_dentry = bkpfs_get_bkdentry(dentry, version_num);
	if (IS_ERR(bk_dentry)) {
		err = PTR_ERR(bk_dentry);
		goto out_open_bk_dentry;
	}

	/* open the backup file */
	bkpfs_get_lower_path(bk_dentry, &lower_bk_path);
	lower_bk_file = dentry_open(&lower_bk_path, file->f_flags,
				    current_cred());
	path_put(&lower_bk_path);

	if (IS_ERR(lower_bk_file)) {
		err = PTR_ERR(lower_bk_file);
		lower_bk_file = bkpfs_lower_file(file);
		if (lower_bk_file)
			bkpfs_set_lower_file(file, NULL);
		goto out_open_bk_dentry;
	}

	/* get the unused fd for the opened backup file and return that */
	fd = get_unused_fd_flags(file->f_flags);
	if (fd >= 0) {
		fsnotify_open(lower_bk_file);
		fd_install(fd, lower_bk_file);
	}

out_open_bk_dentry:
	dput(bk_dentry);
	dput(dentry);
	if (err < 0)
		return err;
	return fd;
}

/*
 * This function clears the fd and closes the file opened
 *
 * Returns 0 on success, else error
 */
int bkpfs_close_version(struct file *file, void __user *p)
{
	int fd = *((int *)p);
	int retval = __close_fd(current->files, fd);

	if (unlikely(retval == -ERESTARTSYS ||
		     retval == -ERESTARTNOINTR ||
		     retval == -ERESTARTNOHAND ||
		     retval == -ERESTART_RESTARTBLOCK))
		retval = -EINTR;

	return retval;
}
