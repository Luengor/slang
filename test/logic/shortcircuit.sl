bool var = true;
true or (var = false); # the assignment should not happen

# expect: true 
print(var);

false and (var = false); # the assignment should not happen
# expect: true 
print(var);

false or (var = false); # the assignment should happen
# expect: false 
print(var);

true and (var = true);

# expect: true 
print(var);

