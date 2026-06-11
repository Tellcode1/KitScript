[INTRODUCTION]
E (working on the name) is a dynamically typed, simple scripting language.
It is a language targeted at integration with game engines, offering
great interopability with C/C++ (other languages will be worked on).

It currently has 5 basic types: int, float, strings, lists and (hash)maps.


[PRINTING]
Values can be printed to the console using one of the two builtin functions provided.
print(): prints all variables provided, in order, to the console.
println(): Similar to print, but prints a newline after printing the arguments.


[LISTS & REFERENCE COUNTED STRUCTURES]
Lists are generic structures that store, well, a list of variables.
Lists are a reference counted structure. All instances of a list (copied around)
point to the same variable, which is freed when all its holders go out of scope.

Sounds like gibberish? I agree.

So, let nums = [ 0, 1, 2, 3, 4, 5 ];
And, let nums_but_not = nums;
nums_but_not now points to the original nums list.
Lets try assigning to nums_but_not to see this in action and print it.

nums_but_not[1] = 100;
println(nums_but_not);

This should print: [0, 100, 2, 3, 4, 5]
Now, lets send nums_but_not to places nums will never
get to go. Say we return it from a function. nums will die.
But its cousin, nums_but_not lives on. And it carries his brother's value(s).

And so when nums_but_not's time eventually comes, and it will go out of scope.
The stack will consume them all.

[LIST INDEXING]
Like most sane programming languages, indexing in E starts from 0.
To index a list of 5, and get 5 from it, we'll use the square brackets (to index it).

let noom = [ 1, 2, 3, 4, 5 ];
println(noom[4]); // << noom[4] is 5. Indexing starts from 0, remember?

The syntax seems confusing at first, but you'll get used to it eventually.
Most programming languages follow it... So it's not a skill wasted.

As a test, Try to assign 30 to replace 3.


[LIST BUILTINS]
The list builtin namespace provides the following functions:
i.  list::make
=>  Make a list from the given elements.
eg. list::make("Yes", "No", "Maybe"); // Returns ["Yes", "No", "Maybe"]

ii. list::append
=>  Append (push) a value to the end of the list.
eg.
let nums = [1, 2, 3];
list::append(nums, 1234); // nums now contains [1, 2, 3, 1234]

iii. list::pop
=>  Pop (remove) the last element of the list.
eg. list::pop(nums); // nums now contains [1, 2, 3]

iv.  list::remove
=>  Remove a value from the specified index, shifting all the leftover
elements.
eg. list::remove(nums, 2); // nums now contains [1,2]

v.   list::insert
=>  Assign a value to the specified index.
eg. list::insert(nums, 4, 4); // nums now contains [1, 2, void, 4]

vi.  list::find
=>  Find an element in the list, and return its index. -1 if it does
not exist in the list.
eg. list::find(names, "John Johnington"); // Returns index of John in the list.

vii.  list::reserve
=>  Increase the capacity of the list to reduce further memory allocations
eg. list::reserve(names, 1024); // Names now has space for 1024 more variables

viii. list::len
=>  Alias for len().


[VARIABLE DECLERATION]
Variables in the script can be declared using the 'let' keyword.
For example, to declare a variable PI, we can use the syntax:
let PI; // Initializes PI to VOID
At first, all variables have the value of VOID. However, variables can be initialized to another value.
Let's initialize our PI variable to 3.
let PI = 3;

Notice we won't change the value of PI across the program. In this case, we can mark the variable as 'const' (constant!)
To do this, we have three, albeit similar methods.
let const PI = 3;
const let PI = 3;
and
const PI = 3;
Note that if you mark a variable as const, you *must* initialize it, and can not change its value later.


[BYTECODE STARTUP]
On starting the script, the VM initializes all global (and namespaced/qualified) variables.
After this, it jumps to the main function.
The program must have a main function defined.
main Can not accept any arguments.

fn main()


[BYTECODE FORMAT]
Compiled binary file structure is:
MAGIC : u32
bytesMemNeeded : u32 // << The number of bytes of memory required to load the whole program
                         Explanation below
nLiterals : u32
Literals : kit_var[nLiterals]{
   Type : kit_var_type
   value : kit_var_payload
 For strings:
   Type : kit_var_type
   Len : u32
   String : u8[Len]
}

nFunctions : u32
Functions = kitc_function[]{
  code_size : u32
  nargs : u32
  name_hash : u32
  arg_slots = u32[nargs]
  function_code : u8[code_size]
}

bytecodeSize : u32
bytecodeStream : u8[bytecodeSize]

We use a single allocation for the entire program to ensure the code remains fast.
Thus, we rely on the compiler to provide us with the bytes of memory required to initialize
the program. The value provided is a conservative estimate, and some bytes may be lost.
Also, strings are ref counted. So We have to point the refc
to the allocation (+offset) and initialize their refcounters to 1.


[C INTEROPABILITY | CALLING C FUNCTIONS FROM SCRIPT]
C functions can be called from the script using the kit_extern_function entry
in kit_exec_info. The table defines a list of functions that can be called,
using their hashes.

A namespaced (external) function can be defined from C simply by hashing the full name.
So, for instance, to define a function 'sin' in the math namespace externally:
Create an external function entry with the name: "math::sin". This function is
fully visible to the script.

External functions are checked BEFORE script functions. This way, one can override
a user defined function easily.
The actual order of function calls is: builtin > external > local.

However, you will need to be careful not to override a function of the
script unintentionally. Choose a clear and concise name. Make sure it
does not or will not appear in the script.