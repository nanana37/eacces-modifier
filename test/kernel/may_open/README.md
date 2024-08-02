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

## OUTPUT

```sh
[  212.434800] --- Hello, I'm Permod ---
[  212.434803] [Permod] /home/hiron/git/linux/fs/namei.c: 3255
[  212.434818] [Permod] may_open() returned -13
[  212.434818] [Permod] {
[  212.434818] [Permod] {
[  212.434818] [Permod] {
[  212.434819] [Permod] {
[  212.434820] [Permod] inode != null
[  212.434823] [Permod] }
[  212.434834] [Permod] i_mode == 16384 (switch)
[  212.434834] [Permod] }
[  212.434839] [Permod] acc_mode & 2 == 0
[  212.434841] [Permod] }
[  212.434841] [Permod] acc_mode & 1 != 0
[  212.434841] [Permod] }
[  212.436252] --- Hello, I'm Permod ---
[  212.436260] [Permod] /home/hiron/git/linux/fs/namei.c: 3260
[  212.436260] [Permod] may_open() returned -13
[  212.436261] [Permod] {
[  212.436263] [Permod] {
[  212.436270] [Permod] {
[  212.436272] [Permod] inode != null
[  212.436272] [Permod] }
[  212.436272] [Permod] i_mode == 24576 (switch)
[  212.436279] [Permod] }
[  212.436280] [Permod] may_open_dev() returned 0
[  212.436283] [Permod] }
[  212.436642] --- Hello, I'm Permod ---
[  212.436652] [Permod] /home/hiron/git/linux/fs/namei.c: 3260
[  212.436652] [Permod] may_open() returned -13
[  212.436653] [Permod] {
[  212.436655] [Permod] {
[  212.436656] [Permod] {
[  212.436658] [Permod] inode != null
[  212.436660] [Permod] }
[  212.436662] [Permod] i_mode == 8192 (switch)
[  212.436665] [Permod] }
[  212.436666] [Permod] may_open_dev() returned 0
[  212.436669] [Permod] }
[  212.436791] --- Hello, I'm Permod ---
[  212.436791] [Permod] /home/hiron/git/linux/fs/namei.c: 3265
[  212.436791] [Permod] may_open() returned -13
[  212.436793] [Permod] {
[  212.436795] [Permod] {
[  212.436797] [Permod] {
[  212.436798] [Permod] {
[  212.436800] [Permod] i_mode == 4096 (switch)
[  212.436803] [Permod] }
[  212.436804] [Permod] may_open_dev() didn't return 0
[  212.436807] [Permod] }
[  212.436809] [Permod] }
[  212.436810] [Permod] {
[  212.436812] [Permod] {
[  212.436814] [Permod] inode != null
[  212.436816] [Permod] }
[  212.436817] [Permod] i_mode == 4096 (switch)
[  212.436820] [Permod] }
[  212.436831] [Permod] acc_mode & 1 != 0
[  212.436831] [Permod] }
[  212.436920] --- Hello, I'm Permod ---
[  212.436922] [Permod] /home/hiron/git/linux/fs/namei.c: 3265
[  212.436925] [Permod] may_open() returned -13
[  212.436928] [Permod] {
[  212.436929] [Permod] {
[  212.436931] [Permod] {
[  212.436933] [Permod] {
[  212.436934] [Permod] i_mode == 49152 (switch)
[  212.436937] [Permod] }
[  212.436939] [Permod] may_open_dev() didn't return 0
[  212.436941] [Permod] }
[  212.436943] [Permod] }
[  212.436945] [Permod] {
[  212.436946] [Permod] {
[  212.436948] [Permod] inode != null
[  212.436950] [Permod] }
[  212.436952] [Permod] i_mode == 49152 (switch)
[  212.436954] [Permod] }
[  212.436956] [Permod] acc_mode & 1 != 0
[  212.436958] [Permod] }
[  212.437215] --- Hello, I'm Permod ---
[  212.437219] [Permod] /home/hiron/git/linux/fs/namei.c: 3270
[  212.437222] [Permod] may_open() returned -13
[  212.437224] [Permod] {
[  212.437226] [Permod] {
[  212.437228] [Permod] {
[  212.437229] [Permod] {
[  212.437231] [Permod] inode != null
[  212.437233] [Permod] }
[  212.437235] [Permod] i_mode == 32768 (switch)
[  212.437237] [Permod] }
[  212.437239] [Permod] acc_mode & 1 != 0
[  212.437241] [Permod] }
[  212.437243] [Permod] path_noexec() didn't return 0
[  212.437245] [Permod] }
[  214.574193] --- Hello, I'm Permod ---
[  214.574200] [Permod] /home/hiron/git/linux/fs/exec.c: 932
[  214.574205] [Permod] do_open_execat() returned -13
[  214.574208] [Permod] {
[  214.574210] [Permod] {
[  214.574212] [Permod] flags & 4096 == 0
[  214.574215] [Permod] }
[  214.574216] [Permod] {
[  214.574218] [Permod] {
[  214.574220] [Permod] {
[  214.574221] [Permod] flags & 256 == 0
[  214.574224] [Permod] }
[  214.574225] [Permod] {
[  214.574227] [Permod] {
[  214.574229] [Permod] {
[  214.574230] [Permod] flags & -4353 == 0
[  214.574233] [Permod] }
[  214.574234] [Permod] flags & 256 != 0
[  214.574237] [Permod] }
[  214.574239] [Permod] }
[  214.574240] [Permod] flags & 4096 != 0
[  214.574243] [Permod] }
[  214.574244] [Permod] }
[  214.574246] [Permod] IS_ERR() returned 0
[  214.574249] [Permod] }
```
