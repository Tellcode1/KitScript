
fn main() {
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
   fvec.v0 = rand::vec2();
   fvec.v1 = rand::vec2();
   fvec.v2 = rand::vec2();
   fvec.v3 = rand::vec2();
   println(fvec);
}