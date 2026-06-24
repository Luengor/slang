# function types are not castable

auto f = () -> none {};
print(f as fixed);

# expect: COMPERROR
