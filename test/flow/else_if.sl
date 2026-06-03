auto check = (fixed n) -> none {
    if (n < 0) {
        print("negative");
    } else if (n == 0) {
        print("zero");
    } else {
        print("positive");
    }
};

# expect: negative
check(-1);

# expect: zero
check(0);

# expect: positive
check(1);
