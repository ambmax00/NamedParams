#ifndef NAMED_PARAMS_H
#define NAMED_PARAMS_H

#include <any>
#include <cstring>
#include <iostream>
#include <functional>
#include <tuple>
#include <typeindex>
#include <vector>
#include <utility>

template <class T>
struct IsOptional : public std::false_type {};

template <class T>
struct IsOptional<std::optional<T>> : public std::true_type {};

class Any
{
  private:

    uint8_t* m_pStorage;
    std::type_index* m_pTypeIndex;
    bool m_isReference;

    Any(uint8_t* _pStorage, std::type_index* _pTypeIndex, bool _isReference)
      : m_pStorage(_pStorage)
      , m_pTypeIndex(_pTypeIndex)
      , m_isReference(_isReference)
    {
      //std::cout << "KEY NR: " << m_keyID << std::endl;
    }

  public:

    Any() = delete;

    template <class T, std::enable_if_t<std::is_reference<T>::value, int> = 0>
    static Any build(T _any)
    {
      std::cout << "INIT REF " << std::endl;
      return Any(reinterpret_cast<uint8_t*>(&_any), new std::type_index(typeid(T)), true);
    }

    template <class T, std::enable_if_t<!std::is_reference<T>::value, bool> = true>
    static Any build(T _any)
    {
      std::cout << "INIT NON-REF " << std::endl;
      auto pStorage = new uint8_t[sizeof(T)];
      auto pTypeIndex = new std::type_index(typeid(T));
      std::memcpy(pStorage, &_any, sizeof(T));

      return Any(pStorage, pTypeIndex, false);
    }

    Any(const Any& _key) = delete;

    Any(Any&& _input)
    {
      m_pStorage = _input.m_pStorage;
      m_pTypeIndex = _input.m_pTypeIndex;
      m_isReference = _input.m_isReference;
      _input.m_pStorage = nullptr;
      _input.m_pTypeIndex = nullptr;
    }

    ~Any()
    {
      if (m_isReference)
      {
        std::cout << "IS REFERENCE" << std::endl;
      }
      else 
      {
        std::cout << "IS NOT REFERENCE" << std::endl;
      }

      if (m_pStorage && !m_isReference) 
      {
        delete [] m_pStorage;
        m_pStorage = nullptr;
      }

      if (m_pTypeIndex)
      {
        delete m_pTypeIndex;
      }

    }

    uint8_t* getStorage() 
    {
      return m_pStorage;
    }

    const std::type_index* getTypeIndex()
    {
      return m_pTypeIndex;
    }

    /*const int getKeyID()
    {
      return m_keyID;
    }*/

};

template <typename T, std::enable_if_t<!std::is_reference<T>::value, bool> = true>
T any_cast(Any& _any)
{
  const std::type_index castType = typeid(T);
  if (castType != *_any.getTypeIndex())
  {
    std::cout << "WRONG" << std::endl;
  } 

  std::cout << "REINTERPRET NON REF" << std::endl;
 
  T out;
  std::memcpy(&out, _any.getStorage(), sizeof(T));
  return out;
}

template <typename T, std::enable_if_t<std::is_reference<T>::value, bool> = true>
T any_cast(Any& _any)
{
  const std::type_index castType = typeid(T);
  if (castType != *_any.getTypeIndex())
  {
    std::cout << "WRONG" << std::endl;
  } 

  std::cout << "REINTERPRET NON REF" << std::endl;

  T out = *reinterpret_cast<typename std::remove_reference<T>::type*>(_any.getStorage());

  return out;
}

#if 0
class KeyCounter 
{
  private:
    static inline int m_globalKeyIndex = 0;

  public:
    static const int getKeyID()
    {
      return m_globalKeyIndex++;
    }
}
#endif

template <typename T>
class Key
{
  private:
    //m_keyID; 

  public:
    constexpr Key() {} //: m_keyID(KeyCounter::getKeyID()) {}

    Key(const Key& _other) = delete;

    Key(Key&& _other) = delete;

    constexpr Any operator=(T _any) const 
    {
      //std::cout << "TYPE: " << std::is_reference<T>::value << " " << std::type_index(typeid(T)).name() << std::endl;
      std::cout << "SHOULD NOT BE CALLED" << std::endl;
      return Any::build<T>(_any);
    }

};

template<typename T> 
struct FunctionTraits;  

template<typename R, typename ...Args> 
struct FunctionTraits<std::function<R(Args...)>>
{
    static const size_t nbArgs = sizeof...(Args);

    typedef R ResultType;

    template <size_t i>
    struct arg
    {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
    };
};

template <typename Func>
class KeyGen
{
  private:

    typedef FunctionTraits<std::function<Func>> KeyFunctionTraits;

    template <class... KeyType>
    typename KeyFunctionTraits::ResultType m_parameterFunction(KeyType... args) 
    {
      return 0;
    }

    std::function<Func> m_baseFunction; 

  public:
    KeyGen(Func* _function) : m_baseFunction(_function)
    {
    }

    ~KeyGen() {}

    std::function<Func> getBaseFunction()
    {
      return m_baseFunction;
    }

    template <class... KeyTypePack>
    typename KeyFunctionTraits::ResultType operator()(KeyTypePack... _args)
    {
      std::tuple<KeyTypePack...> arr = std::make_tuple(std::move(_args)...);
      return internal(arr, std::make_index_sequence<sizeof...(_args)>{});
    }

    template <class... KeyTypePack, size_t... Is>
    typename KeyFunctionTraits::ResultType 
    internal(std::tuple<KeyTypePack...>& arr, std::index_sequence<Is...> const &)
    {
      return m_baseFunction(
          any_cast<typename KeyFunctionTraits::arg<Is>::type>(std::get<Is>(arr))...);
    }

};

#if 0
class Register
{
  private: 
    
    inline static std::map<uint8_t*,std::vector<int>> m_functionKeyMap;

  public:

    Register() = delete;
    ~Register() {}

    template <class T, class... KeyTypes>
    static void addKeys(Key<T>& _key, KeyTypes... args)
    {

    }

}
#endif

#endif // NAMED_PARAMS_H
