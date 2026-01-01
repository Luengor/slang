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

# string in a function
() -> none {
    "hello";
};

# execute function with string
() -> none {
    "hello";
}();

# string in a function assigned to a variable
auto f = () -> none {
    "hello";
};

f();

# overwrite function variable with another function containing a string
f = () -> none {
    "world";
};

