type parity_t = (fixed, parity_t) -> bool;

parity_t _is_odd = (fixed n, parity_t is_even) -> bool {
    if (n == 0)
        return false;

    return is_even(n - 1, self);
};

parity_t _is_even = (fixed n, parity_t is_odd) -> bool {
    if (n == 0)
        return true;

    return is_odd(n - 1, self);
};

auto is_odd = (fixed n ) -> bool {
    return _is_odd(n, _is_even);
};

auto is_even = (fixed n ) -> bool {
    return _is_even(n, _is_odd);
};

# expect: false 
print(is_even(1));

# expect: true
print(is_odd(1));

# expect: true 
print(is_even(4));

# expect: false
print(is_odd(4));

