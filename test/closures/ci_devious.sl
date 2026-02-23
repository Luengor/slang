auto outer = () -> () -> () -> none {
    auto x = "value";
    auto middle = () -> () -> none {
        auto inner = () -> none {
            print(x);
        };

        print("create inner closure");
        return inner;
    };

    print("return from outer");
    return middle;
};

# expect: return from outer
auto mid = outer();

# expect: create inner closure
auto in = mid();

# expect: value
in();

