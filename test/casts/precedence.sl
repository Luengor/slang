# cast binds tighter than * and /, but looser than unary

# (3.5 as fixed) is 3, then 2 * 3 = 6
# expect: 6
print(2 * 3.5 as fixed);

# unary applies before cast: (-3.9) as fixed = -3
# expect: -3
print(-3.9 as fixed);

# (5 as float) / 2.0 = 2.5
fixed x = 5;
# expect: 2.500000
print(x as float / 2.0);
