# PYTHON SCRIPT FOR GENERATING MACROS:
NMAX = 32

listfront = ["_" + str(i) + ", " for i in range(0,NMAX+1)]
listback = [str(i) + "," for i in range(NMAX,0,-1)]

##### NARGS #####

print("#define NARGS_SEQ(", end='')
for ele in listfront:
    print(ele, end='')
print("N, ...) N", end='\n')
print("#define NARGS(...) NARGS_SEQ(0, __VA_ARGS__,", end='')
for ele in listback:
    print(ele, end='')
print("0)",end='\n')
print('\n')

##### ITERATE #####

print("#define ITERATE_LIST(FUNC, DELIM, SUFFIX, constant, list) ITERATE(FUNC, DELIM, SUFFIX, constant, UNPAREN list)", end='\n')
print("#define ITERATE(FUNC, DELIM, SUFFIX, constant, ...) CAT(_ITERATE_, NARGS(__VA_ARGS__))(FUNC, NARGS(__VA_ARGS__), DELIM, SUFFIX, constant, __VA_ARGS__)", end='\n')
print("#define _ITERATE_0(FUNC, NELE, DELIM, SUFFIX, constant, x) ", end='\n')
print("#define _ITERATE_1(FUNC, NELE, DELIM, SUFFIX, constant, x) FUNC(constant, x, 0, NELE) UNPAREN SUFFIX", end='\n')
for i in range(2,NMAX+1):
    print("#define _ITERATE_" + str(i) + "(FUNC, NELE, DELIM, SUFFIX, function, x, ...) FUNC(function, x," + str(i-1) + ", NELE) UNPAREN DELIM _ITERATE_" +  str(i-1) + "(FUNC, NELE, DELIM, SUFFIX, function, __VA_ARGS__)")
print('\n')
