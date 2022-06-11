#include "../NamedParams.h"
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

PARAM(char0, char)
PARAM(char1, char)
PARAM(char2, char)
PARAM(char3, char)

PARAMETRIZE(word, char0, char1, char2, char3)

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

    inline static Key<int,3> paramA;
    inline static Key<int,4> paramB;
    inline static Key<float&,5> paramC;
    inline static Key<std::optional<int>,6> paramD;

    Test(int _i, float _f, std::string _s) 
      : m_int(_i)
      , m_float(_f)
      , m_str(_s)
      , computeW(this, &Test::compute,paramA,paramB,paramC,paramD)
    {
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

    const KeyGenClass<
      decltype(&Test::compute),
      decltype(paramA),
      decltype(paramB),
      decltype(paramC),
      decltype(paramD)> 
    computeW;
  
};

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
  int ret6 = t0.computeW(1, 2, Test::paramC = val, Test::paramD = 4);

  CHECK_EQUAL(ret6, 0, result);
  CHECK_ALMOST_EQUAL(val, 11.0, result);

  //testKey.test<0>();

  return result;

}