fn only_accept_strings(s) {
   if (type_of(s) != type::STRING) {
      println("Invalid input: Only expect strings");
      return -1;
   }

   println(s);
   return 0;
}

fn list_of_args(args) {
   for (let i = 0; i < len(args); i++) {
      if (type_of(args[i]) != type::STRING) {
         println("Function only expects strings in the list");
         return -1;
      }
   }

   for (let i = 0; i < len(args); i++) {
      print(args[i]);
   }
}

fn main() {
   println("-- ignore any errors, the output will always contain 2 of them. the source code is more relevant --");
   only_accept_strings(123);
   only_accept_strings(456);
   only_accept_strings("We are printing a string");

   list_of_args([ 1, 2, 3, 4, 5 ]);
   list_of_args([ 1, 2, "gaming", false, true ]);
   list_of_args([ "Hello", "Darkness", "My", "Old", "Friend\n", ]);
}