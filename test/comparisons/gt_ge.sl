auto a = 5 > 3;
# expect: true
print(a);

auto b = 3 > 5;
# expect: false
print(b);

auto c = 3 > 3;
# expect: false
print(c);

auto d = 3 >= 3;
# expect: true
print(d);

auto e = 2 >= 3;
# expect: false
print(e);
