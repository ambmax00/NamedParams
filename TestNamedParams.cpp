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

    /*KEY((int), 3) paramA;
    KEY((int), 4) paramB;
    KEY((float), 5) paramC;
    KEYOPT((int), 6) paramD;*/

    inline static Key<int,3> paramA;
    inline static Key<int,4> paramB;
    inline static Key<float,5> paramC;
    inline static Key<int,6> paramD;

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

    KEYGEN buildWrapper = KeyGen(&Test::build, paramI, paramF, paramS);

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

    //const KeyGenClass computeW = KeyGenClass(this, &Test::compute,paramA,paramB,paramC,paramD);

    /*const KeyGen<int(Test::*)(int,int,float,std::optional<int>),
      decltype(paramA), decltype(paramB), decltype(paramC),
      decltype(paramD)> 
    computeWrapper = KeyGen(&Test::compute, paramA, paramB, paramC, paramD);*/

};

template <typename FuncPtr> //, class... FunctionKeys>
class KeyGenTest
{
  private:

    typedef FunctionTraits<typename std::remove_pointer<FuncPtr>::type> KeyFunctionTraits;

    FuncPtr m_baseFunction; 

    std::array<int, KeyFunctionTraits::nbArgs> m_keyIDs;

  public:

    KeyGenTest(FuncPtr f) 
      : m_baseFunction(f)
      //, m_keyIDs({_keys.ID...})
    {
      //ConstExprError<2> test;
    }

    /*template <typename FunctionPointer, 
      std::enable_if_t<std::is_member_function_pointer<FunctionPointer>::value,bool> = true>
    KeyGenTest(FunctionPointer _function) // const FunctionKeys&... _keys)
    {
      ConstExprError<3> test;
    }*/

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
 
  int ret2 = foo1Wrapper(key0 = 1, key2 = "HELLO", key1 = 1);
  CHECK_EQUAL(ret2, 2, result);

  int ret3 = foo1Wrapper(key0 = 1, key2 = "HELLO");
  CHECK_EQUAL(ret3, 1, result);

  int ret4 = foo2Wrapper(key0 = 1, key3 = 3);
  CHECK_EQUAL(ret4, 4, result);

  //int retNone = foo2Wrapper(key0 = 1, key0 =2);

  std::string ret5 = foo3Wrapper();
  CHECK_EQUAL(ret5, "", result);
  
  Test t0 = Test::buildWrapper(Test::paramF = 3.14, Test::paramS = "HELLO", Test::paramI = 1);
  CHECK_EQUAL(t0.m_int, 1, result);
  CHECK_ALMOST_EQUAL(t0.m_float, 3.14, result);
  CHECK_EQUAL(t0.m_str, "HELLO", result);

  auto testCompute = &Test::compute;
  (t0.*testCompute)(5,6,7.0,8.0);

  KeyGenTest b(&foo1); //, 5, 6, 7);

  KeyGenTest b2(&Test::compute); //, 5, 6, 7);

  //t0.*computeW(Test::paramA = 1, Test::paramB = 2, Test::paramC = 3, Test::paramD = 4);

  /*Test t1 = Test::buildWrapper(Test::param0 = 5, Test::param1 = 6.28); // TAU GANG REPRESENT
  CHECK_EQUAL(t1.m_int, 5, result);
  CHECK_ALMOST_EQUAL(t1.m_float, 6.28, result);
  CHECK_EQUAL(t0.m_str, "", result);*/

  // FIX SAME KEY

  return result;

}