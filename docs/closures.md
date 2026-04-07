# Slang closure approach
Even tho Slang's closure implementation is pretty influenced by Lua's, it has
some significant differences. The main difference is that Lua "lifts" register
values to the heap when they are captured by a closure, while Slang directly
creates those values on the heap.

## Runtime representation 
Closures are achieved at runtime by the combined use of three objects:
functions, closures and upvalues.

### Function
Function objects are the static part of a closure. All data in them is immutable
and known at compile time. Appart from the name, type info and bytecode, they also
contain a list of upvalue descriptors.

These descriptors (UpvalueInfo) are used to determine how to create the UpValues
of a closure when it is created. They contain the following information:
 - The index to capture from the parent closure. This field has 2 special values:
   - HALF_BAKED (-1): the upvalue is actually created by the current closure and
     not captured from the parent. 
   - LOOP_UPVAL (-2): the upvalue is created by the current closure in a loop. This
     upvalues need to be re-created on each iteration of the loop, and the same index
     is used for all of them. The compiler emits a special instruction (`CreateUpVal`)
     for this purpose.
 - Whether the upvalue is an object and needs to be retained and released. If
   it is, this process is handled automatically by the `GetUpval` and
   `SetUpval` instructions, so the compiler doesn't need to worry about it.

### UpValue
UpValues are the runtime representation of captured variables. They are created
by closures when they are created, called or requested. As other objects, they
are reference counted and can be retained and released ([for now at
least](https://todo.soymilk.lueng.org/tasks/97)).

They contain the following fields:
 - The value of the upvalue. This value **is** reference counted in the case of
   being an object.
 - A boolean indicating whether the upvalue is an object and needs to be retained
   and released.
 - A pointer to the next upvalue of the same index in the same closure. This is used
   to allow for recursive closures, where a new upvalue is created for each recursive
   call but old upvalues are still valid.

Accessing upvalues is done through the `GetUpval` and `SetUpval` instructions,
which take care of retaining and releasing the upvalue value if needed.

### Closure
Closure objects are the runtime representation of functions. They contain a pointer
to the base function object and a list of upvalues either created by the closure
itself or captured from the parent one.

Closures are created by the `Closure` instruction, which creates a closure from
a given object constant and fills its upvalues according to the upvalue
descriptors of the function object.

