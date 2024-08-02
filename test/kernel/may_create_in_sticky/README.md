# may_create_in_sticky

## Source Code

```c fs/namei.c
/**
 * may_create_in_sticky - Check whether an O_CREAT open in a sticky directory
 *     should be allowed, or not, on files that already
 *     exist.
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

## Description

Cannot O_CREAT open a FIFO (or a regular file) in a sticky directory when:

- `sysctl_protected_fifos` or `sysctl_protected_regular` is enabled
- the file is owned by other user
- *this should happens even for root (but not actually printed in my case)

NOTE: Disabling `sysctl_protected_fifos` or `sysctl_protected_regular` will also
      cause EACCES somewhere else. CHECK IT OUT!

```bash
$ make clean
$ make init #sh init.sh
$ su <other user>
$ make #sh main.sh
Permission denied
```

## NOT WORK BELOW

## sticky bit

sticky directory

```bash
chmod 1777 <dir>
```

## Traceout

Assumption:
sticky bit will prevent other usr's action in a sticky directory.

```bash
make

sh scripts/trace.sh <owner/otheruser>
```

need to cmp trace-as-owner.list vs trace-as-other.list

## OUTPUT

```sh
[  832.414203] --- Hello, I'm Permod ---
[  832.414203] [Permod] /home/hiron/git/linux/fs/namei.c: 1265
[  832.414213] [Permod] may_create_in_sticky() returned -13
[  832.414217] [Permod] {
[  832.414217] [Permod] llvm.expect.i64 != 0
[  832.414222] [Permod] }
[  832.414224] [Permod] {
[  832.414224] [Permod] i_mode52 & 61440 == 0
[  832.414229] [Permod] }
[  832.414231] [Permod] {
[  832.414231] [Permod] {
[  832.414233] [Permod] {
[  832.414235] [Permod] sysctl_protected_fifos < 2
[  832.414237] [Permod] }
[  832.414240] [Permod] {
[  832.414240] [Permod] {
[  832.414242] [Permod] {
[  832.414244] [Permod] {
[  832.414246] [Permod] {
[  832.414246] [Permod] {
[  832.414248] [Permod] {
[  832.414250] [Permod] {
[  832.414252] [Permod] {
[  832.414255] [Permod] {
[  832.414255] [Permod] {
[  832.414257] [Permod] sysctl_protected_regular != 0
[  832.414260] [Permod] }
[  832.414260] [Permod] {
[  832.414262] [Permod] {
[  832.414264] [Permod] {
[  832.414266] [Permod] sysctl_protected_fifos != 0
[  832.414268] [Permod] }
[  832.414270] [Permod] {
[  832.414272] [Permod] {
[  832.414272] [Permod] sysctl_protected_fifos == 0
[  832.414277] [Permod] }
[  832.414278] [Permod] i_mode & 61440 != 0
[  832.414280] [Permod] }
[  832.414282] [Permod] sysctl_protected_regular == 0
[  832.414285] [Permod] }
[  832.414285] [Permod] i_mode6 & 61440 != 0
[  832.414289] [Permod] }
[  832.414291] [Permod] llvm.expect.i64 == 0
[  832.414293] [Permod] }
[  832.414293] [Permod] vfsuid_eq() returned 0
[  832.414297] [Permod] }
[  832.414299] [Permod] }
[  832.414299] [Permod] }
[  832.414301] [Permod] }
[  832.414303] [Permod] vfsuid_eq_kuid() returned 0
[  832.414307] [Permod] }
[  832.414308] [Permod] llvm.expect.i64 == 0
[  832.414309] [Permod] }
[  832.414311] [Permod] dir_mode & 16 != 0
[  832.414314] [Permod] }
[  832.414315] [Permod] sysctl_protected_fifos >= 2
[  832.414317] [Permod] }
[  832.414319] [Permod] i_mode52 & 61440 != 0
[  832.414322] [Permod] }
[  832.414323] [Permod] sysctl_protected_regular >= 2
[  832.414325] [Permod] }
[  832.414327] [Permod] i_mode61 & 61440 == 0
[  832.414329] [Permod] }
[  832.415033] --- Hello, I'm Permod ---
[  832.415038] [Permod] /home/hiron/git/linux/fs/exec.c: 932
[  832.415041] [Permod] do_open_execat() returned -13
[  832.415147] [Permod] do_open_execat() returned -13
[  832.416799] --- Hello, I'm Permod ---
[  832.416804] [Permod] /home/hiron/git/linux/fs/namei.c: 1265
[  832.416809] [Permod] may_create_in_sticky() returned -13
[  832.416813] [Permod] {
[  832.416815] [Permod] llvm.expect.i64 != 0
[  832.416819] [Permod] }
[  832.416821] [Permod] {
[  832.416822] [Permod] i_mode52 & 61440 == 0
[  832.416825] [Permod] }
[  832.416827] [Permod] {
[  832.416828] [Permod] {
[  832.416830] [Permod] {
[  832.416832] [Permod] sysctl_protected_fifos < 2
[  832.416834] [Permod] }
[  832.416836] [Permod] {
[  832.416838] [Permod] {
[  832.416839] [Permod] {
[  832.416841] [Permod] {
[  832.416843] [Permod] {
[  832.416844] [Permod] {
[  832.416846] [Permod] {
[  832.416848] [Permod] {
[  832.416849] [Permod] {
[  832.416851] [Permod] {
[  832.416853] [Permod] {
[  832.416854] [Permod] sysctl_protected_regular != 0
[  832.416857] [Permod] }
[  832.416858] [Permod] {
[  832.416860] [Permod] {
[  832.416862] [Permod] {
[  832.416863] [Permod] sysctl_protected_fifos != 0
[  832.416866] [Permod] }
[  832.416867] [Permod] {
[  832.416869] [Permod] {
[  832.416871] [Permod] sysctl_protected_fifos == 0
[  832.416873] [Permod] }
[  832.416875] [Permod] i_mode & 61440 != 0
[  832.416877] [Permod] }
[  832.416879] [Permod] sysctl_protected_regular == 0
[  832.416881] [Permod] }
[  832.416883] [Permod] i_mode6 & 61440 != 0
[  832.416885] [Permod] }
[  832.416887] [Permod] llvm.expect.i64 == 0
[  832.416889] [Permod] }
[  832.416891] [Permod] vfsuid_eq() returned 0
[  832.416894] [Permod] }
[  832.416895] [Permod] }
[  832.416897] [Permod] }
[  832.416899] [Permod] }
[  832.416900] [Permod] vfsuid_eq_kuid() returned 0
[  832.416903] [Permod] }
[  832.416904] [Permod] llvm.expect.i64 == 0
[  832.416907] [Permod] }
[  832.416908] [Permod] dir_mode & 16 != 0
[  832.416911] [Permod] }
[  832.416913] [Permod] sysctl_protected_fifos >= 2
[  832.416915] [Permod] }
[  832.416917] [Permod] i_mode52 & 61440 != 0
[  832.416919] [Permod] }
[  832.416921] [Permod] sysctl_protected_regular >= 2
[  832.416923] [Permod] }
[  832.416925] [Permod] i_mode61 & 61440 == 0
[  832.416927] [Permod] }
```
