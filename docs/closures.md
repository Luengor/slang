# Slang closure approach
Slang's closure implementation is very similar to Lua's. The main idea is to
capture variables on the registers of the parent closure, and then "lift" them
to the heap when they go out of scope.

## Runtime representation 
Closures are achieved at runtime by the combined use of three objects:
functions, closures and upvalues.

### Function
Function objects are the static part of a closure. All data in them is immutable
and known at compile time. Appart from the name, type info and bytecode, they also
contain a list of upvalue descriptors.

These descriptors (UpvalueInfo) are used to determine how to create the UpValues
of a closure when it is created. They contain the following information:
 - Whether the upvalue refers to another upvalue or to a local variable.
 - The upvalue index or the register index of the variable to capture.
 - Whether the upvalue is an object and needs to be retained and released. If
   it is, this process is handled automatically by the `GetUpval` and
   `SetUpval` instructions, so the compiler doesn't need to worry about it.

### UpValue
UpValues are the runtime representation of captured variables. They are always
created by the `Closure` instruction, which creates a closure and fills its
upvalues according to the upvalue descriptors of the function object.

UpValues have a series of quirks that its important to keep in mind:
 - An Upvalue always refers to a variable in the register file or to the value
   they contain. That value can be either an object or a primitive, but it
   **never** is another UpValue. More generally, almost everywhere where an
   object can be used, an upvalue **will not** be used. (This will be more
   obvious when [Upvalues stop being
   Objects](https://todo.soymilk.lueng.org/tasks/97))
 - UpValues are created by the closure that uses them, not by the closure the
   captured value belongs to.
 - A value can only be captured/pointed to by *1* UpValue. If more than one
   upvalue tries to capture the same variable, the same upvalue should be
   shared between them.

Upvalues contain the following fields:
 - Wether the upvalue is open or closed. Open upvalues point to a register in
   the stack, while closed upvalues "hold" the value of the variable in them. 
 - Whether the upvalue is an object and needs to be retained and released.
 - The data of the upvalue, which can be either a register index or a value,
   depending on whether the upvalue is open or closed. If the upvalue is
   closed, the value is referenced counted.
 - A pointer to the next upvalue in the list of open upvalues on the
   call-frame.

Accessing upvalues is done through the `GetUpval` and `SetUpval` instructions,
which take care of retaining and releasing the upvalue value if needed. There
is also a `LiftUpval` instruction, which is used to "lift" an open upvalue to
the heap when the variable goes out of scope. More on this later.

### Closure
Closure objects are the runtime representation of functions. They contain a pointer
to the base function object and a list of upvalues either created by the closure
itself or captured from the parent one.

Closures are created by the `Closure` instruction, which creates a closure from
a given object constant and fills its upvalues according to the upvalue
descriptors of the function object. Note that when an upvalue descriptor refers
to another upvalue, a new one will not be created, but the same upvalue will be
shared between the two closures. UpValues **never** point to other upvalues. On
top of that, because only one upvalue can refer to a given register, when a
closure tries to capture a variable on a register, it first checks if there is
already an open upvalue for that register. If there is, it shares it instead of
creating a new one.

### VM
At runtime, the VM maintains a linked list of open upvalues on each call-frame.
When a closure creates a new upvalue, it is added to this list. When "lifting"
an upvalue, the VM looks for it in the list of open upvalues and if it finds it,
it removes it from the list and closes it. If it doesn't find it, it means the
upvalue wasn't captured by any closure and it can be safely ignored.

This lifting isn't necessary when returning from functions, since before
returning from a function, all open upvalues are either lifted or closed,
depending on whether they are referenced by another object or not.


## Compiler implementation
On type resolution, when all other methods of name resolution fail, the
compiler checks if a parent context exists and tries to perform the same lookup
on it recursively. If a variable is found, it is marked as captured on that and
following contexts until the current one. Intermidiate contexts and the current
one mark the variable as an upvalue.

To clarify, a variable is marked is marked as captured if an inner context
captures it, and it is marked as an upvalue if the current context captures it.

```
Outer context:
  auto a  -> captured, not an upvalue

  Intermidiate context:
    (a) -> captured, upvalue

    Inner context:
      a  -> not captured, upvalue

Legend:
  'auto x' means that variable 'x' is created here.
  '(x) means that variable 'x' is here even tho it is not used in this context.
  'x' means that variable 'x' is used in this context.
```

Then, during code generation, variables marked only as captured are ignored, as
they behave just like normal variables. Variables marked as upvalues usually
follow a different compilation path.

Instead of normal register assignment, they use the
`CompileContext::getUpvaluIndex`. This method checks if there is already an
upvalue for the variable. If not, it recursively checks on the same variable on
the parent context. Is should always end up finding the starting variable,
which should be marked as captured but not an upvalue. It will have a normal
register assigned to it. From there, the method creates a new upvalue pointing
to that register, and intermidiate contexts create new upvalues pointing to the
previous one until it reaches the top context.

Keep in mind that this upvalue chaining is only a compilation detail. At
runtime, closures don't create chained upvalues, but reuse the same upvalue
directly.

An example with 3 variables and 3 nested contexts would look like this:
```
Outer context:
  auto a  -> not captured, not an upvalue, assigned to register 0
  auto b  -> captured, not an upvalue, assigned to register 1
  auto c  -> captured, not an upvalue, assigned to register 2

  Intermidiate context:
    b  -> not captured, upvalue, assigned to upvalue 0 which points to register 1
    (c) -> captured, upvalue, assigned to upvalue 1 which points to register 2

    Inner context:
      c  -> not captured, upvalue, assigned to upvalue 0 which points to upvalue 1

Legend:
  'auto x' means that variable 'x' is created here.
  '(x) means that variable 'x' is here even tho it is not used in this context.
  'x' means that variable 'x' is used in this context.
```

