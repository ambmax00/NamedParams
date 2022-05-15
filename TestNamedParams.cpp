#include "NamedParams.h"
#include <string>

#define UNPAREN(...) __VA_ARGS__ 
#define KEY(TYPE, ID) inline static const Key< UNPAREN TYPE , ID > 
#define KEYGEN inline static const KeyGen

inline static const Key<float,0> pa;
inline static const Key<int&,1> pb;
 
int foo(float a, int& b)
{
  std::cout << "+" << a << " " << b << std::endl;
  b = 5;
  return a + b;
}

KEYGEN fooWrapper(&foo,pa,pb);

int foo2(int a, std::optional<int> b, std::string c)
{
  std::cout << "IN FUNCTION " << c << std::endl;
  return (b) ? *b + a : a;
}

KEY((int),2) key0;
KEY((std::optional<int>),3) key1;
KEY((std::string),4) key2;

KEYGEN foo2Wrapper(&foo2, key0, key1, key2);

consteval int bar(const int& i)
{
  return i;
}

int main(int argc, char** argv)
{
  std::cout << &pa << " " << &pb << std::endl;

  int res = 0;

  int c = atoi(argv[1]);

  FORCE_UNIQUE(t0);
  FORCE_UNIQUE(t1);

  std::cout << *t0 << " " << *t1 << std::endl;
 
  std::cout << "NK: " << pa.ID << " " << pb.ID << std::endl;

  //Register::print();

  int ret = fooWrapper(pb = c, pa = 2);

  std::cout << "C: " << c << std::endl;
 
  int ret2 = foo2Wrapper(key0 = 1, key2 = "HELLO", key1 = 1);

  std::cout << "RETURNED" << std::endl;
  
  // correct result
  res += (ret == 7) ? 0 : 1;

  // correct reference
  res += (c == 5) ? 0 : 1;

  // correct optional
  res + (ret2 == 4) ? 0 : 1;

  return res;

}