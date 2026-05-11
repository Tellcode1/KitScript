#include "stack.h"

int
e_stack_init(u32 capacity, u32 frame_capacity, u32 variable_capacity, e_stack* stack)
{
  const u32 stack_alignment = 16;

  capacity        = MAX(capacity, 8);
  stack->capacity = capacity;
  stack->size     = 0;
  stack->stack    = (e_var*)e_aligned_malloc(sizeof(e_var) * capacity, stack_alignment);
  if (stack->stack == nullptr) return E_EMALLOC;

  stack->depth          = 0;
  stack->frame_capacity = frame_capacity;
  stack->frames         = (e_stack_frame*)e_xalloc(frame_capacity, sizeof(e_stack_frame));
  if (stack->frames == nullptr) return E_EMALLOC;

  stack->variable_capacity = variable_capacity;
  stack->nvariables        = 0;
  stack->variables         = (e_var_entry*)e_xalloc(variable_capacity, sizeof(e_var_entry));
  if (stack->variables == nullptr) return E_EMALLOC;

  stack->max_usage = 0;

  return 0;
}

void
e_stack_free(e_stack* stack)
{
  for (u32 i = 0; i < stack->size; i++) e_var_release(&stack->stack[i]);
  e_xfree((void**)&stack->frames);
  e_xfree((void**)&stack->variables);
  e_aligned_free(stack->stack);
  memset(stack, 0, sizeof *stack);
}

int
e_stack_push_frame(e_stack* stack)
{
  if (stack->depth >= stack->frame_capacity) {
    u32            frames_capacity = stack->frame_capacity * 2;
    e_stack_frame* frames          = (e_stack_frame*)realloc(stack->frames, frames_capacity * sizeof(e_stack_frame));
    if (frames == nullptr) return E_EMALLOC;

    stack->frames         = frames;
    stack->frame_capacity = frames_capacity;
  }

  stack->frames[stack->depth] = (e_stack_frame){
    .variable_offset = stack->nvariables,
    .stack_size      = stack->size,
  };
  // printf("Pushed frame [depth=%u]\n", stack->depth);
  stack->depth++;

  return 0;
}

void
e_stack_pop_frame(e_stack* stack)
{
  // printf("Popped frame [depth=%u]\n", stack->depth - 1);
  const e_stack_frame* frame = &stack->frames[--stack->depth];

  /* Release the variables we're popping. */
  for (size_t i = frame->stack_size; i < stack->size; i++) { e_var_release(&stack->stack[i]); }

  stack->size       = frame->stack_size;
  stack->nvariables = frame->variable_offset;
}

int
e_stack_push(e_stack* stack, const e_var* v)
{
  const u32 alignment = 16;
  if (stack->size >= stack->capacity) {
    u32    new_cap   = stack->capacity * 2;
    e_var* new_stack = (e_var*)e_aligned_realloc(stack->stack, stack->capacity * sizeof(e_var), new_cap * sizeof(e_var), alignment);
    if (new_stack == nullptr) return E_EMALLOC;

    stack->capacity = new_cap;
    stack->stack    = new_stack;
  }

  // printf("Pushed: ");
  // eb_println((e_var*)v, 1);

  e_var* slot = &stack->stack[stack->size];
  e_var_shallow_cpy(v, slot);
  e_var_acquire(slot);

  stack->size++;
  if (stack->size > stack->max_usage) stack->max_usage = stack->size;

  return 0;
}

void
e_stack_pop(e_stack* stack)
{
  e_var* top = &stack->stack[stack->size - 1];
  // printf("Popped: ");
  // eb_println((e_var*)top, 1);
  e_var_release(top);

  *top = E_NULLVAR;
  stack->size--;
}

e_var*
e_stack_top(e_stack* stack)
{ return &stack->stack[stack->size - 1]; }

/**
 * Push a variable.
 * All variables are popped on frame pop.
 */
e_var*
e_stack_push_variable(u32 id, e_stack* stack)
{
  if (stack->nvariables >= stack->variable_capacity) {
    u32          new_c         = stack->variable_capacity * 2;
    e_var_entry* new_variables = (e_var_entry*)realloc(stack->variables, new_c * sizeof(e_var_entry));
    if (new_variables == nullptr) return nullptr;

    // set new variables to null
    for (u32 i = stack->nvariables; i < new_c; i++) {}
    memset(new_variables + stack->nvariables, 0, (new_c - stack->nvariables) * sizeof(e_var_entry));

    stack->variables         = new_variables;
    stack->variable_capacity = new_c;
  }

  // fprintf(stderr, "Variable with ID %u pushed to stack top %u\n", id, stack->size);

  /* Push entry in variables table */
  e_var_entry* var  = &stack->variables[stack->nvariables];
  var->offset_index = stack->size;
  var->depth        = stack->depth;
  var->id           = id;
  stack->nvariables++;

  /* Initialized to VOID */
  e_var void_var = (e_var){ .type = E_VARTYPE_NULL };
  if (e_stack_push(stack, &void_var)) return NULL;

  return e_stack_top(stack);
}

/**
 * Find a variable. NULL if doesn't exist.
 */

e_var*
e_stack_find(const e_stack* stack, u32 hash)
{
  // Old ver broke for stack->nvariables = 0
  for (u32 i = stack->nvariables; i-- > 0;) {
    u32 offset = stack->variables[i].offset_index;
    if (stack->variables[i].id == hash) return &stack->stack[offset];
  }
  return nullptr;
}

/**
 * Find a variable in the current depth of the stack.
 * NULL if not in stack.
 */
e_var*
e_stack_find_in_current_scope(const e_stack* stack, u32 hash)
{
  u32 depth = stack->depth;
  // Old ver broke for stack->nvariables = 0
  for (u32 i = stack->nvariables; i-- > 0;) {
    if (stack->variables[i].depth != depth) break;

    u32 offset = stack->variables[i].offset_index;
    if (stack->variables[i].id == hash) return &stack->stack[offset];
  }
  return nullptr;
}