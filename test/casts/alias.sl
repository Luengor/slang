# casting to a type alias resolves to the aliased type

type myint = fixed;
type real = float;

# expect: 3
print(3.7 as myint);

# expect: 7.000000
print(7 as real);
