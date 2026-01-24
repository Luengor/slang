# expect: COMPERROR
# self should not be allowed to be shadowed

auto fn = () -> none {
    auto self = 1;
};
