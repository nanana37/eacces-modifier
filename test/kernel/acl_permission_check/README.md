```c
/**
 * acl_permission_check - perform basic UNIX permission checking
 * @idmap:	idmap of the mount the inode was found from
 * @inode:	inode to check permissions on
 * @mask:	right to check for (%MAY_READ, %MAY_WRITE, %MAY_EXEC ...)
 *
 * This function performs the basic UNIX permission checking. Since this
 * function may retrieve POSIX acls it needs to know whether it is called from a
 * blocking or non-blocking context and thus cares about the MAY_NOT_BLOCK bit.
 *
 * If the inode has been found through an idmapped mount the idmap of
 * the vfsmount must be passed through @idmap. This function will then take
 * care to map the inode according to @idmap before checking permissions.
 * On non-idmapped mounts or if permission checking is to be performed on the
 * raw inode simply passs @nop_mnt_idmap.
 */
static int acl_permission_check(struct mnt_idmap *idmap,
				struct inode *inode, int mask)
{
	unsigned int mode = inode->i_mode;
	vfsuid_t vfsuid;

	/* Are we the owner? If so, ACL's don't matter */
	vfsuid = i_uid_into_vfsuid(idmap, inode);
	if (likely(vfsuid_eq_kuid(vfsuid, current_fsuid()))) {
		mask &= 7;
		mode >>= 6;
		return (mask & ~mode) ? -EACCES : 0;
	}

	/* Do we have ACL's? */
	if (IS_POSIXACL(inode) && (mode & S_IRWXG)) {
		int error = check_acl(idmap, inode, mask);
		if (error != -EAGAIN)
			return error;
	}

	/* Only RWX matters for group/other mode bits */
	mask &= 7;

	/*
	 * Are the group permissions different from
	 * the other permissions in the bits we care
	 * about? Need to check group ownership if so.
	 */
	if (mask & (mode ^ (mode >> 3))) {
		vfsgid_t vfsgid = i_gid_into_vfsgid(idmap, inode);
		if (vfsgid_in_group_p(vfsgid))
			mode >>= 3;
	}

	/* Bits in 'mode' clear that we require? */
	return (mask & ~mode) ? -EACCES : 0;
}
```
