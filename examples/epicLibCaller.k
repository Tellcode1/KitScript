/* Compile epicLib.c first (or don't who cares) */

extern fn epicLibraryFunction(name);

fn main() {
   print("Calling our epic library function with the argument Albert Intestine\n");
   epicLibraryFunction("Albert Intestine");
}