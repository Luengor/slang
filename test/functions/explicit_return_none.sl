auto fn = (bool skip) -> none {
    if (skip) return;
    print("ran");
};

# expect: ran
fn(false);

fn(true);
