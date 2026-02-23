auto get_str = () -> str {
    auto hello = "hello";
    return hello;
};

# expect: hello
print(get_str());

