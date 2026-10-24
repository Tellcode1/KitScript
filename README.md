# KScript
KScript is a scripting language written in C99, aiming to provide seamless integration with game engines for scripts.

***This project is in development: Expect things to break for no utter reason***

## Installation
Prebuilt binaries for KScript are available at: https://github.com/Tellcode1/KitScript/releases/

To build KScript for yourselves, you can use the Makefile provided in the project's root directory.

Simply run `make` in the project's directory: If succcessful, it will create the KScript binary (usually named kscript) and the KScript shared library.

## Example
A Hello world in KScript is:
```
fn main() {
   println("Hello, World!");
}
```

This file can be compiled to bytecode using the kscript program:
` ./kscript -c hello.k -o hello.ok`, adjust to your installation and script file.
The bytecode can be executed with ` ./kscript -e hello.ok`

If you want to compile and execute a program at once, you can use the `-x` switch: ` ./kscript -x hello.k`

## Features
KScript currently supports:
   - A small(ish) standary library.
   - Dynamic code execution.
   - Compilation and pretty good optimization, if needed.
   
   - First class support for vectors and matrices.
   - Weird syntax.

## Documentation
New to KScript (or programming in general?), Try out the tutorial set at https://github.com/Tellcode1/KitScript/tree/main/docs! \
Additionally, example programs are provided in the examples/ folder of the project.
