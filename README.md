# NamedParams

NamedParams is a header-only C++17 library that allows to use unordered named parameters for functions, in addition to compile-time type checking of optional arguments.

```
int result = function(var0 = 0, var2 = 2, var1 = 1);
```

## Motivation

Let's suppose we have a function with a long list of parameters such as
```
void calculateWaveFunction(Wavefunctiom* _wavefunction, 
                            const std::vector<Atom>& _atoms,
                            const Basis& _basis,
                            const int _method,
                            std::optional<Basis> _dfBasis,
                            std::optional<int> _scfMaxIter,
                            std::optional<int> _diisMaxIter,
                            std::optional<int> _nbBatches, 
                            std::optional<std::string> _guess,
                            std::optional<double> _diagThreshold,
                            std::optional<double> _scaling,
                            std::optional<double> _orthoDiis);
```

If we like to call that function, we will need to pass std::nullopt for each parameters that we do not want to use:

```
calculateWaveFunction(&wavefunction, atoms, basis, method, std::nullopt, std::nullopt, ..., std::nullopt);
```

There are several ways to make using that function a bit easier on the wrist. First, we pass std::nullopt as default argument to each optional argument. Secondly, we pack everything into a struct. We can then call the function like this

```
struct Parameters 
{
  Wavefunction* wavefunction;
  std::vector<Atoms>& _atoms;
  ...
  std::optional<double> orthoDiis = std::nullopt;
};

...

// init non-optional parameters
Parameters p = {&wavefunction, atoms, basis, method};

// rest of the arguments as needed
p.nbBatches = 5;
p,scaling = 5.0;

calculateWavefunction(p);
```

In C++20 we can even do
```
calculateWavefuncton(Parameters{
                                .wavefunction = &wavefunction, 
                                .atoms = atoms,
                                .basis = basis,
                                .method = method,
                                .nbBatches = 5,
                                .scaling = 5.0
                              });
```

using named initializers.

However, apart from forcing every required parameter in the struct to be initialized as a reference, there is no direct way to have a compile-time checking of required vs optional parameters. You have to do the check at run-time. You might omit ```.basis``` or ```.method```, without the compiler minding at all. Also, all members need to be in the correct order.

In that context, I always liked the way Fortran or Python handle it. We can differentiate between positionals and named parameters. With a feature like that, we could pass the arguments like

```
CALCULATE_WAVE_FUNCTION(wavefunction, atoms, basis, method, scaling=5.0,                     
                       nbBatches = 6);
```

There have been some attempts to emulate named parameters, such as Boost::Parameter. However, they do not offer compile time errors, and often need a lot of boilerplate code.


## Example

Enter: the NamedParams library! First we define a list of all the parameters we want:
```
#define VARS (kWaveFunction, kAtoms, kBasis, kMethod, kDfBasis, kJMethod, kKMethod, kEris, \
              kThreshold, kDoDiis, kDoLocal, kScfMaxIter, kDiisMaxIter, kNbBatches, kGuess, \
              kDiagThreshold, kScaling, kOrthoDiis)
```

Then we use another macro to declare a new std::function-like object we can call later:

``` 
PARAMETRIZE(namedFunction, &calculateWavefunction, VARS);
```

And that's it! Now we can call it in our program, using different ways:

```
// using all named parameters
namedFunction(kWavefunction = &wavefunction, kAtoms = atoms, kBasis = basis, 
              kMethod = method, kNbBatches = 5, kScaling = 5.0);

// positionals with unordered optional arguments
namedFunction(&wavefunction, atoms, basis, method, kScaling = 5.0, kNbBatches = 5);
```

It is not very straightforward to implement, but is also not a novel idea. However, if you actually omit a required parameter, it will fail at compile time.

```
namedFunction(&wavefunction, atoms, method, kScaling = 5.0, kNbBatches = 5);
```

Gives you the following error in GCC:
```
In instantiation of ‘constexpr void NamedParams::failWithMessage() [with NamedParams::ErrorType error = NamedParams::ErrorType::COULD_NOT_CONVERT_KEY_TYPE_TO_ARGUMENT_TYPE; auto ...Var = {_KEY_kBasis}]’:
...
[a lot of template error non-sense ..]
...
static_assert((error != ErrorType::COULD_NOT_CONVERT_KEY_TYPE_TO_ARGUMENT_TYPE));
``` 

It will actually tell you what you did wrong, and when applicable, what key the error is referring to! It checks for correct type, correct number of arguments, correct number of required arguments, invalid keys, etc... Neat!

You can use the above syntax for static member functions as well. It complements the builder function design pattern well, at least in my oppinion. 

Setting up the same thing for non-static member functions is a bit more involved. Please have a look at TestNamedParams.cpp on how to do that.

## How It Works

The ```PARAMETRIZE``` macro does several things. First, it actually declares each key and adds an enum:
```
enum _ENUM_wavefunction {     
    _KEY_wavefunction          
  };                      
const inline static NamedParams::Key<wavefunction*, UNIQUE(wavefunction), 
                                     _KEY_wavefunction> kWavefunction;
```

The enum is useful to get better compile-time errors. UNIQUE is a macro that combines the name of the key, with ```__LINE__``` and ```__TIME__``` to get a (hopefully) unique 64-bit integer ID. Each key in one function needs to have a unique ID for everything to work correctly. You can probably use ```__COUNTER__```, but that macro is not part of the C++ standard. 

Then, the actual function object is created:
```
inline const NamedParams::KeyFunction functionName(&function, kWaveFunction, kAtoms, ...);
```
 
The ```KeyFunction``` class is a variadic template class which takes in the key types and IDs, and does some prechecking for types and number of parameters. 

The underlying function pointer is then called with ```operator()``` which itself is a variadic template function. What ```key = variable``` does, is create a new type ```AssignedKey``` which contains the address of the variable. Using templates and constexpr functions, we can reorder the types and check if the passed arguments are all valid

## Overhead

I have not done any extensive benchmarking, so take this with a grain of salt. Given that most stuff is evaluated at compile time, the overhead should be relatively small. Using clang, I generally observe that passing arguments via the KeyFunction object takes about 5 to 20 times longer (on the scale of 1e-7 seconds) than just passing it to the function itself (about 1e-8 seconds). So if your function is not called very often, or has a significantly larger runtime than a few microseconds, it should be ok.

## But... why?

I know, most people don't get into that situation where you have a million parameters in a function. Most of the time, it is better to organize it into larger structs. This was mainly a personal project to see what is possible in C++. If you can get some use out of it, great! But this allowed me to learn a lot about template, constexpr and macro magic.
