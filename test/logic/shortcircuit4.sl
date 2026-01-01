bool should_be_false = true;
true and (should_be_false = false); # the assignment should happen
# expect: false 
print(should_be_false);
