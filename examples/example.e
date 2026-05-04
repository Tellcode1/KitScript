extern say_hello_from_c();
extern gooboog();

fn PISS()
{
  println("PISS!");
}

fn WHATS2PLUS2()
{
  println("2 + 2 is ", 2 + 2);
}

fn TakesIn2Arguments(x, y)
{
  return x * y;
}

fn different_return_types(flag)
{
  if (flag) {
    return "True!";
  } else {
    return 0;
  }
}

fn never_return_because_im_evil()
{
  while (true);
}

// struct vec2 {
//   let x;
//   let y;
// };

fn update()
{
  println("Running 100 iterations of free fall test (Results are for after 100 seconds):");

  let position = 0;
  let velocity = 0;
  let acceleration = 9.8;

  let i = 0;
  while (i < 100)
  {
    let tmp_v = velocity + acceleration;
    position = position + velocity;
    velocity = tmp_v;
    i++;
  }

  println("position = ", position, " meters");
  println("velocity = ", velocity, " meters/sec");
}

fn divide_by_2(v)
{
  return v / 2;
}

namespace gaming
{
  namespace another_one {
    fn fart() {
      return 67;
    }
  };
  let hidden_var = 0;
};

fn maptest()
{
  let map = #{ 1 : "One!", 2 : "Two!", 3 : "Three!", 4 : "Four!", 5 : "Five!" };
  for (let i = 1; i < len(map)+1; i++)
  {
    print(i, ": ");
    println(map[i]);
  }

  let reverse = #{
    "One!"   : 1,
    "Two!"   : 2,
    "Three!" : 3,
    "Four!"  : 4,
    "Five!"  : 5 
  };
  let index_table = [
    map[1], // Uses the key to get the value for the map.
    map[2],
    map[3],
    map[4],
    map[5],
  ];
  for (let i = 0; i < len(map); i++)
  {
    println("Look up of ", index_table[i], " : ", reverse[index_table[i]]);
  }
}

// Shouldn't modify the actual value of lol
fn reference_test(lol)
{
  for (let i = 0; i < len(lol); i++)
  {
    lol[i] *= 2;
  }
}

fn outlive()
{
  let one = [1, 2, 3];
  let two = [one, one, one];
  return two;
}

fn main() {
  println("Im gaming rn");
  println("println(32) = ", 32);
  println("println(16.0) = ", 16.0);
  println("println(false) = ", false);

  println("Should be 10: ", 5 * 2);
  println("Should be 7: ", 5 + 2);
  println("Should be 2: ", 5 / 2);
  println("Should be 2.5: ", 5.0 / 2.0);

  println("Should be true: ", !!!!!!!!!!true);

  println("Should be true: ", 10 > 2);
  println("Should be false: ", 10 < 2);
  println("Should be 2: ", 10 % 4);

  if (3 > 2) {
    println("Scientific breakthrough: 3 is greater than 2");
  } else {
    println("2nd class maths fail :3");
  }

  if ((10 - 2) < 0) {
    println("PISS");
  } else if ((10 - 2) > 0) {
    println("BIG PISS");
  } else {
    println("Secret message");
  }

  if (0) {
    println("goob ");
  }
  else if (0) {
    println("goop");
  }
  else {
    let goob = "goob";
    println("Gambing: ", goob);
  }

  let x = 16;
  let y = 32;
  println(x);

  x = 32 + y;
  println(x);

  let z = x + y - 90;
  while (z >= 0)
  {
    print(z);
    if (z != 0)
    {
      print(", ");
    }
    z--;
  }
  print("\n");

  println("16 ^ 32 = ", 16 ^ 32);
  println("16 | 32 = ", 16 | 32);
  println("3 & 2 = ", 3 & 2);

  while (false) {} while (false) {}
  while (false) {} while (false) {}
  while (false) {} while (false) {}
  while (false) {} while (false) {}
  while (false) {} while (false) {}

  if (false)
  {

  }
  println("Shouldn't be skipped ^-^"); // there was bug where jump would set the instruction index
                                      // and then the loop would iterate (instruction + 1), skipping an instruction.

  PISS();

  WHATS2PLUS2();

  println(TakesIn2Arguments(10, 20));

  println("Same function, multiple return types: ", different_return_types(true), ", ", different_return_types(0));

  let coercion_test = 2 + 2.5;
  println(coercion_test);

  let it_did_not_work = 3.2 + 5.2;
  println(it_did_not_work);

  let why_does_it_work_now = 6.2 + true;
  println(why_does_it_work_now);

  let NO_LEAKS_YAYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY = 68;
  println(NO_LEAKS_YAYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY, " ", NO_LEAKS_YAYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY, " ", NO_LEAKS_YAYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY);

  update();

  // Unlike C, functions can be called before they are declared!
  call_before_declare();
  fn call_before_declare()
  {
    println("This function was called before it was declared!");
  }

  say_hello_from_c();

  file_test();

  let multi_assign_test1 = 2;
  let multi_assign_test2 = 1;
  let multi_assign_test3 = 4;
  println("Before multi assignment: ", multi_assign_test1, ", ", multi_assign_test2, ", ", multi_assign_test3);
  multi_assign_test1 = multi_assign_test2 = multi_assign_test3 = 16;
  println("After multi assignment: ", multi_assign_test1, ", ", multi_assign_test2, ", ", multi_assign_test3);

  print("For statement test (Running numbers 1 through 10): ");
  for (let i = 1; i <= 10; i++)
  {
    print(i);
    if (i != 10)
    {
      print(", ");
    }
  }
  print("\n");

  for (let i = 0; i <= 16; i++)
  {
    // divide by 8
    print(divide_by_2(divide_by_2(divide_by_2(float(i)))));
    if (i != 16-1)
    {
      print(", ");
    }
  }
  print('\n');

  let bigl = [12, 2 + 2, "Gaming", 68.0, 16, false, ["Another list!", 2 - 2.5, true], "More_stuff"];
  println(bigl);

  bigl[6] = "lmao";
  println(bigl);

  let l = [1,2,3,4,5,6,7,8,9,10];
  println(l[2]);

  println("len(", l, ") = ", len(l));

  print("[");
  for (let i = 0; i < len(l); i++)
  {
    print(l[i]);
    if (i != len(l) - 1)
    {
      print(", ");
    }
  }
  print("]");
  print("\n");

  for (let i = 0; i < len(l); i++)
  {
    l[i] *= 2;
    l[i] *= 2;
  }

  println(l);

  let sum = 0;
  for (let i = 0; i < len(l); i++)
  {
    sum += l[i];
  }
  println("Sum of all elements in the list is: ", sum);

  println("INT32_MIN = ", int::MIN, ", " , "INT32_MAX = ", int::MAX);

  let i = (-500);
  while (i < 500)
  {
    i += 1;
  }
  println("Should be 500: ", i);

  println(gaming::another_one::fart());
  gaming::hidden_var = 66;
  println(gaming::hidden_var);

  for (let multi1 = 0, multi2 = 4, multi3 = 6; multi1 <= 3 && multi2 >= 0 && multi3 >= 0; multi1++, multi2--, multi3 -= 2)
  {
    println(multi1, ", ", multi2, ", ", multi3);
  }

  let PI  = 3.14159265358979323846;
  let PI2 = 2 * math::PI;
  println("PI = ", PI);
  print("math::sin(0) = ", math::sin(0), "; ");  print("math::sin(PI) = ", math::round(math::sin(PI)), "; "); println("math::sin(PI / 4) = ", math::sin(PI / 4), "; ");
  print("math::cos(0) = ", math::cos(0), "; ");  print("math::cos(PI) = ", math::round(math::cos(PI)), "; ");   println("math::cos(PI / 4) = ", math::cos(PI / 4), "; ");
  print("math::tan(0) = ", math::tan(0), "; ");  print("math::tan(PI) = ", math::round(math::tan(PI)), "; "); println("math::tan(PI / 4) = ", math::tan(PI / 4), "; ");

  if (PI == math::PI)
  {
    println(math::PI, " vs ", PI);
    println("Our constant is pretty close to the value of PI!");
  }

  // math::PI = 3;

  // math vs meth
  // PI = 3;

  const let constant = "I hate all humans";
  println(constant);

  let b = 1;
  while (gooboog())
  {
    b*=2;
  }
  println(b);

  for (let i = 0, x = 1, y = 1, z = 1; i < 5; i++)
  {
    x++;
    y--;
    z *= 2;
    println(x, ", ", y, ", ", z);
  }

  maptest();

  let s = "Pee is stored in the balls";
  let lo = str::len(s);
  println("len(s) = ", lo);

  let s1 = "I'm equal to the 2nd string!";
  let s2 = "I'm equal to the 2nd string!";
  if (str::equal(s1,s2))
  {
    println("Strings are expectedly equal!");
  } else {
    println("str::equal is wrong!");
  }

  s1 = string(x + 1 - 1 + 1);
  s2 = string(x + 1);
  if (str::equal(s1,s2))
  {
    println("Strings are, again, expectedly equal!");
  } 

  let to_repeat = "Repeat";
  let repeated = str::repeat(to_repeat, 5);
  println("repeat(s, 5) = ", repeated);

  let advanced_listing = list::make(1,2,3,4,5,6,7,8,9,10);
  println("Our list is: ", advanced_listing, " And our query is for 5");
  println("list::find(l,5) returns: ", list::find(advanced_listing, 5));

  list::remove(advanced_listing, list::find(advanced_listing, 5));
  println("Removed 5, I don't like it");

  println("Our new list is: ", advanced_listing, " And our query is for 5");
  println("list::find(l,5) returns: ", list::find(advanced_listing, 5));

  println("Appending 11 to 20 to the list...");
  for (let i = 11; i <= 20; i++)
  {
    list::append(advanced_listing, i);
  }

  println("Our new list is: ", advanced_listing);

  println("Ehh.. I kinda don't like the new list. I'm popping the last 5 elements");
  for (let i = 0; i < 5; i++) list::pop(advanced_listing);

  println("Perfect! Now I have: ", advanced_listing);

  let items_to_get_from_store = "Bread, Butter, Cheese, Cheese, Cheese, Ketchup, Milk, Cheese, Jam";
  let split_items = str::split(items_to_get_from_store, ", ");
  println("We'll need to get these items from the store: ", split_items);

  let numbers = [1,2,3,4,5,6,7,8,9,10,];
  reference_test(numbers);
  println(numbers);

  let uninitialized,uninitialized1,uninitialized2;
  println(uninitialized, ", ", uninitialized1, ", ", uninitialized2);

  println(uninitialized==uninitialized1 && uninitialized1==uninitialized2);
  if (uninitialized == null && uninitialized1 == null && uninitialized2 == null)
  {
    println("Riveting. All the uninitialized variables are null");
  }

  let numbers_but_not = numbers;
  numbers_but_not[1] = 100;
  println(numbers_but_not);

  let out_of_bounds_access = numbers_but_not[69];
  if (out_of_bounds_access == null)
  {
    println("Our out of bounds access was caught and handled :>");
  }

  let p = #{ "Poot" : "Dispenser", "Still need" : "Dispenser", "WHERE IS MY" : "Dispenser" };
  if (!p["NonExistentString"])
  {
    println("Map invalid value access validated :3");
  }
  println("glooby");

  p["Goopy"] = "no goopy";
  println(p["Goopy"]);

  println(outlive());

  let piss = [[16]];
  piss[0][0] = 42;
  io::println(io::STDOUT, "HELP: ", piss);

  let random_nums = rand::list(0, int::MAX, 32);
  println("Generating 32 random numbers for no reason at all: ", random_nums);
}

fn file_test()
{
  let file = io::open("file", "r");
  if (file == null)
  {
    println("Failed to load file: ", "file", ": ", io::error());
    return null;
  }
  defer io::close(file);

  let contents = io::readln(file);
  println(contents);
}