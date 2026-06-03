auto a = 3 == 4;
# expect: false
print(a);

auto b = 3 == 3;
# expect: true
print(b);

auto c = 3 != 4;
# expect: true
print(c);

auto d = 3 != 3;
# expect: false
print(d);
