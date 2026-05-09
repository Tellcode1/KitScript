fn edit_refer(r) {
   r[0] = 3;
   r[1] = 2;
   r[2] = 1;
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
      io::println(io::STDERR, "Vector indexing failed");
      error = true;
   }

   vector.x = 10.0;
   vector.y += 10.0;
   if (vector != vec2(10.0, 10.0)) {
      io::println(io::STDERR, "Vector indexing & member access failed");
      error = true;
   }

   vector += vec2(100, 200);
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
   let s = stroctor(true, false);
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

   if ((mix_and_match_1 != mix_and_match_2[0]) || (mix_and_match_2[0] != mix_and_match_3[2])) {
      io::println(io::STDERR, "Expected [[64], [64], #{2:64}], but got ", [mix_and_match_1, mix_and_match_2, mix_and_match_3]);
   }


   if (!error) {
      io::println(io::STDERR, "Successful! No errors");
   } else {
      io::println(io::STDERR, "Errors were encountered...");
   }

   return 0;
}