# Compilation error: Parsing failed.
auto func1 = (fixed a) -> fixed {
    self = (fixed a) -> fixed {
        return a + 1;
    };
    self(a);
};
