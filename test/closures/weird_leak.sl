auto global_f = () -> none { };

auto create_closure = () -> str {
    str x = "captured_string";
    global_f = () -> none {
        print(x);
    };
    return x;
};

# Call and discard return value
create_closure();

# Call the closure
global_f();

# expect: captured_string

