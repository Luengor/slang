# explicit casts to str

# expect: 42
print(42 as str);

# expect: 3.140000
print(3.14 as str);

# expect: true
print(true as str);

# expect: false
print(false as str);

# str result can be stored and reprinted
str s = 100 as str;
# expect: 100
print(s);
