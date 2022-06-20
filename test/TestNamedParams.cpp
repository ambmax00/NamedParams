#include "../NamedParams.h"
#include <chrono>
#include <iostream>
#include <string>

#define CHECK_EQUAL(_A, _B, _RETURN) \
  if (_A != _B) \
  { \
    std::cerr << "Not equal: " << #_A << " " << #_B << std::endl; \
    _RETURN += 1; \
  } 

#define CHECK_ALMOST_EQUAL(_A, _B, _RETURN) \
  if (!(std::abs(_A - _B) < 1e-6)) \
  { \
    std::cerr << "Not equal: " << #_A << " " << #_B << std::endl; \
    _RETURN += 1; \
  } 

#define _MEMKEYGEN() 
#define MEMKEYGEN()

// function for testing correct order
std::string word(char _a, char _b, char _c, char _d)
{ 
  std::string out = {_a, _b, _c, _d};
  return out;
}

#define WORD_VARS (char0, char1, char2, char3)
PARAMETRIZE_NEW(word, WORD_VARS)

// function for testing reference passing
void concat(std::string& _str0, std::string& _str1)
{
  _str0 += _str1;
}

PARAM(str0, std::string&)
PARAM(str1, std::string&)
PARAMETRIZE(concat, str0, str1)

// function for testing optional params
int sum(int _a, const int _b, std::optional<int> _c, std::optional<int> _d, std::optional<int> _e) 
{
  return _a + _b + (_c ? *_c : 0) + (_d ? *_d : 1) + (_e ? *_e : 2);
}

PARAM2(keyA, int)
PARAM2(keyB, const int)
OPTPARAM(keyC, int)
OPTPARAM(keyD, int)
OPTPARAM(keyE, int)

PARAMETRIZE(sum, keyA, keyB, keyC, keyD, keyE)

int sumPointer(int* p0, int const* p1, const int* p2)
{
  return *p0 + *p1 + *p2;
}

PARAM2(keyP0, int*)
PARAM2(keyP1, int const*)
PARAM2(keyP2, const int*)

PARAMETRIZE(sumPointer, keyP0, keyP1, keyP2)

int singleArgument(std::optional<int> _i)
{
  return _i ? *_i : 0;
}

OPTPARAM(single, int)
PARAMETRIZE(singleArgument, single)

class Uncopyable
{
  public:
    Uncopyable() {};
    Uncopyable(const Uncopyable&) = delete;
    Uncopyable& operator=(const Uncopyable&) = delete;
};

int processUncopyable([[maybe_unused]]Uncopyable& _ucopy)
{
  return 0;
}

PARAM(pcopy, Uncopyable&)
PARAMETRIZE(processUncopyable, pcopy)

int foo(float a, int& b)
{
  b = 5;
  return a + b;
}

class Test
{
  public:

    int m_int;
    float m_float;
    std::string m_str;

    KEY((int), 0) paramI;
    KEY((float), 1) paramF;
    KEYOPT((std::string),2) paramS;

    Test(int _i, float _f, std::string _s) 
      : m_int(_i)
      , m_float(_f)
      , m_str(_s)
      , np_compute()
    {
      np_compute.setFunction(&Test::compute);
      np_compute.setClassPtr(this);
    }
  
    static Test build(int _i, float _f, std::optional<std::string> _strOpt)
    {
      if (_strOpt)
      {
        return Test(_i, _f, *_strOpt);
      }
      else 
      {
        return Test(_i, _f, "");
      }
    }

    KEYGEN buildWrapper = KeyGenClass(&Test::build, paramI, paramF, paramS);

    int compute(int _a, int _b, float& _c, std::optional<int> _d)
    {
      if (_d)
      {
        _c = m_int + _a + _b + _c + *_d;
        return 0;
      }
      else 
      {
        _c = m_int + _a + _b + _c;
        return 1;
      }
    }

    #define COMPUTE_LIST (paramA, paramB, paramC, paramD)
    DECLARE_KEYS(&Test::compute, COMPUTE_LIST)
    DECLARE_CLASS_FUNCTION(np_compute, &Test::compute, COMPUTE_LIST)

};

using intOpt = std::optional<int>;
#define ADD_OPT(name) \
  if (name) sum += *name;

int manyArgs(int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9,
             intOpt i10, intOpt i11, intOpt i12, intOpt i13, intOpt i14, intOpt i15, intOpt i16, 
             intOpt i17, intOpt i18, intOpt i19)
{
  int sum = 0;
  ADD_OPT(i10) 
  ADD_OPT(i11) 
  ADD_OPT(i12) 
  ADD_OPT(i13) 
  ADD_OPT(i14) 
  ADD_OPT(i15) 
  ADD_OPT(i16) 
  ADD_OPT(i17) 
  ADD_OPT(i18)
  ADD_OPT(i19) 

  return i0+i1+i2+i3+i4+i5+i6+i7+i8+i9 + sum;
}

PARAM2(keyI0, int)
PARAM2(keyI1, int)
PARAM2(keyI2, int)
PARAM2(keyI3, int)
PARAM2(keyI4, int)
PARAM2(keyI5, int)
PARAM2(keyI6, int)
PARAM2(keyI7, int)
PARAM2(keyI8, int)
PARAM2(keyI9, int)
OPTPARAM2(keyI10, int)
OPTPARAM2(keyI11, int)
OPTPARAM2(keyI12, int)
OPTPARAM2(keyI13, int)
OPTPARAM2(keyI14, int)
OPTPARAM2(keyI15, int)
OPTPARAM2(keyI16, int)
OPTPARAM2(keyI17, int)
OPTPARAM2(keyI18, int)
OPTPARAM2(keyI19, int)

PARAMETRIZE(manyArgs, keyI0, keyI1, keyI2, keyI3, keyI4, keyI5, keyI6, keyI7, keyI8, keyI9,
            keyI10, keyI11, keyI12, keyI13, keyI14, keyI15, keyI16, keyI17, keyI18,keyI19)


int main()
{

  

  int result = 0;

  std::string str = np_word(char2 = 'r', char1 = 'o', char3 = 'd', char0 = 'w');
  CHECK_EQUAL(str, "word", result);

  std::string a = "a";
  std::string b = "b";

  np_concat(str0 = a, str1 = b);

  CHECK_EQUAL(a, "ab", result);

  Uncopyable ucopy;
  int ret = np_processUncopyable(pcopy = ucopy);

  CHECK_EQUAL(0, ret, result);

  int sum = np_sum(keyA = 1, keyB = 2, keyD = 4);

  CHECK_EQUAL(sum, 9, result);

  int i0 = 0;
  const int i1 = 1;
  int i2 = 2;

  int sumP = np_sumPointer(keyP0 = &i0, keyP1 = &i1, keyP2 = &i2);

  CHECK_EQUAL(sumP, 3, result);
  
  Test t0 = Test::buildWrapper(Test::paramF = 3.14, Test::paramS = "HELLO", Test::paramI = 1);
  CHECK_EQUAL(t0.m_int, 1, result);
  CHECK_ALMOST_EQUAL(t0.m_float, 3.14, result);
  CHECK_EQUAL(t0.m_str, "HELLO", result);

  float val = 3.0;
  int ret6 = t0.np_compute(1, 2, Test::paramC = val, Test::paramD = 4);

  CHECK_EQUAL(ret6, 0, result);
  CHECK_ALMOST_EQUAL(val, 11.0, result);

  //testKey.test<0>();
  auto start = std::chrono::steady_clock::now();
  int sumArgs = np_manyArgs(keyI5 = 5, keyI0 = 0, keyI1 = 1, keyI2 = 2, keyI6 = 6, keyI7 = 7, keyI15 = 15,
                            keyI10 = 10, keyI3 = 3, keyI9 = 9, keyI8 = 8, keyI4 = 4, keyI16 = 16);
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;
  std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";  

  CHECK_EQUAL(sumArgs, 86, result);

  start = std::chrono::steady_clock::now();
  sumArgs = manyArgs(0,1,2,3,4,5,6,7,8,9,10,std::nullopt,std::nullopt,std::nullopt,std::nullopt,15,
                    16, std::nullopt, std::nullopt, std::nullopt);
  end = std::chrono::steady_clock::now();
  elapsed_seconds = end-start;
  std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";

  CHECK_EQUAL(sumArgs, 86, result);
  

  return result;

}