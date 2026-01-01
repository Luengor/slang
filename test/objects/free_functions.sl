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

# function in a function 
() -> none {
    () -> none {};
};

# call function containing another function
() -> none {
    () -> none {};
}();

# function in a function assigned to a variable
auto g = () -> none {
    () -> none {};
};

g();

# overwrite function variable with another function containing a function
g = () -> none {
    () -> none {};
};

g();

