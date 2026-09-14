#ifndef KIT_LIBRARY_H
#define KIT_LIBRARY_H

#include "kit.exec.h"
#include "kit.stdafx.h"

typedef struct kit_library_export {
  const char* name;                                                          /* What the KScript side uses to call this function */
  kit_ecode (*funcp)(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result); /* Function pointer */
} kit_library_export;

typedef struct kit_library_info {
  const char*               module_name;
  const kit_library_export* exports;
  u32                       nexports;
} kit_library_info;

/* Your library must expose this!! It will be loaded through dlsym */
const kit_library_info* kit_library_entry_point(void);
typedef const kit_library_info* (*kit_library_entry_point_fn)(void);

#endif // KIT_LIBRARY_H
