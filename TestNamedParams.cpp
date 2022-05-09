#include "NamedParams.h"

int foo(float a, int& b)
{
  std::cout << "+" << a << " " << b << std::endl;
  b = 5;
  return a + b;
}

inline static const Key<float> pa;
inline static const Key<int&> pb;

int main(int argc, char** argv)
{

  int res = 0;

  int c = atoi(argv[1]);

  KeyGen gen(&foo);

  int ret = gen(pa = 2, pb = c);
  
  // correct result
  res += (ret == 7) ? 0 : 1;

  // correct reference
  res += (c == 5) ? 0 : 1;

  return res;

}