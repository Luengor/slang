# 2
auto fn = (fixed a) -> fixed {
    self = (fixed a) -> fixed {
        return a + 1;
    };
    return self(a);
};

print(fn(1));

