# plain string
"hello";

# string in a block
{
    "hello";
}

# string in a variable declaration
auto a = "hello";

# overwrite variable with another string
a = "world";

# The same in a block
{
    auto a = "hello";

    a = "world";
}

