fn opaque(arg) {
   *result = arg;
return KIT_OK;
}

fn sum(l) {
   let s = 0;
   for (let i = 0; i < len(l); i++) {
      s += l[i];
   }
   *result = s;
return KIT_OK;
}

fn main() {
   assert(2 > 5);
   println(2+3);

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
      println(x);
   }

   let nomb = [1, 2, 3, 4, 5];
   let bomb = sum(nomb);
   println(bomb);
}