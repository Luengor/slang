auto fn = (fixed left) -> fixed {
    if (left <= 0) {
        return left;
    }

    self(left - 1) + 1;
};

# expect: 3000
print(fn(3000));

