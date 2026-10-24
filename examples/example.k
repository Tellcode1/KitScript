fn main() {
  let this_is_a_variable_decleration = "string";
  println(this_is_a_variable_decleration);

  // This is a comment

  /*
  This is also a comment
  */

 "is this a comment?"; /* he isn't. don't trust him */

 let this_is_initialized_to_null;

 let this_is_an_assignment;
 this_is_an_assignment = 10;
 this_is_an_assignment = "lmao balls";
 this_is_an_assignment = null;

 let this_is_a_list = [];
 list::append(this_is_a_list, 1);
 list::append(this_is_a_list, 2);
 list::append(this_is_a_list, 3);
 println("[1,2,3] == ",this_is_a_list);

 let work = [];
 for (let this_is_a_c_style_loop=1; this_is_a_c_style_loop <= 10; this_is_a_c_style_loop++) {
  /* do stuff */
  list::append(work, this_is_a_c_style_loop);
 }

 for (this_is_a_ranged_loop in 11..21) {
  list::append(work, this_is_a_ranged_loop);
 }

 println("shuld contain 1 thru 20: ", work);

    struct StringsList {
      let l;
   };

   let strings = StringsList([]);

   list::append(strings.l, "Hello, ");
   list::append(strings.l, "World!");
   list::append(strings.l, "and");
   list::append(strings.l, "now");
   list::append(strings.l, "Goodbye :(");

   println(strings);
   println(strings.l);

   struct fourvectors {
      let v0;
      let v1;
      let v2;
      let v3;
   };

   rand::seed("12345678910ABCDEFGHIJKLMNOPQRSTUVWXYZ");

   let fvec = fourvectors(vec2::ZERO,vec2::ZERO,vec2::ZERO,vec2::ZERO);
   println(fvec);
   fvec.v0 = rand::vec2();
   fvec.v1 = rand::vec2();
   fvec.v2 = rand::vec2();
   fvec.v3 = rand::vec2();
   println(fvec);

   struct Fill {
      let a;
      let b;
   };

   let filled = Fill{};
   println(filled);

   filled = Fill{ a:10, b:20 };
   println(filled);
}