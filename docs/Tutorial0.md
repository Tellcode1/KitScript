# Tutorial 1: Hello world

If you've ever done any programming, you've probably encountered the classic Hello, World! program.

The idea is simple: write a program that prints Hello, world! to the terminal.
It's a useful first exercise because it lets us verify that KScript is working and introduces a few basic concepts.

By the end of this tutorial, you'll have written and run your first KScript program.

## Installation
To start, ensure KScript is installed and functioning.
You can find prebuild binaries for KScript here: https://github.com/Tellcode1/KitScript/releases

Or you can build it yourself, following the instructions from the BUILD section of README.

Run KScript with the -h flag to verify it's working.

## Creating your script
Create your script file and name it hello.k
We will write code in this file that the computer will execute.
- By convention, script files have the extension ".k". But it is a convention nonetheless.

Open the file in your favourite editor, and continue from there.

## Our first function
In KScript, "main" is the first function your program will always execute.

Lets declare one:
```
fn main() {

}
```


Breaking it down:
- fn is used to denote that we are defining a function.
- main is the name of the function we're declaring.
- The parentheses () is used for the argument list. main can not take arguments.
- The braces {} enclose the function body. It is empty right now.
- [Unlike C, no return value is expected from main]

Now, you can run your program and it'll run, but it won't do anything.

*Your computer did what you told it to: Nothing at all.*\
*But we do need some way to tell our program is running, don't we?*

## Printing to the terminal
To get some sort of response from our program, we can use the println function.
The println function prints a line (ln) to the terminal.

Lets use that:
```
fn main() {
	println("Hello, world!");
}
```

A few things:
- println is a function. We want to call it.
  - This is done through writing the name of the function, and then parentheses containing all the arguments we want it to take.
  - As in `FunctionName(Argument0, Argument1, Argument2)`
 - println takes in a list (however many) of things to print. We pass it a string, "Hello, world!"
- Strings are "sequences of characters".


## Running your program
Now, to run your program, locate the KScript binary, and pass the script file to it with the -x switch (CompileToBinary + ExecuteBinary).
As in:
	`./kscript -x hello.k`

If everything is working correctly, you will see `Hello, world` in the console.

Congratulations! You're written your first KScript program.
