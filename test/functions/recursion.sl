auto fib = (fixed n) -> fixed {
    if (n <= 1) return n;
    return self(n - 1) + self(n - 2);
};

# expect: 55
print(fib(10));

