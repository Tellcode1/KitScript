fn opaque(arg) {
   return arg;
}

fn sum(l) {
   let s = 0;
   for (let i = 0; i < len(l); i++) {
      s += l[i];
   }
   return s;
}

fn LICM_HELP_ME(x) {
   let y = 0;
   for ( i in 0..x ) {
      y += 10;
   }
   return y;
}

fn main() {
   let a = 10;
   a = 20;
   a = 30;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   a = 40;
   if (a != 40) println("wtf");
   if (40 != a) println("wtf");

   let x = null;
   if (opaque(x)) {
      println(x); /* won't print */
   }

   let nomb = [1, 2, 3, 4, 5];
   let bomb = sum(nomb);
   println("Expect 15: ", bomb);

   println("Expect 100: ", LICM_HELP_ME(10));
}