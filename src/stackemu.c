#include "../inc/stackemu.h"

#include "../inc/cc.h"

#include <stdlib.h>
#include <string.h>

int
e_stackemu_init(e_stackemu* emu)
{
  const u32 new_capacity = 32; // Atleast 1

  memset(emu, 0, sizeof *emu);

  u32* new_frames = (u32*)e_xalloc(new_capacity, sizeof(u32));
  if (!new_frames) return -1;

  struct ecc_variable_information* new_vars = e_xalloc(new_capacity, sizeof(ecc_variable_information));
  if (!new_vars) {
    free(new_frames);
    return -1;
  }

  emu->frame_var_starts = new_frames;
  emu->vars             = new_vars;

  emu->vars_count    = 0;
  emu->vars_capacity = new_capacity;

  emu->strucs_count    = 0;
  emu->strucs_capacity = 0;

  emu->frame_top      = 0;
  emu->frame_capacity = new_capacity;

  return e_stackemu_push_frame(emu);
}

void
e_stackemu_free(e_stackemu* emu)
{
  free(emu->vars);
  free(emu->frame_var_starts);
  memset(emu, 0, sizeof *emu);
}

int
e_stackemu_push_frame(e_stackemu* emu)
{
  if (emu->frame_top >= emu->frame_capacity) {
    u32  new_capacity = emu->frame_capacity * 2;
    u32* new_frames   = realloc(emu->frame_var_starts, sizeof(u32) * new_capacity);
    if (!new_frames) return -1;

    emu->frame_var_starts = new_frames;
    emu->frame_capacity   = new_capacity;
  }

  emu->frame_var_starts[emu->frame_top++] = emu->vars_count;
  return 0;
}

void
e_stackemu_pop_frame(e_stackemu* emu)
{
  u32 start       = emu->frame_var_starts[--emu->frame_top];
  emu->vars_count = start;
}

int
e_stackemu_push_var(e_stackemu* emu, const ecc_variable_information* info)
{
  if (emu->vars_count >= emu->vars_capacity) {
    u32                              new_capacity = emu->vars_capacity * 2;
    struct ecc_variable_information* new_vars     = realloc(emu->vars, sizeof(ecc_variable_information) * new_capacity);
    if (!new_vars) return -1;

    emu->vars_capacity = new_capacity;
    emu->vars          = new_vars;
  }

  memcpy(&emu->vars[emu->vars_count++], info, sizeof(*info));
  return 0;
}

struct ecc_variable_information*
e_stackemu_find_var(const e_stackemu* emu, u32 id)
{
  for (u32 i = emu->vars_count; i > 0; i--) {
    if (emu->vars[i - 1].name_hash == id) return &emu->vars[i - 1];
  }
  return NULL;
}

struct ecc_variable_information*
e_stackemu_find_var_in_curr_scope(const e_stackemu* emu, u32 id)
{
  if (emu->frame_top == 0) return NULL;

  u32 start = emu->frame_var_starts[emu->frame_top - 1];

  for (u32 i = emu->vars_count; i > start; i--) {
    if (emu->vars[i - 1].name_hash == id) return &emu->vars[i - 1];
  }

  return NULL;
}

int
e_stackemu_copy_top_scope(e_stackemu* emu, const e_stackemu* old)
{
  if (old->frame_top == 0) return 0;

  u32 start = old->frame_var_starts[old->frame_top - 1];

  for (u32 i = start; i < old->vars_count; i++) {
    int e = e_stackemu_push_var(emu, &old->vars[i]);
    if (e) return e;
  }

  return 0;
}