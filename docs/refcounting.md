# Reference counting 
On slang, all heap allocated objects are reference counted. This is done to
avoid a complex garbage collector and to allow for deterministic destruction of
objects.

## What is counted 
 - All objects are reference counted. This includes strings, upvalues,
   functions and closures but not plain Values.
 - Every object in an active register is counted. There is no way to do this
   automatically, so the compiler must emit `Retain` and `Release` instructions
   to manage the reference count of objects.
 - All object constants but the first in a chunk are counted. The first one is
   self and treated differently.
 - Running functions and closures are counted. When a function/closure is called,
   it is retained, and when it returns, it is released.
 - Both the upvalues and the function objects of a closure are counted.
 - Objects in upvalues are _usually_ counted. The exception is with not closed
   upvalues. Because this upvalues refer to registers, they are considered to
   be a "duplicate" of the value and not a separate object. When an upvalue is
   closed, the value is moved to the heap. Because of that, a retain for the
   upvalue and a release for the register are emitted. This cannot be omitted
   because an upvalue may or may not be captured by a closure (test
   closures.conditional_closure), and the compiler cannot know it at compile
   time.
 - Upvalues in the list of a CallFrame are counted. This is needed because the
   closure that created could have been released before the end of the current
   CallFrame, and those upvalues need to be kept alive.

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
 - When a captured object variable is the return value of a function, a `Retain`
   is emitted on the return register before the `Return` instruction. Normally,
   the pre-return `Release` for captured variables is suppressed and their +1
   reference count transfers to the upvalue via `cleanUpvalues`. When that same
   variable is also returned, both the upvalue and the caller would claim the
   same +1 without this extra `Retain`, leading to a use-after-free once the
   caller releases the value.

#### Releasing
 - When assigning an object, the previous value is released.
 - After calling a function, the callee is released.
 - When an object goes out of scope, it is released. The exception to this is
   when the scope ending is a function and the object is captured by a closure.
   In this case, the object is not released, as it could cause to be deleted
   before the upvalue lifting done on the return op. If the captured variable
   is also the return value, a `Retain` is emitted instead (see Retaining
   above).
 - An object that is the result of an expression statement is released.
 - Releasing the arguments of a function call is the responsibility of the
   callee, not the caller. They are treated as locals variables inside the
   callee.

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

