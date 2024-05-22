```c
/**
 * may_create_in_sticky - Check whether an O_CREAT open in a sticky directory
 *			  should be allowed, or not, on files that already
 *			  exist.
 * @idmap: idmap of the mount the inode was found from
 * @nd: nameidata pathwalk data
 * @inode: the inode of the file to open
 *
 * Block an O_CREAT open of a FIFO (or a regular file) when:
 *   - sysctl_protected_fifos (or sysctl_protected_regular) is enabled
 *   - the file already exists
 *   - we are in a sticky directory
 *   - we don't own the file
 *   - the owner of the directory doesn't own the file
 *   - the directory is world writable
 * If the sysctl_protected_fifos (or sysctl_protected_regular) is set to 2
 * the directory doesn't have to be world writable: being group writable will
 * be enough.
 *
 * If the inode has been found through an idmapped mount the idmap of
 * the vfsmount must be passed through @idmap. This function will then take
 * care to map the inode according to @idmap before checking permissions.
 * On non-idmapped mounts or if permission checking is to be performed on the
 * raw inode simply pass @nop_mnt_idmap.
 *
 * Returns 0 if the open is allowed, -ve on error.
 */
static int may_create_in_sticky(struct mnt_idmap *idmap, struct nameidata *nd,
                                struct inode *const inode) {
    umode_t dir_mode = nd->dir_mode;
    vfsuid_t dir_vfsuid = nd->dir_vfsuid;

    if ((!sysctl_protected_fifos && S_ISFIFO(inode->i_mode)) ||
        (!sysctl_protected_regular && S_ISREG(inode->i_mode)) ||
        likely(!(dir_mode & S_ISVTX)) ||
        vfsuid_eq(i_uid_into_vfsuid(idmap, inode), dir_vfsuid) ||
        vfsuid_eq_kuid(i_uid_into_vfsuid(idmap, inode), current_fsuid()))
        return 0;

    if (likely(dir_mode & 0002) ||
        (dir_mode & 0020 &&
         ((sysctl_protected_fifos >= 2 && S_ISFIFO(inode->i_mode)) ||
          (sysctl_protected_regular >= 2 && S_ISREG(inode->i_mode))))) {
        const char *operation = S_ISFIFO(inode->i_mode)
                                    ? "sticky_create_fifo"
                                    : "sticky_create_regular";
        audit_log_path_denied(AUDIT_ANOM_CREAT, operation);
        return -EACCES;
    }
    return 0;
}
```

# Description

Cannot O_CREAT open a FIFO (or a regular file) in a sticky directory when:
- `sysctl_protected_fifos` or `sysctl_protected_regular` is enabled
- the file is owned by other user
* this should happens even for root (but not actually printed in my case)
* NOTE: Disabling `sysctl_protected_fifos` or `sysctl_protected_regular` will also cause EACCES somewhere else. CHECK IT OUT!

```bash
$ make clean
$ make init #sh init.sh
$ su <other user>
$ make #sh main.sh
Permission denied
```


# NOT WORK BELOW
## sticky bit

sticky directory

```
chmod 1777 <dir>
```


## Traceout

Assumption:
sticky bit will prevent other usr's action in a sticky directory.

```
make

sh scripts/trace.sh <owner/otheruser>
```

need to cmp trace-as-owner.list vs trace-as-other.list
