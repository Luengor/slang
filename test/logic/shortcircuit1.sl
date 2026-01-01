bool should_be_true = true;
true or (should_be_true = false); # the assignment should not happen

# expect: true 
print(should_be_true);
