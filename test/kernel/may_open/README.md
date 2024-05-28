# may_open

## Source Code

```c fs/namei.c
static int may_open(struct mnt_idmap *idmap, const struct path *path,
      int acc_mode, int flag)
{
 struct dentry *dentry = path->dentry;
 struct inode *inode = dentry->d_inode;
 int error;

 if (!inode)
  return -ENOENT;

 switch (inode->i_mode & S_IFMT) {
 case S_IFLNK:
  return -ELOOP;
 case S_IFDIR:
  if (acc_mode & MAY_WRITE)
   return -EISDIR;
  if (acc_mode & MAY_EXEC)
   return -EACCES;
  break;
 case S_IFBLK:
 case S_IFCHR:
  if (!may_open_dev(path))
   return -EACCES;
  fallthrough;
 case S_IFIFO:
 case S_IFSOCK:
  if (acc_mode & MAY_EXEC)
   return -EACCES;
  flag &= ~O_TRUNC;
  break;
 case S_IFREG:
  if ((acc_mode & MAY_EXEC) && path_noexec(path))
   return -EACCES;
  break;
 }

 error = inode_permission(idmap, inode, MAY_OPEN | acc_mode);
 if (error)
  return error;

 /*
  * An append-only file must be opened in append mode for writing.
  */
 if (IS_APPEND(inode)) {
  if  ((flag & O_ACCMODE) != O_RDONLY && !(flag & O_APPEND))
   return -EPERM;
  if (flag & O_TRUNC)
   return -EPERM;
 }

 /* O_NOATIME can only be set by the owner or superuser */
 if (flag & O_NOATIME && !inode_owner_or_capable(idmap, inode))
  return -EPERM;

 return 0;
}
```
