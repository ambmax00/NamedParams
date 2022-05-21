#include "NamedParams.h"
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

#define UNPAREN(...) __VA_ARGS__ 
#define KEY(TYPE, ID) inline static const Key< UNPAREN TYPE , ID > 
#define KEYOPT(TYPE, ID) inline static const Key<std::optional< UNPAREN TYPE >, ID >
#define KEYGEN inline static const KeyGen

#define _MEMKEYGEN() 
#define MEMKEYGEN()
 
int foo(float a, int& b)
{
  b = 5;
  return a + b;
}

KEY((float),0) pa;
KEY((int&),1) pb;
//KEYGEN 
inline static const KeyGen fooWrapper(&foo,pa,pb);

int foo1(int a, std::optional<int> b, std::string c)
{
  return (b) ? *b + a : a;
}

int foo2(int a, int b, std::optional<int> c)
{
  return (c) ? *c + a + b : a + b;
}

KEY((int),2) key0;
KEYOPT((int),3) key1;
KEY((std::string),4) key2;
KEY((int),5) key3;
KEYOPT((int),6) key4;

KEYGEN foo1Wrapper(&foo1, key0, key1, key2);
KEYGEN foo2Wrapper(&foo2, key0, key3, key4);

std::string foo3(std::optional<std::string> s)
{
  return s ? *s : "";
}

int func(int a, int& b, std::optional<int> c, std::optional<int> d)
{
  return a + b + (c ? *c : 0) + (d ? *d : 0);
}

KEY((int),10) paramA;
KEY((int&),11) paramB;
KEYOPT((int),12) paramC;
KEYOPT((int),13) paramD;
KEYGEN funcWrap(&func, paramA, paramB, paramC, paramD);

KEYOPT((std::string),7) key6;
KEYGEN foo3Wrapper(&foo3, key6);

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

    KEYGEN buildWrapper = KeyGen(&Test::build, paramI, paramF, paramS);

    int compute(int _a, int _b, float& _c, std::optional<int> _d)
    {
      if (_d)
      {
        _c = 6.0;
        return m_int + _a + _b + _c + *_d;
      }
      else 
      {
        _c = 6.0;
        return m_int + _a + _b + _c;
      }
    }

    const KeyGenClass<
      Test, 
      decltype(&Test::compute),
      decltype(paramA),
      decltype(paramB),
      decltype(paramC),
      decltype(paramD)> 
    computeW;
  
};

int main(int argc, char** argv)
{
  int result = 0;

  int c = 3;

  int ret = fooWrapper(pb = c, pa = 2);
  CHECK_EQUAL(ret, 7, result);
  CHECK_EQUAL(c, 5, result);
 
  int ret2 = foo1Wrapper(key2 = "HELLO", key1 = 1, key0 = 1);
  CHECK_EQUAL(ret2, 2, result);

  int ret3 = foo1Wrapper(key0 = 1, key2 = "HELLO");
  CHECK_EQUAL(ret3, 1, result);

  int ret4 = foo2Wrapper(key0 = 1, key3 = 3);
  CHECK_EQUAL(ret4, 4, result);

  int d = 5;

  ret4 = funcWrap(1,d,paramD=5,paramC=4);
  CHECK_EQUAL(ret4, 15, result);

  //int retNone = foo2Wrapper(key0 = 1, key0 =2);

  std::string ret5 = foo3Wrapper();
  CHECK_EQUAL(ret5, "", result);
  
  Test t0 = Test::buildWrapper(Test::paramF = 3.14, Test::paramS = "HELLO", Test::paramI = 1);
  CHECK_EQUAL(t0.m_int, 1, result);
  CHECK_ALMOST_EQUAL(t0.m_float, 3.14, result);
  CHECK_EQUAL(t0.m_str, "HELLO", result);

  float val = 3.0;
  int ret6 = t0.computeW(Test::paramA = 1, Test::paramB = 2, Test::paramC = val, Test::paramD = 4);

  CHECK_EQUAL(ret6, t0.m_int + 13, result);
  CHECK_ALMOST_EQUAL(val, 6.0, result);

  return result;

}