#pragma once

#define TypeGuard \
    if (this->type_resolved) return; \
    this->type_resolved = true;

#define CompileGuard\
    if (this->has_compiled) return; \
    this->has_compiled = true;

#define reg(nm) nm->result.reg
#define type(nm) nm->result.type
#define is_var(nm) nm->result.is_var

#define reg_var_alloc(nm)                                                      \
    (reg != -1                                                                 \
         ? reg                                                                 \
         : (is_var(nm) ? ctx.allocateRegister() : reg(nm)))

// Free when this is not a var and we didn't use it
#define should_free(nm) (!is_var(nm) && reg(this) != reg(nm))

#define SKIP_CONSTANT_GET_REG(var_name)                                        \
    if (ctx.function->chunk.currentOffset() != 0 &&                            \
        !ctx.nameTable.isRegisterVariableOwned(var_name)) {                    \
        auto last_instr = ctx.function->chunk.last();                          \
        if (GET_op(last_instr) == OpCode::Constant) {                          \
            uint16_t constant_index = GET_Bx(last_instr);                      \
            ctx.function->chunk.pop();                                         \
            var_name = constant_index + 256;                                   \
        }                                                                      \
    }
