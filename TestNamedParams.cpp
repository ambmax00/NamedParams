#include "NamedParams.h"
#include <string>

inline static constexpr Key<float,0> pa;

inline static constexpr Key<int&,1> pb;
 
int foo(float a, int& b)
{
  std::cout << "+" << a << " " << b << std::endl;
  b = 5;
  return a + b;
}

inline static const KeyGen fooWrapper(&foo,pa,pb);

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
  
  // correct result
  res += (ret == 7) ? 0 : 1;

  // correct reference
  res += (c == 5) ? 0 : 1;

  return res;

}