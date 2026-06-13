# Slang closure approach
Slang's closure implementation is very similar to Lua's. The main idea is to
capture variables on the registers of the parent closure, and then "lift" them
to the heap when they go out of scope.

## Runtime representation 
Closures are achieved at runtime by the combined use of three objects:
functions, closures and upvalues.

### Function
Function objects are the static part of a closure. All data in them is immutable
and known at compile time. Apart from the name, type info and bytecode, they also
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

UpValues have a series of quirks that should be kept in mind when dealing with
them:
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
 - Whether the upvalue is open or closed. Open upvalues point to a register in
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
Each call frame holds two distinct upvalue lists that serve opposite roles:

- **`closure->upvalues`** — the upvalues *this* closure reads and writes.
  Created once when the `Closure` instruction runs; lives as long as the
  `ClosureObj`. These are the upvalues the compiler refers to by index via
  `GetUpval`/`SetUpval`.

- **`frame.captured_upvalue`** — a linked list of open upvalues that point
  *into this frame's registers*, created by inner closures defined inside this
  frame. Its purpose is to know which registers were captured, so they can be
  closed when their scope ends.

#### Upvalue creation (`Closure` instruction)

When the `Closure` instruction runs, a new `ClosureObj` is created. For each
upvalue descriptor in the function object:

- **Local capture** (`is_local = true`): the closure walks the *current frame's*
  `captured_upvalue` list looking for an existing open upvalue for that register.
  If found, it is shared (not duplicated). If not, a new `UpValue` is created,
  set to open (pointing to that register), and **prepended** to
  `frame.captured_upvalue`.
- **Transitive capture** (`is_local = false`): the upvalue is copied directly
  from the parent closure's `upvalues` list. No new `UpValue` is created; both
  closures share the same `shared_ptr`.

#### Lifting (`LiftUpvalue` instruction)

Emitted by the compiler whenever a captured local variable goes out of scope
(but the frame is still alive). Steps:

1. Walk `frame.captured_upvalue` to find the `UpValue` for the given register.
2. If not found: the variable was never captured by any closure. Nothing to do.
3. If `use_count == 1`: the linked list is the only owner — no closure holds a
   reference to this upvalue. Skip closing; just unlink and drop it.
4. Otherwise: at least one closure still holds it. Copy the register value into
   `upval->data.value`, set `is_closed = true`, and retain it if it is an
   object. From this point on, `GetUpval`/`SetUpval` read from the heap value
   instead of the register.
5. Unlink the upvalue from `frame.captured_upvalue` in either case.

#### Cleanup on return (`cleanUpvalues`)

When a frame returns, any upvalues still in `frame.captured_upvalue` are closed
unconditionally (no `use_count` check). The registers are about to become
invalid, so any surviving upvalue must have its value copied to the heap.
Unlike `LiftUpvalue`, no `retain` is emitted here — the compiler already skips
the pre-return `Release` for captured variables, so their existing reference
count transfers directly to the upvalue. After closing all of them,
`frame.captured_upvalue` is set to `nullptr`, releasing the list's `shared_ptr`
references. Upvalues with no remaining owners are destroyed immediately at that
point.

**Exception — captured variable as return value:** if the return expression is
itself a captured local object variable, `cleanUpvalues` will consume that
register's +1 for the upvalue, leaving the caller with no reference of its own.
To prevent this, the compiler emits a `Retain` on the return register before
emitting the `Return` instruction (`ReturnStmt::compile`, `ast_stmt.cpp`). This
gives the caller its own independent +1 so that the caller and the upvalue each
hold one reference to the object.


## Compiler implementation
On type resolution, when all other methods of name resolution fail, the
compiler checks if a parent context exists and tries to perform the same lookup
on it recursively. If a variable is found, it is marked as captured on that and
following contexts until the current one. Intermediate contexts and the current
one mark the variable as an upvalue.

To clarify, a variable is marked is marked as captured if an inner context
captures it, and it is marked as an upvalue if the current context captures it.

```
Outer context:
  auto a  -> captured, not an upvalue

  Intermediate context:
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
`CompileContext::getUpvalueIndex`. This method checks if there is already an
upvalue for the variable. If not, it recursively checks on the same variable on
the parent context. Is should always end up finding the starting variable,
which should be marked as captured but not an upvalue. It will have a normal
register assigned to it. From there, the method creates a new upvalue pointing
to that register, and intermediate contexts create new upvalues pointing to the
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

  Intermediate context:
    b  -> not captured, upvalue, assigned to upvalue 0 which points to register 1
    (c) -> captured, upvalue, assigned to upvalue 1 which points to register 2

    Inner context:
      c  -> not captured, upvalue, assigned to upvalue 0 which points to upvalue 1

Legend:
  'auto x' means that variable 'x' is created here.
  '(x) means that variable 'x' is here even tho it is not used in this context.
  'x' means that variable 'x' is used in this context.
```

