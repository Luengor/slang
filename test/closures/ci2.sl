auto makeClosure = (str value) -> () -> none {
    auto closure = () -> none {
        print(value);
    };
    return closure;
};

auto doughnut = makeClosure("doughnut");
auto bagel = makeClosure("bagel");

# expect: doughnut
doughnut();

# expect: bagel
bagel();

