auto result = () -> fixed {
    return 42;
}();

# expect: 42
print(result);

# iife used directly as argument
# expect: hello
print(() -> str {
    return "hello";
}());
