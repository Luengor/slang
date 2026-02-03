auto get_fn = (bool get_true) -> () -> bool {
    if (get_true) {
        return () -> bool {
            return true;
        };
    } else {
        return () -> bool {
            return false;
        };
    }
};

auto true_fn = get_fn(true);
auto false_fn = get_fn(false);

# expect: true
print(true_fn());

# expect: false
print(false_fn());

