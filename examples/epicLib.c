#include "../inc/kit.library.h"
#include "../inc/kit.var.h"

#include <stdio.h>

/**
 * Compile with gcc -c -fPIC epicLib.c -o epicLib.o
 * gcc -shared epicLib.o libKScript.a -o epicLib.so
 *
 * Link while running your executable by passing the path to epicLib.so with the -lib command line argument.
 */

static kit_ecode
epicCFunction(kit_vm* vm, kit_var* args, u32 nargs, kit_var* result)
{
  printf("Hello, ");
  kit_var_print(&args[0], stdout);
  printf(", from the land of C!\n");

  *result = KIT_NULLVAR;
  return KIT_OK;
}

static const kit_library_info lib = {
  "libc",
  .exports  = (kit_library_export[]){ { .name = "epicLibraryFunction", .funcp = epicCFunction } },
  .nexports = 1,
};

const kit_library_info*
kit_library_entry_point()
{ return &lib; }