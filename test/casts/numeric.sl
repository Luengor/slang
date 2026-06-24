# fixed <-> float casts

# expect: 3
print(3.9 as fixed);

# truncation is toward zero
# expect: -3
print(-3.9 as fixed);

# expect: 5.000000
print(5 as float);

# expect: 0
print(0.9 as fixed);
