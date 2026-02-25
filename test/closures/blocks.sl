{
    auto i = 0;
    auto closure = () -> none {print(i);};
    # expect: 0
    closure();
}

{
    auto i = 1;
    auto closure = () -> none {print(i);};
    # expect: 1
    closure();
}


