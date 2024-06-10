<div align="center">

# asmlox

The **[Lox programming language]** compiling to ARM64 machine code.

</div>

## Current status

The project currently has support for:
- Global and local variables
- branching, i.e. `if-else`, `while`, and `for`
- Function calls.
- Most unary and binary operators
- Garbage collection (except for code segments in REPL mode)

## TODO

The items that currently need to be implemented are:

- Remaining binary and unary operators
- Garbage collection of code segment in REPL mode
- Implementation of upvalues
- Implementation of classes
- Native functions

Beyond full feature parity with `clox`, I'd like to implement the following:
- Full support for calls back and forth between asmlox and native code.

## Technical details

The architecture is still a single-pass compiler, with no optimization and type inference.
All operations are still entirely dynamic in nature.

Dynamic operations are implemented via lookup in an operation table. They could also be "statically linked", but this seems to yield no significant performance benefit, and would require that the JIT code is allocated within max jump offset (128MB) from the text segment of the binary, and would get in the way of any subsequent GC operations that move the code.

Calls use the C stack to spill link and frame pointer registers, as well as to store and pass values. Operations will remove operands from the stack, and push the result.

On ARM64, the stack pointer must be aligned at a 16 byte boundary, which is fine when using a struct-based Value type. If one wanted to use NaN boxing, or make GC simpler, one could instead use instead use one of the callee-save registers as a Lox stack pointer, and `mmap` memory for the Lox stack, with a forbidden page at the end of that stack.

[lox programming language]: http://craftinginterpreters.com/

