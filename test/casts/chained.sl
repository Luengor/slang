# casts chain left to right without parens

# (1 as bool) is true, then true as fixed = 1
# expect: 1
print(1 as bool as fixed);

# 3.9 -> fixed (3) -> float (3.0)
# expect: 3.000000
print(3.9 as fixed as float);

# 7 -> float (7.0) -> bool (true) -> fixed (1)
# expect: 1
print(7 as float as bool as fixed);

# parens are still allowed and equivalent
# expect: 1
print((1 as bool) as fixed);
