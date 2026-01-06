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

