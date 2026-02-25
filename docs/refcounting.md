# Reference counting 
On slang, all heap allocated objects are reference counted. This is done to
avoid a complex garbage collector and to allow for deterministic destruction of
objects.

## What is counted 
 - All objects are reference counted. This includes strings, upvalues,
   functions and closures but not plain Values.
 - Every object in an active register is counted. There is no way to do
   this automatically, so the compiler must emit `Retain` and `Release` instructions
   to manage the reference count of objects.
 - All object constants but the first in a chunk are counted. The first one is
   self and treated differently.
 - Running functions and closures are counted. When a function/closure is called,
   it is retained, and when it returns, it is released.
 - Both the upvalues and the function objects of a closure are counted.
 - Objects in Upvalues are counted. When an object is stored in an upvalue, it
   is retained, and when the upvalue is released, the object is released as
   well. Because of this, upvalues must know whether they contain an object or
   not.

## How it is kept track of
Because (most of the time) the interpreter does not know the type of the values
being manipulated, it cannot automatically manage all reference counts. Most of
the time, the compiler must emit explicit `Retain` and `Release` instructions
to manage the reference count of objects.

### Explicit reference counting
#### Retaining
 - When a variable (containing an object) access is being compiled, if the
   proposed result register is not the same as the one of the variable, a
   `Copy` instruction followed by a `Retain` is emitted to put the variable in
   the proposed register and retain it.
 - In an assign expression, if the proposed register is not the same as the one
   of the variable being assigned to, a `Copy` instruction followed by a
   `Retain` is emitted to put the new value in the proposed register and retain
   it.

#### Releasing
 - When assigning an object, the previous value is released.
 - After calling a function, the callee is released.
 - When an object goes out of scope, it is released.
 - An object that is the result of an expression statement is released.
 - Releasing the arguments of a function call is the responsibility of the
   callee, not the caller. They are treated as locals variables inside the
   callee.
 - When an upvalue is written to, the old value is released from the registers
   if its not needed there anymore.

### Implicit reference counting
Some instructions implicitly modify the reference count of objects:
 - `Call` retains the closure or function being called and releases it when the
   call is done.
 - Closures retain the function and upvalues they capture, and release them
   when they are released themselves. The upvalues created by a half-baked
   closure are also released when that closure is returned from. 
 - `Object` and `Self` always place an object in the target register.
 - Upvalues **do** know whether they contain an object or not. `GetUpval`
   retains the object, and `SetUpval` releases the old value if there is one
   and retains the new value.

