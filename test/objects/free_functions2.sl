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

