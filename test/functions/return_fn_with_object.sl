auto get_fn = (str obj) -> () -> none {
    return () -> none {};
};

auto fn = get_fn("example");
fn();

