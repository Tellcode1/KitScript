#include "../inc/kit.cast.h"
#include "../inc/kit.list.h"
#include "../inc/kit.var.h"

#include <stdlib.h>

static inline int
vector_cmp(const double* va, const double* vb, u32 nelems)
{
  double mgtd_a = 0.0F;
  double mgtd_b = 0.0F;
  for (u32 i = 0; i < nelems; i++) {
    mgtd_a += va[i] * va[i];
    mgtd_b += vb[i] * vb[i];
  }
  return (mgtd_a > mgtd_b) - (mgtd_a < mgtd_b);
}

static inline u32
vector_dims(kit_vartype type)
{
  switch (type) {
    case KIT_VARTYPE_VEC2: return 2;
    case KIT_VARTYPE_VEC3: return 3;
    case KIT_VARTYPE_VEC4: return 4;
    default: break;
  }
  return 0;
}

static int
var_cmp(const void* a, const void* b)
{
  const kit_var* va = (const kit_var*)a;
  const kit_var* vb = (const kit_var*)b;

  if (va->type != vb->type) return (int)va->type - (int)vb->type;

  switch (va->type) {
    case KIT_VARTYPE_INT:
    case KIT_VARTYPE_BOOL:
    case KIT_VARTYPE_CHAR: return kit_cast_to_int(va) - kit_cast_to_int(vb);
    case KIT_VARTYPE_FLOAT: return (va->val.f > vb->val.f) - (va->val.f < vb->val.f);
    case KIT_VARTYPE_STRING: return strcmp(KIT_VAR_AS_STRING(va)->s, KIT_VAR_AS_STRING(vb)->s);

    case KIT_VARTYPE_VEC2:
    case KIT_VARTYPE_VEC3:
    case KIT_VARTYPE_VEC4: return vector_cmp(va->val.vec4, vb->val.vec4, vector_dims(va->type));

    default: return 0;
  }
}

static void
insertion_sort(kit_var* arr, u32 left, u32 right)
{
  for (u32 i = left + 1; i <= right; i++) {
    kit_var tmp = arr[i];

    i32 j = (i32)i - 1; /* decremented */
    while (j >= (i32)left && var_cmp(&arr[j], &tmp) > 0) {
      arr[j + 1] = arr[j];
      j--;
    }

    arr[j + 1] = tmp;
  }
}

static void
merge(kit_var* arr, u32 left, u32 mid, u32 right)
{
  u32 len1 = mid - left + 1;
  u32 len2 = right - mid;

  kit_var* left_arr  = kit_xalloc(len1, sizeof(kit_var));
  kit_var* right_arr = kit_xalloc(len2, sizeof(kit_var));

  memcpy(left_arr, &arr[left], len1 * sizeof(kit_var));
  memcpy(right_arr, &arr[mid + 1], len2 * sizeof(kit_var));

  u32 i = 0;
  u32 j = 0;
  u32 k = left;
  while (i < len1 && j < len2) {
    if (var_cmp(&left_arr[i], &right_arr[j]) < 0) {
      arr[k++] = left_arr[i++];
    } else {
      arr[k++] = right_arr[j++];
    }
  }
  while (i < len1) { arr[k++] = left_arr[i++]; }
  while (j < len2) { arr[k++] = right_arr[j++]; }

  free(right_arr);
  free(left_arr);
}

int
kit_tim_sort(struct kit_var* elems, u32 nelems)
{
  const u32 RUN = 32;
  for (u32 i = 0; i < nelems; i += RUN) insertion_sort(elems, i, (i + RUN - 1 < nelems - 1) ? i + RUN - 1 : nelems - 1);

  for (u32 size = RUN; size < nelems; size = 2 * size) {
    for (u32 left = 0; left < nelems; left += 2 * size) {
      u32 mid   = left + size - 1;
      u32 right = (left + (2 * size) - 1 < nelems - 1) ? left + (2 * size) - 1 : nelems - 1;

      if (mid < right) merge(elems, left, mid, right);
    }
  }

  return 0;
}