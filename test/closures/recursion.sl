auto fn = (bool first_call) -> () -> none {
    # Create an upvalue that will be captured by the closure
    auto text = first_call ? "first call" : "second call";
    auto print_fn = () -> none { print(text); };

    # If this is not the first call, return print_fn directly
    if (not first_call) {
        return print_fn;
    }

    # If this is the first call, call self
    auto other_print_fn = self(false);

    print_fn();
    print(text);
    other_print_fn();
    print(text);

    return () -> none {};
};

# expect: first call
# expect: first call
# expect: second call
# expect: first call
fn(true);

