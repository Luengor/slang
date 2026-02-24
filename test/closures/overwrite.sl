auto fn = () -> none {
    auto upval = "hello";
    auto closure = () -> none { print(upval); };

    upval = "world";
};

fn();

