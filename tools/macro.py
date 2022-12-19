# PYTHON SCRIPT FOR GENERATING MACROS:
NMAX = 32

listfront = ["_" + str(i) + ", " for i in range(0,NMAX+1)]
listback = [str(i) + "," for i in range(NMAX,0,-1)]

##### NARGS #####

print("#define _NAMEDPARAMS_NARGS_SEQ(", end='')
for ele in listfront:
    print(ele, end='')
print("N, ...) N", end='\n')
print("#define _NAMEDPARAMS_NARGS(...) \\\n  _NAMEDPARAMS_NARGS_SEQ(0, __VA_ARGS__,", end='')
for ele in listback:
    print(ele, end='')
print("0)",end='\n')
print('\n')

##### ITERATE #####

print("#define _NAMEDPARAMS_ITERATE_LIST(FUNC, DELIM, SUFFIX, constant, list) \\\n  _NAMEDPARAMS_ITERATE(FUNC, DELIM, SUFFIX, constant, _NAMEDPARAMS_UNPAREN list)", end='\n')
print("#define _NAMEDPARAMS_ITERATE(FUNC, DELIM, SUFFIX, constant, ...) \\\n  _NAMEDPARAMS_CAT(_NAMEDPARAMS_ITERATE_, _NAMEDPARAMS_NARGS(__VA_ARGS__))(FUNC, _NAMEDPARAMS_NARGS(__VA_ARGS__), DELIM, SUFFIX, constant, __VA_ARGS__)", end='\n')
print("#define _NAMEDPARAMS_ITERATE_0(FUNC, NELE, DELIM, SUFFIX, constant, x) ", end='\n')
print("#define _NAMEDPARAMS_ITERATE_1(FUNC, NELE, DELIM, SUFFIX, constant, x) \\\n  FUNC(constant, x, 0, NELE) _NAMEDPARAMS_UNPAREN SUFFIX", end='\n')
for i in range(2,NMAX+1):
    print("#define _NAMEDPARAMS_ITERATE_" + str(i) + "(FUNC, NELE, DELIM, SUFFIX, function, x, ...) \\\n  FUNC(function, x," + str(i-1) + ", NELE) _NAMEDPARAMS_UNPAREN DELIM \\\n  _NAMEDPARAMS_ITERATE_" +  str(i-1) + "(FUNC, NELE, DELIM, SUFFIX, function, __VA_ARGS__)")
print('\n')
