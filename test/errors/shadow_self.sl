# expect: COMPERROR
# self should not be allowed to be shadowed

auto self = 1;

auto fn = () -> none {
    auto self = 1;
};
