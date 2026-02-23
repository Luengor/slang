auto x = "global";

auto outer = () -> none {
    auto x = "outer";
    auto inner = () -> none {
        print(x);
    };
    inner();
};

# expect: outer
outer();

