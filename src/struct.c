#include "../inc/struct.h"

#include "../inc/stdafx.h"
#include "../inc/var.h"

#include <stdlib.h>
#include <string.h>

int
e_struct_init_from(const e_struct* from, e_struct* s)
{
  e_var*       members = calloc(from->member_count, sizeof(e_var));
  const char** names   = (const char**)calloc(from->member_count, sizeof(char*));
  u32*         hashes  = calloc(from->member_count, sizeof(u32));

  for (u32 i = 0; i < from->member_count; i++) {
    e_var_deep_cpy(&from->members[i], &members[i]);
    hashes[i] = from->member_hashes[i];
    names[i]  = from->member_names[i];
  }

  s->members       = members;
  s->member_hashes = hashes;
  s->member_names  = names;
  s->member_count  = from->member_count;

  return 0;
}

int
e_struct_init(const char* name, u32 nmembers, const char** member_names, e_struct* s)
{
  s->name         = name;
  s->member_count = nmembers;
  s->member_names = (const char**)e_xalloc(nmembers, sizeof(const char*));
  if (!s->member_names) return -1;

  s->member_hashes = (u32*)e_xalloc(nmembers, sizeof(u32));
  if (!s->member_hashes) {
    free((void*)s->member_names);
    return -1;
  }

  s->members = (e_var*)e_xalloc(nmembers, sizeof(e_var));
  if (!s->members) {
    free((void*)s->member_names);
    free((void*)s->members);
    return -1;
  }

  for (u32 i = 0; i < nmembers; i++) {
    s->member_names[i]  = member_names[i];
    s->member_hashes[i] = e_hash(member_names[i], strlen(member_names[i]));
  }

  return 0;
}

int
e_struct_init_paired(const char* name, u32 nmembers, const char** member_names, const struct e_struct_member_pair* pairs, e_struct* s)
{
  if (e_struct_init(name, nmembers, member_names, s)) return -1;

  int e = 0;
  for (u32 i = 0; i < nmembers; i++) {
    int xe = e_struct_set_member_by_name(s, pairs[i].name, &pairs[i].value);
    if (e == 0 && xe != 0) e = xe;
  }

  return e;
}

int
e_struct_set_member_by_name(const e_struct* s, const char* name, const struct e_var* value)
{
  u32  hash  = e_hash(name, strlen(name));
  u32  index = 0;
  bool found = false;
  for (u32 i = 0; i < s->member_count; i++) {
    if (s->member_hashes[i] == hash) {
      index = i;
      found = true;
      break;
    }
  }

  if (!found) return -1;

  return e_struct_set_member_by_index(s, index, value);
}

int
e_struct_set_member_by_index(const e_struct* s, u32 index, const struct e_var* value)
{
  if (index >= s->member_count) return -1;
  return e_var_shallow_cpy(value, &s->members[index]);
}

int
e_struct_set_member_pairs(const e_struct* s, u32 npairs, const struct e_struct_member_pair* pairs)
{
  int e = 0;
  for (u32 i = 0; i < npairs; i++) {
    int xe = e_struct_set_member_by_name(s, pairs[i].name, &pairs[i].value);
    if (e == 0 && xe != 0) e = xe;
  }
  return e;
}
