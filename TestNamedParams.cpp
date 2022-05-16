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
 
int foo(float a, int& b)
{
  b = 5;
  return a + b;
}

KEY((float),0) pa;
KEY((int&),1) pb;
KEYGEN fooWrapper(&foo,pa,pb);

int foo2(int a, std::optional<int> b, std::string c)
{
  return (b) ? *b + a : a;
}

KEY((int),2) key0;
KEYOPT((int),3) key1;
KEY((std::string),4) key2;
KEYGEN foo2Wrapper(&foo2, key0, key1, key2);

std::string foo3(std::optional<std::string> s)
{
  return s ? *s : "";
}

KEYOPT((std::string),5) key3;
KEYGEN foo3Wrapper(&foo3, key3);

class Test
{
  public:

    int m_int;
    float m_float;
    std::string m_str;

    KEY((int), 0) param0;
    KEY((float), 1) param1;
    KEYOPT((std::string),2) param2;

    KEY((int), 3) param3;
    KEY((int), 4) param4;
    KEY((float), 5) param5;
    KEYOPT((int), 6) param6;

    Test(int _i, float _f, std::string _s) 
      : m_int(_i)
      , m_float(_f)
      , m_str(_s)
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

    KEYGEN buildWrapper = KeyGen(&Test::build, param0, param1, param2);

    int compute(int _a, int _b, float _c, std::optional<int> _d)
    {
      if (_d)
      {
        return m_int + _a + _b + _c + *_d;
      }
      else 
      {
        return m_int + _a + _b + _c;
      }
    }

};

int main(int argc, char** argv)
{
  int result = 0;

  std::cout << UNIQUE << std::endl;
  std::cout << UNIQUE << std::endl;

  int c = 3;

  int ret = fooWrapper(pb = c, pa = 2);
  CHECK_EQUAL(ret, 7, result);
  CHECK_EQUAL(c, 5, result);
 
  int ret2 = foo2Wrapper(key0 = 1, key2 = "HELLO", key1 = 1);
  CHECK_EQUAL(ret2, 2, result);

  int ret3 = foo2Wrapper(key0 = 1, key2 = "HELLO");
  CHECK_EQUAL(ret3, 1, result);

  std::string ret4 = foo3Wrapper();
  CHECK_EQUAL(ret4, "", result);
  
  Test t0 = Test::buildWrapper(Test::param1 = 3.14, Test::param2 = "HELLO", Test::param0 = 1);
  CHECK_EQUAL(t0.m_int, 1, result);
  CHECK_ALMOST_EQUAL(t0.m_float, 3.14, result);
  CHECK_EQUAL(t0.m_str, "HELLO", result);

  /*Test t1 = Test::buildWrapper(Test::param0 = 5, Test::param1 = 6.28); // TAU GANG REPRESENT
  CHECK_EQUAL(t1.m_int, 5, result);
  CHECK_ALMOST_EQUAL(t1.m_float, 6.28, result);
  CHECK_EQUAL(t0.m_str, "", result);*/

  // FIX SAME KEY

  return result;

}