# may_follow_link

## Source Code

```c fs/namei.c
/**
 * may_follow_link - Check symlink following for unsafe situations
 * @nd: nameidata pathwalk data
 *
 * In the case of the sysctl_protected_symlinks sysctl being enabled,
 * CAP_DAC_OVERRIDE needs to be specifically ignored if the symlink is
 * in a sticky world-writable directory. This is to protect privileged
 * processes from failing races against path names that may change out
 * from under them by way of other users creating malicious symlinks.
 * It will permit symlinks to be followed only when outside a sticky
 * world-writable directory, or when the uid of the symlink and follower
 * match, or when the directory owner matches the symlink's owner.
 *
 * Returns 0 if following the symlink is allowed, -ve on error.
 */
static inline int may_follow_link(struct nameidata *nd,
                                  const struct inode *inode) {
    struct mnt_idmap *idmap;
    vfsuid_t vfsuid;

    if (!sysctl_protected_symlinks)
        return 0;

    idmap = mnt_idmap(nd->path.mnt);
    vfsuid = i_uid_into_vfsuid(idmap, inode);
    /* Allowed if owner and follower match. */
    if (vfsuid_eq_kuid(vfsuid, current_fsuid()))
        return 0;

    /* Allowed if parent directory not sticky and world-writable. */
    if ((nd->dir_mode & (S_ISVTX | S_IWOTH)) != (S_ISVTX | S_IWOTH))
        return 0;

    /* Allowed if parent directory and link owner match. */
    if (vfsuid_valid(nd->dir_vfsuid) && vfsuid_eq(nd->dir_vfsuid, vfsuid))
        return 0;

    if (nd->flags & LOOKUP_RCU)
        return -ECHILD;

    audit_inode(nd->name, nd->stack[0].link.dentry, 0);
    audit_log_path_denied(AUDIT_ANOM_LINK, "follow_link");
    return -EACCES; //1083
}
```

## Description

Cannot go through symlink which is:

- under sticky & world-wide dir(e.g., /tmp).
- made by the user who does not own the directory
- *this happens even if you are root

```bash
$ make clean
$ make init #sh init.sh
$ su <other user>
$ make #sh main.sh
Permission denied
```

[thanks to: Qiita@kusano_k](https://qiita.com/kusano_k/items/8763374d0dc3edc927cf)

## OUTPUT

```sh
[  676.729323] --- Hello, I'm Permod ---
[  676.729327] [Permod] /home/hiron/git/linux/fs/namei.c: 1135
[  676.729331] [Permod] may_follow_link() returned -13
[  676.729334] [Permod] {
[  676.729335] [Permod] {
[  676.729337] [Permod] vfsuid_valid() returned 0
[  676.729340] [Permod] }
[  676.729341] [Permod] {
[  676.729343] [Permod] {
[  676.729345] [Permod] {
[  676.729346] [Permod] {
[  676.729348] [Permod] {
[  676.729349] [Permod] {
[  676.729351] [Permod] {
[  676.729353] [Permod] {
[  676.729355] [Permod] sysctl_protected_symlinks != 0
[  676.729357] [Permod] }
[  676.729359] [Permod] }
[  676.729360] [Permod] }
[  676.729362] [Permod] }
[  676.729364] [Permod] vfsuid_eq_kuid() returned 0
[  676.729366] [Permod] }
[  676.729368] [Permod] dir_mode & 514 == 0
[  676.729370] [Permod] }
[  676.729372] [Permod] vfsuid_valid() didn't return 0
[  676.729375] [Permod] }
[  676.729376] [Permod] vfsuid_eq() returned 0
[  676.729379] [Permod] }
[  676.729380] [Permod] flags & 64 == 0
[  676.729383] [Permod] }
```
