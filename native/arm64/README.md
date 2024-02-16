# Assembly

## Conventions

The closure pointer is stored in x19, and the constant table pointer in x20.
These two registers must be spilled onto the stack, along with the frame pointer and link pointer,
when a new lox function is called.

### Calling Convention
Arguments to a function will be passed on the stack, with the stack pointer pointing to the last stack slot at when the call is made.
The runtime call handler must then:

1. Ensure that the called value is callable.
2. That the number of arguments match.
3. Spill frame pointer, return address, x19 (closure pointer), and x20 (constant table pointer) onto the stack, with fp pointing to the stack slot with the frame pointer and return address.
4. Make a tail call to the starting address of the function of the called value.

On return, the return handler must:
1. Read the arity of the returning function.
2. Restore the spilled registers of the calling function.
3. Place the returned result of the function in the first argument slot, and move the stack pointer so as to clear the remaining slots.
