# profile_transition

## Build Error

See [error log](./build-error.log)

## Source code

```c security/apparmor/domain.c
static struct aa_label *profile_transition(struct aa_profile *profile,
        const struct linux_binprm *bprm,
        char *buffer, struct path_cond *cond,
        bool *secure_exec)
{
 struct aa_ruleset *rules = list_first_entry(&profile->rules,
          typeof(*rules), list);
 struct aa_label *new = NULL;
 const char *info = NULL, *name = NULL, *target = NULL;
 aa_state_t state = rules->file.start[AA_CLASS_FILE];
 struct aa_perms perms = {};
 bool nonewprivs = false;
 int error = 0;

 AA_BUG(!profile);
 AA_BUG(!bprm);
 AA_BUG(!buffer);

 error = aa_path_name(&bprm->file->f_path, profile->path_flags, buffer,
        &name, &info, profile->disconnected);
 if (error) {
  if (profile_unconfined(profile) ||
      (profile->label.flags & FLAG_IX_ON_NAME_ERROR)) {
   AA_DEBUG("name lookup ix on error");
   error = 0;
   new = aa_get_newest_label(&profile->label);
  }
  name = bprm->filename;
  goto audit;
 }

 if (profile_unconfined(profile)) {
  new = find_attach(bprm, profile->ns,
      &profile->ns->base.profiles, name, &info);
  if (new) {
   AA_DEBUG("unconfined attached to new label");
   return new;
  }
  AA_DEBUG("unconfined exec no attachment");
  return aa_get_newest_label(&profile->label);
 }

 /* find exec permissions for name */
 state = aa_str_perms(&(rules->file), state, name, cond, &perms);
 if (perms.allow & MAY_EXEC) {
  /* exec permission determine how to transition */
  new = x_to_label(profile, bprm, name, perms.xindex, &target,
     &info);
  if (new && new->proxy == profile->label.proxy && info) {
   /* hack ix fallback - improve how this is detected */
   goto audit;
  } else if (!new) {
   error = -EACCES;
   info = "profile transition not found";
   /* remove MAY_EXEC to audit as failure */
   perms.allow &= ~MAY_EXEC;
  }
 } else if (COMPLAIN_MODE(profile)) {
  /* no exec permission - learning mode */
  struct aa_profile *new_profile = NULL;

  new_profile = aa_new_learning_profile(profile, false, name,
            GFP_KERNEL);
  if (!new_profile) {
   error = -ENOMEM;
   info = "could not create null profile";
  } else {
   error = -EACCES;
   new = &new_profile->label;
  }
  perms.xindex |= AA_X_UNSAFE;
 } else
  /* fail exec */
  error = -EACCES;

 if (!new)
  goto audit;


 if (!(perms.xindex & AA_X_UNSAFE)) {
  if (DEBUG_ON) {
   dbg_printk("apparmor: scrubbing environment variables"
       " for %s profile=", name);
   aa_label_printk(new, GFP_KERNEL);
   dbg_printk("\n");
  }
  *secure_exec = true;
 }

audit:
 aa_audit_file(profile, &perms, OP_EXEC, MAY_EXEC, name, target, new,
        cond->uid, info, error);
 if (!new || nonewprivs) {
  aa_put_label(new);
  return ERR_PTR(error);
 }

 return new;
}

```
