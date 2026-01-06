# plain function
() -> none {};

# function in a block
{
    () -> none {};
}

# function in a variable declaration
auto f = () -> none {};

# overwrite variable with another function
f = () -> none {};

f();

# the same in a block
{
    auto f = () -> none {};

    f = () -> none {};

    f();
}

