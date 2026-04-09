auto func = () -> none {
    auto upval = 1;

    auto first_closure = () -> none { upval = upval + 1; };
    auto second_closure = () -> none { upval = upval + 2; };

    first_closure();
    second_closure();

    # expect: 4
    print(upval);
};

func();

