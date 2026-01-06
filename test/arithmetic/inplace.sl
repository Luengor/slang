fixed a = 10;

# expect:  -10
a = -a;
print(a);

# expect:  5
a = a + 15
print(a);

# expect:  10
a = a * 2
print(a);

# expect:  5
a = a / 2
print(a);

