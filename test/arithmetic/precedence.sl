# * has higher precedence than +.
# expect: 14
print(2 + 3 * 4);

# * has higher precedence than -.
# expect: 8
print(20 - 3 * 4);

# / has higher precedence than +.
# expect: 4
print(2 + 6 / 3);

# / has higher precedence than -.
# expect: 0
print(2 - 6 / 3);

# < has higher precedence than ==.
# expect: true
print(false == 2 < 1);

# > has higher precedence than ==.
# expect: true
print(false == 1 > 2);

# <= has higher precedence than ==.
# expect: true
print(false == 2 <= 1);

# >= has higher precedence than ==.
# expect: true
print(false == 1 >= 2);

# 1 - 1 is not space-sensitive.
# expect: 0
print(1 - 1);

# expect: 0
print(1 -1); 

# expect: 0
print(1- 1); 

# expect: 0
print(1-1);  


# Using () for grouping.
# expect: 4
print((2 * (6 - (2 + 2))));

