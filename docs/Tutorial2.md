# Tutorial 2: Variables

By now, you have created a basic program to verify the functionality of KScript.\
This tutorial will teach you what variables are and how we can print them.

***A variable, at its core, is simply a container that lets us store values for later.***

## Declaring a variable
Variables in KScript are declared using the `let` keyword, followed by the variable's name.\
As in `let number`. This declares a variable, number.

Suppose we want our variable to hold `10` for us. We can do this by adding the `=` operator with whatever value we wish for it to hold.
All in all, this becomes: `let number = 10` (declare a variable , "number", with the value 10).


## Using variables
Printing hello world to the console is cool and all. But how about we print our number?

To do this, we simply replace "Hello, World!" with our number:
```
fn main() {
    let number = 10;
    println(number);
}
```

Notice however, that there are no quotes(") surrounding number.\
This is because we were printing the *string literal* Hello world earlier, and now we're printing the *value* of our variable number.

\
If you are already feeling detached from our Hello world program, we can rewrite it to use a variable too!
```
fn main() {
    let number = "Hello, world!";
    println(number);
}
```

## Assignment
What if, we wanted to print both our number and a greeting?

We *could* use two variables as shown:
```
fn main() {
    let number = 10;
    println(number);

    let hello = "Hello, world!";
    println(hello);
}
```
We however have to use two different names for our variables since it is illegal for two variables in the same function to have the same name.

(Think about why that is. If we allowed two variables to share the same name in the same place, how would KScript know which one you meant?)

Or, we could just reuse a single variable:
```
fn main() {
    let shared = 10;
    println(shared);

    shared = "Hello, world!";
    println(shared);
}
```

Two things:
- Firstly, there is no `let` in the third line. That is because we are not *declaring* a variable anymore, we're *setting* its value.

- KScript allows us to change not only a variable's value, but also the type of value it holds. Case in point, our variable `shared` holds an integer (Computer science speak for number) first, then a string.

Run your program using the same method as in tutorial 0:
`./kscript -x hello.k` and you should see the following output in your terminal:
```
10
Hello, world!
```

Congratulations! You've created and used your first variables.
You will be using them everywhere before long.

In our next tutorial, we will try to get our program to do some basic maths!

In the meantime, why don't you try out the following exercises?
   - Try to put something else in place of `10` or `Hello, world`. A different number perhaps.
   - Try to add another line that prints `"Goodbye, world!"`.
   - Try to predict what the following program will output:
```
fn main() {
    let a = "Hello, world!";
    a = "Goodbye, world!";
    println(a);
    println(a);
```
  - Think about why it prints "Goodbye, world!" twice.
