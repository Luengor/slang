auto Hello_World = ((str) -> none print_f) -> none {
    print_f("Hello, World!");
};

# expect: Hello, World!
Hello_World(print);

