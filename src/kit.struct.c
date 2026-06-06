#include "../inc/kit.struct.h"

#include "../inc/kit.stdafx.h"
#include "../inc/kit.var.h"

#include <stdlib.h>
#include <string.h>

int
kit_struct_init_from(const kit_struct* from, kit_struct* s)
{
  kit_var*     members = calloc(from->member_count, sizeof(kit_var));
  const char** names   = (const char**)calloc(from->member_count, sizeof(char*));
  u32*         hashes  = calloc(from->member_count, sizeof(u32));

  for (u32 i = 0; i < from->member_count; i++) {
    kit_var_deep_cpy(&from->members[i], &members[i]);
    hashes[i] = from->member_hashes[i];
    names[i]  = from->member_names[i];
  }

  s->name          = from->name;
  s->members       = members;
  s->member_hashes = hashes;
  s->member_names  = names;
  s->member_count  = from->member_count;

  return 0;
}

int
kit_struct_init(const char* name, u32 nmembers, const char** member_names, kit_struct* s)
{
  s->name         = name;
  s->member_count = nmembers;
  s->member_names = (const char**)kit_xalloc(nmembers, sizeof(const char*));
  if (!s->member_names) return -1;

  s->member_hashes = (u32*)kit_xalloc(nmembers, sizeof(u32));
  if (!s->member_hashes) {
    free((void*)s->member_names);
    return -1;
  }

  s->members = (kit_var*)kit_xalloc(nmembers, sizeof(kit_var));
  if (!s->members) {
    free((void*)s->member_names);
    free((void*)s->member_hashes);
    return -1;
  }

  for (u32 i = 0; i < nmembers; i++) {
    s->member_names[i]  = member_names[i];
    s->member_hashes[i] = kit_hash(member_names[i], strlen(member_names[i]));
  }

  return 0;
}

int
kit_struct_init_paired(const char* name, u32 nmembers, const char** member_names, const struct kit_struct_member_pair* pairs, kit_struct* s)
{
  if (kit_struct_init(name, nmembers, member_names, s)) return -1;

  int e = 0;
  for (u32 i = 0; i < nmembers; i++) {
    int xe = kit_struct_set_member_by_name(s, pairs[i].name, &pairs[i].value);
    if (e == 0 && xe != 0) e = xe;
  }

  return e;
}

int
kit_struct_set_member_by_name(const kit_struct* s, const char* name, const struct kit_var* value)
{
  u32  hash  = kit_hash(name, strlen(name));
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

  return kit_struct_set_member_by_index(s, index, value);
}

int
kit_struct_set_member_by_index(const kit_struct* s, u32 index, const struct kit_var* value)
{
  if (index >= s->member_count) return -1;
  return kit_var_shallow_cpy(value, &s->members[index]);
}

int
kit_struct_set_member_pairs(const kit_struct* s, u32 npairs, const struct kit_struct_member_pair* pairs)
{
  int e = 0;
  for (u32 i = 0; i < npairs; i++) {
    int xe = kit_struct_set_member_by_name(s, pairs[i].name, &pairs[i].value);
    if (e == 0 && xe != 0) e = xe;
  }
  return e;
}
