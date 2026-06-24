# casts to/from bool

# zero is false, anything else is true
# expect: false
print(0 as bool);

# expect: true
print(7 as bool);

# expect: true
print(-2 as bool);

# expect: false
print(0.0 as bool);

# bool to numbers
# expect: 1
print(true as fixed);

# expect: 0
print(false as fixed);

# expect: 1.000000
print(true as float);
