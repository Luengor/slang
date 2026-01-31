auto get_fn = (fixed a) -> () -> none {
    return () -> none {};
};

auto fn = get_fn(1);
fn();

