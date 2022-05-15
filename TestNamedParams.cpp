#include "NamedParams.h"
#include <iostream>
#include <string>

#define CHECK_EQUAL(_A, _B, _RETURN) \
  if (_A != _B) \
  { \
    std::cerr << "Not equal: " << #_A << " " << #_B << std::endl; \
    _RETURN += 1; \
  } 

#define UNPAREN(...) __VA_ARGS__ 
#define KEY(TYPE, ID) inline static const Key< UNPAREN TYPE , ID > 
#define KEYGEN inline static const KeyGen

inline static const Key<float,0> pa;
inline static const Key<int&,1> pb;
 
int foo(float a, int& b)
{
  b = 5;
  return a + b;
}

KEYGEN fooWrapper(&foo,pa,pb);

int foo2(int a, std::optional<int> b, std::string c)
{
  return (b) ? *b + a : a;
}

KEY((int),2) key0;
KEY((std::optional<int>),3) key1;
KEY((std::string),4) key2;

KEYGEN foo2Wrapper(&foo2, key0, key1, key2);

std::string foo3(std::optional<std::string> s)
{
  return s ? *s : "";
}

KEY((std::optional<std::string>),5) key3;

KEYGEN foo3Wrapper(&foo3, key3);

int main(int argc, char** argv)
{
  int result = 0;

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
  
  return result;

}