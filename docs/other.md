# Other notes
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

