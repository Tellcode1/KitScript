fn main() {
   println(2+3);

   let l = ["hello","world"];
   for (let i = 0; i < len(l); i++) {
      if (l[i] == "hello") println("hello");
      else if (l[i] == "world") println("world");
      else println(i, ", ", l[i]);
   }
}