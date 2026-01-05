#pragma once

#define ResolveGuard \
    if (this->result.has_value()) \
        return; \
    this->result.emplace();

#define reg(nm) nm->result.value().reg
#define type(nm) nm->result.value().type
#define is_var(nm) nm->result.value().is_var

#define reg_var_alloc(nm)                                                      \
    (reg != -1                                                                 \
         ? reg                                                                 \
         : (is_var(nm) ? ctx.allocateRegister() : reg(nm)))
