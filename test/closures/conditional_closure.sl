auto func = (bool should_closure) -> none {
    auto upval_sometimes = 1;

    if (!should_closure) {
        return;
    }

    auto closure = () -> none { upval_sometimes = upval_sometimes + 1; };
    closure();

    print(upval_sometimes);
};

# expect: 1
func(false);

# expect: 2
func(true);

