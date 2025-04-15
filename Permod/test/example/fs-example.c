#include <stdio.h>

#define EAGAIN 11
#define EACCES 13

int check_acl(int x) {
  if (x == 0) {
    return -EAGAIN;
  }
  if (x == 1) {
    return -EACCES;
  }
  return 0;
}

int func(int x) {
  int error = check_acl(x);
  if (error != -EAGAIN) {
    return error;
  }
  int y = check_acl(x);
  if (y == 0) {
    y = 1;
  }
  return 0;
}

int funcX(int y) {
  int error = 1;
  error = check_acl(y);
  if (error != -EAGAIN) {
    return error;
  }
  return 0;
}

int main() {
  for (int i = 0; i < 2; i++) {
    int error = func(i);
    error = funcX(i);
    if (error == -EAGAIN) {
      printf("Error: EAGAIN\n");
    } else if (error == -EACCES) {
      printf("Error: EACCES\n");
    } else {
      printf("No error\n");
    }
  }

  return 0;
}

#ifdef HOGE
static int acl_permission_check(struct mnt_idmap *idmap, struct inode *inode,
                                int mask) {
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
#endif