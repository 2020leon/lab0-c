option echo 0
option verbose 1

# dummy sort
new
it RAND 262144
time sort

# user-version sort
option sort 0
new
it RAND 262144
time sort
new
it RAND 262144
time sort
new
it RAND 262144
time sort
new
it RAND 262144
time sort
new
it RAND 262144
time sort
# sort sorted list
time sort

# kernel-version sort
option sort 1
new
it RAND 262144
time sort
new
it RAND 262144
time sort
new
it RAND 262144
time sort
new
it RAND 262144
time sort
new
it RAND 262144
time sort
# sort sorted list
time sort
