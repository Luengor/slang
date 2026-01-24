auto fn = (fixed a) -> fixed {
    self = (fixed a) -> fixed {
        return a + 1;
    };
    return self(a);
};

# expect: COMPERROR
print(fn(1));

