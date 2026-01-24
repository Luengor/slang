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

