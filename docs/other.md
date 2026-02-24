# Other notes
## Reference Counting
The following rules apply to reference counting:
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

## Register Usage
 - After compiling a Node, its result type, register and if it's a variable
   is stored in a structure.
    ```cpp
    struct ResultInfo {
        TypeID type;
        bool is_var = false;
        int reg = -1;
    };
    ```
 - All compile functions take an optional register to store the result.
 - If the register **is** provided:
    - The result **must** be stored in that register.
    - The given register _should_ not be passed down to any sub-expressions.
 - If the register is **not** provided:
    - A node should first compile its sub-expressions without a target
      register, and then use a free register or the child ones if possible.

