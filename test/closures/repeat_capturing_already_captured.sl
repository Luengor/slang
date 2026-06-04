auto func = () -> none {
    auto a = 1;
    auto b = 2;
    auto c = 3;

    auto closure_1 = () -> none {
        print(a);
        print(b);
        print(c);
    };

    auto closure_2 = () -> none {
        print(a);
        print(b);
        print(c);
    };

    closure_1();
    closure_2();
};
func();

# expect: 1
# expect: 2
# expect: 3
# expect: 1
# expect: 2
# expect: 3

