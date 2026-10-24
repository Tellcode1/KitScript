fn edit_refer(r) {
   r[0] = 3;
   r[1] = 2;
   r[2] = 1;
}

let global = 0;

fn v2add(va, vb)
{
  return vec2(va.x + vb.x, va.y + vb.y);
}

fn v2sub(va, vb)
{
  return vec2(va.x - vb.x, va.y - vb.y);
}

fn v2scale(v, f)
{
  return vec2( v.x * f, v.y * f );
}

fn m4identity()
{
  return mat4(
    vec4(1.0, 0.0, 0.0, 0.0),
    vec4(0.0, 1.0, 0.0, 0.0),
    vec4(0.0, 0.0, 1.0, 0.0),
    vec4(0.0, 0.0, 0.0, 1.0),
  );
}


fn main() {
   let error = false;

   let variable;
   if (variable != null) {
      io::println(io::STDERR, "Uninitialized variable not set to NULL");
      error = true;
   }

   let list_2d = [[42]];
   if (list_2d[0][0] != 42) {
      io::println(io::STDERR, "2D list assign/access failed (expected [[42]], have ", list_2d, ")");
      error = true;
   }

   let mop = #{ "Unrelated" : 100 };
   if (mop["Related?"] != null) {
      io::println(io::STDERR, "Map has entry for something we never added...?");
      error = true;
   }

   let l = [1,2,3];
   list::append(l, 4);
   list::append(l, 5);
   for (let i = 1; i <= len(l); i++) {
      if (l[i - 1] != i) {
         io::println(io::STDERR, "Expected ", i, ", but got ", l[i - 1]);
         error = true;
      }
   }

   let s = "([{<";
   let new_s = str::cat(s, ">}])");
   if (new_s != "([{<>}])") {
      io::println(io::STDERR, "String catenation failed, expected ([{<>}]), but got ", new_s);
      error = true;
   }

   let vector = vec2(1.0, 0.0);
   vector[0] = 2.0;
   if (vector[0] != 2.0 || vector[1] != 0.0) {
      io::println(io::STDERR, "Vector indexing failed. Expected <x=2.0, y=0.0>, but got: ", vector);
      error = true;
   }

   vector.x = 10.0;
   vector.y += 10.0;
   if (vector != vec2(10.0, 10.0)) {
      io::println(io::STDERR, "Vector indexing & member access failed. Expected <10, 10> but got: ", vector);
      error = true;
   }

   vector += vec2(100, 100);
   if (vector != vec2(110.0, 110.0)) {
      io::println(io::STDERR, "Vector addition (v+v) failed");
      error = true;
   }

   let ctr = 0;
   for (let i = 0; i < 100; i++) {
      ctr++;
      defer ctr--;
   }

   for (let i = 0; i < 100; i++) {
      defer {
         ctr++;
         ctr--;
      };
   }

   if (ctr != 0) {
      io::println(io::STDERR, "Defer count invalid! There is a problem with defer in loop");
      error = true;
   }

   let refer = [1,2,3];
   edit_refer(refer);
   if (refer != [3,2,1]) {
      io::println(io::STDERR, "List reference was not edited correctly: Expected ", [3,2,1], ", but got ", refer);
      error = true;
   }

   list::append(refer, 3);
   list::append(refer, 2);
   list::append(refer, 1);
   if (refer != [3,2,1, 3,2,1]) { // List comparison, wow!
      io::println(io::STDERR, "List append/comparison failed: Appended [3,2,1] to [3,2,1] but got: ", refer);
      error = true;
   }

   let multiassign_1 = 2;
   let multiassign_2 = 4;
   let multiassign_3 = 8;
   
   const multiassign_target = 16;
   multiassign_1 = multiassign_2 = multiassign_3 = multiassign_target;

   if (multiassign_1 != multiassign_target || multiassign_2 != multiassign_target || multiassign_3 != multiassign_target) {
      io::println(io::STDERR, "Multiassignment failed! Expected [16, 16, 16], got ", [multiassign_1, multiassign_2, multiassign_3]);
      error = true;
   }

   struct stroctor {
      let glooby;
      let goopy;
   };
   s = stroctor(true, false);
   if (s.glooby != true || s.goopy != false) {
      io::println(io::STDERR, "Structure member assignment failed: Expected [true, false], got ", [s.glooby, s.goopy]);
      error = true;
   }

   struct Cell {
      let another_cell;
   };
   let nested_struct = Cell(Cell(null));
   if (nested_struct.another_cell.another_cell != null) {
      io::println(io::STDERR, "Nested structure member assignment failed, expected {{null}}, got ", nested_struct);
      error = true;
   }

   let mix_and_match_1 = 2;
   let mix_and_match_2 = [3];
   let mix_and_match_3 = #{ 2 : 64 };
   mix_and_match_1 = mix_and_match_2[0] = mix_and_match_3[2];

   if ((mix_and_match_1 != mix_and_match_3[2]) || (mix_and_match_2[0] != mix_and_match_3[2])) {
    io::println(io::STDERR, "Expected [[64], [64], #{2:64}], but got ", [mix_and_match_1, mix_and_match_2, mix_and_match_3]);
    error = true;
   }

   s = "I COUGH need to COUGH add COUGH dynamic compilation COUGH and execution COUGH support to the COUGH language";
   let replaced = str::replace(s, " COUGH ", " ");
   assert (replaced == "I need to add dynamic compilation and execution support to the language");

   assert "I added dynamic execution" == "I added dynamic execution";
   assert [] != false;
   assert "Hello" != "hello";
   assert kit::type_of("[]") == kit::type::STRING;
   assert kit::type_of([]) == kit::type::LIST;
   assert kit::type_of(nested_struct) == kit::type::STRUCT;
   assert 0 != null; // They're different types. NULL should only be equal to NULL.
   assert [] != "";

   const source_file = str::cat(
      "fn main() {",
         "let x = sys::get_cmd_args();",
         "return int(x[0]);",
      "}"
   );
   let exec_info = kit::exec_info(
      /* source_code */ source_file,
      /* entry_point */ "main",
      /* optimization_level */ 0,
      /* arguments */ [],
      /* command_line_arguments */ [ "123456789" ],
      /* libraries */ []
   );
   let r = kit::compile_and_exec(exec_info);
   if (r != 123456789) {
      io::println(io::STDERR, "JIT compilation failed... Expected 123456789, got:  ", r);
      error = true;
   }

   let a = 0.0;
   let b = 1.0;
   let t = 0.5;
   if (math::lerp(a, b, t) != 0.5) {
      io::println(io::STDERR, "lerp returned invalid value");
      error = true;
   }

   a = vec2::ZERO;
   b = vec2::ONE;
   if (math::lerp(a, b, t) != (vec2::ONE * 0.5)) {
      io::println(io::STDERR, "lerp (vector) returned invalid value");
      error = true;
   }

   let original = ["avx", "avx2", "sse"];
   let i = 0;
   
   /* try to clobber original */ 
   while (i < len(original)) {
      let elem = original[i];
      i = i + 1;
   }

   if (kit::type_of(original) != kit::type::LIST || len(original) != 3 || original[0] != "avx") {
      io::println(io::STDERR, "loop clobbered our register!");
      error = true;
   }

   let synchronize = 0;
   for (synchronize in 0..3) { /* should run [0,1,2] */

   }
   if (synchronize != 3) {
      io::println(io::STDERR, "ranged for loop expected to run 3 times (loop increment counter) = ", synchronize);
      error = true;
   }

   for (i in 0..100) {
      global++;
   }

   for (i in 0..100) {
      global = 0;
   }
   if (global != 0) {
      io::println(io::STDERR, "Global variable has invalid value, expected 0, got: ", global);
      error = true;
   }

   struct Filler {
      let a;
      let b;
      let c;
   };
   let f = Filler(1,2,3);
   if (f.a != 1 || f.b != 2 || f.c != 3) {
      io::println(io::STDERR, "Struct constructor did not fill in values correctly (Expected {1,2,3}), got:  ", f);
      error = true;
   }

   f = Filler{ a:2, b:3, c:4 };
   if (f.a != 2 || f.b != 3 || f.c != 4) {
      io::println(io::STDERR, "Struct constructor did not fill in values correctly (Expected {1,2,3}), got:  ", f);
      error = true;
   }

   f = Filler{ a:2, /*b:3,*/ c:4 };
   if (f.b != null) {
      io::println(io::STDERR, "Struct constructor did not fill in uninitialized member to null, got:  ", f);
      error = true;
   }

   let nomber = 0;
   if (nomber++ == nomber++) { /* Left one *should* execute first */
      io::println(io::STDERR, "LHS of comparison did not execute first. Expected 2:  ", nomber);
      error = true;
   }

   /* Try to open a random file */
   let fool = io::open("asdddddddddddddakjsdhkjhdfjakjsdhfjkhsjkdhfsjkdhfa", "r");
   if (fool != null) {
      io::println(io::STDERR, "Why do you have a file named asdddddddddddddakjsdhkjhdfjakjsdhfjkhsjkdhfsjkdhfa?");
      error = true;
   }

   const OPERATE_ADD = 0;
   const OPERATE_SUB = 1;
   const OPERATE_MUL = 2;

   const RESULT_EVEN = 0;
   const RESULT_ODD = 1;

   fn add(x,y) { return x + y; }
   fn sub(x,y) { return x - y; }
   fn mul(x,y) { return x * y; }

   fn operate_function(operation) {
      if (operation == OPERATE_ADD) {
         return add;
      }
      if (operation == OPERATE_SUB) {
         return sub;
      }
      if (operation == OPERATE_MUL) {
         return mul;
      }
   }

   let lamb = operate_function(OPERATE_ADD)(3,4);
   if (lamb != 7) {
      io::println(io::STDERR, "Expected 7 from function lambda call, got: ", lamb);
      error = true;
   }
   
   let pos = vec2(1.0, 1.0);
   let vel = vec2(1.0, 0.0);

   r = v2add(pos, vel);
   if (r != vec2(2, 1)) {
      io::println(io::STDERR, "Expected vector addition to yield (2,1), but got: ", r);
      error = true;
   }

   r.x = r.y = 0.1;
   if (r != vec2(0.1, 0.1)) {
      io::println(io::STDERR, "Expected vector member set to yield (0.1,0.1), but got: ", r);
      error = true;
   }

   let u = r;
   u[0] = 69;
   u[1] = 32;
   if (u != vec2(69, 32)) {
      io::println(io::STDERR, "Expected vector index set to yield (69,32), but got: ", u);
      error = true;
   }

   fn maybe_have_a_little_less_parameters(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x,y,z,a1,a2,a3,a4,a5,a6,a7,a8,a9,a0) {
      if (a != null) return null;
      if (b != null) return null;
      if (c != null) return null;
      if (d != null) return null;
      if (e != null) return null;
      if (f != null) return null;
      if (g != null) return null;
      if (h != null) return null;
      if (i != null) return null;
      if (j != null) return null;
      if (k != null) return null;
      if (l != null) return null;
      if (m != null) return null;
      if (n != null) return null;
      if (o != null) return null;
      if (p != null) return null;
      if (q != null) return null;
      if (r != null) return null;
      if (s != null) return null;
      if (t != null) return null;
      if (u != null) return null;
      if (v != null) return null;
      if (w != null) return null;
      if (x != null) return null;
      if (y != null) return null;
      if (z != null) return null;
      if (a1 != null) return null;
      if (a2 != null) return null;
      if (a3 != null) return null;
      if (a4 != null) return null;
      if (a5 != null) return null;
      if (a6 != null) return null;
      if (a7 != null) return null;
      if (a8 != null) return null;
      if (a9 != null) return null;
      if (a0 != null) return null;
      return "gaming";
   }

   s = maybe_have_a_little_less_parameters(null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null);
   if (s != "gaming") {
      io::println(io::STDERR, "Function call with ~30 arguments broke: Expected \"gaming\", but got: ", s);
      error = true;
   }

   if (!error) {
      io::println(io::STDERR, "Successful! All tests passed");
   } else {
      io::println(io::STDERR, "Errors were encountered...");
   }

   return 0;
}