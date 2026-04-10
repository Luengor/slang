fixed i = 0;
print(i);
while (i < 3) {
    i = i + 1;
    auto j = i;
    auto closure = () -> none { print(j); };
    closure();
}

# expect: 0
# expect: 1
# expect: 2
# expect: 3

