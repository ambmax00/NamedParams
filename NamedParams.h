#ifndef NAMED_PARAMS_H
#define NAMED_PARAMS_H

#include <array>
#include <cstring>
#include <iostream>
#include <functional>
#include <numeric>
#include <tuple>
#include <typeindex>
#include <vector>
#include <utility>

using id_value = const int *;

template <class T>
struct unique_id {
  static constexpr int value = 0;
  constexpr unique_id(T const &) {}
  constexpr operator id_value() const { return &value; }
};

/**
    The following is one of the base of this hack!
    This works because the conversion from unique_id to bool is delayed,
    therefore the lambda is a new one at each instantiation of a template
   depending on that non-type template which leads to 'name' to have a different
   value at each deduction
*/
#define FORCE_UNIQUE(name...) id_value name = unique_id([] {})

template <class T>
struct IsOptional : public std::false_type {};

template <class T>
struct IsOptional<std::optional<T>> : public std::true_type {};

template <class T, int N>
class Key;

template <class T>
struct IsKey : public std::false_type {};

template <class T, int N>
struct IsKey<Key<T,N>> : public std::true_type {};

template <class KeyType, std::enable_if_t<
  (IsKey<KeyType>::value || std::is_same<KeyType,void>::value),bool> = true>
class AssignedKey
{
  private:

    uint8_t* m_pStorage;
    std::type_index* m_pTypeIndex;
    bool m_isReference;
    bool m_isOptional;
    int m_keyID;

  public:

    AssignedKey() = delete;

    AssignedKey(uint8_t* _pStorage, std::type_index* _pTypeIndex, 
      bool _isReference, bool _isOptional, int _keyID)
      : m_pStorage(_pStorage)
      , m_pTypeIndex(_pTypeIndex)
      , m_isReference(_isReference)
      , m_isOptional(_isOptional)
      , m_keyID(_keyID)
    {
      //std::cout << "KEY NR: " << m_keyID << std::endl;
    }

    template <class T, std::enable_if_t<std::is_reference<T>::value, int> = 0>
    static AssignedKey build(T _any)
    {
      std::cout << "INIT REF " << std::endl;
      return AssignedKey(reinterpret_cast<uint8_t*>(&_any), 
        new std::type_index(typeid(T)), true, IsOptional<T>::value, KeyType::ID);
    }

    template <class T, std::enable_if_t<!std::is_reference<T>::value, bool> = true>
    static AssignedKey build(T _any)
    {
      std::cout << "INIT NON-REF " << std::endl;
      auto pStorage = new uint8_t[sizeof(T)];
      auto pTypeIndex = new std::type_index(typeid(T));
      std::memcpy(pStorage, &_any, sizeof(T));

      return AssignedKey(pStorage, pTypeIndex, false, IsOptional<T>::value, KeyType::ID);
    }

    AssignedKey(const AssignedKey& _key) = delete;

    AssignedKey(AssignedKey&& _input)
    {
      m_pStorage = _input.m_pStorage;
      m_pTypeIndex = _input.m_pTypeIndex;
      m_isReference = _input.m_isReference;
      m_isOptional = _input.m_isOptional;
      m_keyID = _input.m_keyID;
      _input.m_pStorage = nullptr;
      _input.m_pTypeIndex = nullptr;
    }

    AssignedKey& operator=(const AssignedKey& _input) = delete;

    AssignedKey& operator=(AssignedKey&& _input) 
    {
      m_pStorage = _input.m_pStorage;
      m_pTypeIndex = _input.m_pTypeIndex;
      m_isReference = _input.m_isReference;
      m_isOptional = _input.m_isOptional;
      m_keyID = _input.m_keyID;
      _input.m_pStorage = nullptr;
      _input.m_pTypeIndex = nullptr;
      return *this;
    }

    AssignedKey<void> moveToVoid() 
    {
      AssignedKey<void> out(m_pStorage, m_pTypeIndex, m_isReference, m_isOptional, m_keyID);
      m_pStorage = nullptr;
      m_pTypeIndex = nullptr;
      return out;
    }

    ~AssignedKey()
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
        std::cout << "FREEING " << m_keyID << std::endl;
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

    const std::type_index* getTypeIndex() const 
    {
      return m_pTypeIndex;
    }

    int getKeyID() const
    {
      return m_keyID;
    }

    typedef KeyType Type;

    //template <typename OtherKeyType>
    //friend class AssignedKey;


};

template <typename T, class KeyType, std::enable_if_t<!std::is_reference<T>::value, bool> = true>
T any_cast(AssignedKey<KeyType>& _any)
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

template <typename T, class KeyType, std::enable_if_t<std::is_reference<T>::value, bool> = true>
T any_cast(AssignedKey<KeyType>& _any)
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

template <class KeyType, std::enable_if_t<!std::is_reference<typename KeyType::type>::value, bool> = true>
inline auto cast(AssignedKey<KeyType>& _any)
{
  std::cout << "REINTERPRET NON REF" << std::endl;
 
  typename KeyType::type out;
  std::memcpy(&out, _any.getStorage(), sizeof(decltype(out)));
  return out;
}

template <class KeyType, std::enable_if_t<std::is_reference<typename KeyType::type>::value, bool> = true>
inline auto cast(AssignedKey<KeyType>& _any)
{
  typename KeyType::type out = *reinterpret_cast<
    typename std::remove_reference<typename KeyType::type>::type*>(_any.getStorage());

  return out;
}

template <typename T, int UNIQUE_ID> //FORCE_UNIQUE(UNIQUE_ID)>
class Key
{
  private:

  public:
    Key() {}

    Key(const Key& _other) = delete;

    Key(Key&& _other) = delete;

    auto operator=(T _any) const
    {
      //std::cout << "TYPE: " << std::is_reference<T>::value << " " << std::type_index(typeid(T)).name() << std::endl;
      std::cout << "SHOULD NOT BE CALLED" << std::endl;
      auto k = AssignedKey<Key>::template build<T>(_any);
      return k;
    }

    typedef T type;

    static const int ID = UNIQUE_ID;

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

template <typename Func, class... FunctionKeys>
class KeyGen
{
  private:

    typedef FunctionTraits<std::function<Func>> KeyFunctionTraits;

    Func* m_baseFunction; 

    std::array<int, KeyFunctionTraits::nbArgs> m_keyIDs;

  public:
    KeyGen(Func* _function, const FunctionKeys&... _keys) 
      : m_baseFunction(_function)
      //, m_keyIDs({_keys.ID...})
    {
    }

    ~KeyGen() {}

    std::function<Func> getBaseFunction() const
    {
      return m_baseFunction;
    }

    template <class... AssignedKeyPack>
    constexpr static inline int eval()
    {
      constexpr int nbAssignedKeys = sizeof...(AssignedKeyPack);
      constexpr int nbFunctionKeys = sizeof...(FunctionKeys);

      std::array<int,nbAssignedKeys> passedKeys = {AssignedKeyPack::Type::ID...};
      std::array<int,nbFunctionKeys> functionKeys = {FunctionKeys::ID...};

      std::array<bool,nbFunctionKeys> functionKeyIsOptional = {
        IsOptional<typename FunctionKeys::type>::value...};
      std::array<bool,nbAssignedKeys> passedKeyIsOptional = {
        IsOptional<typename AssignedKeyPack::Type::type>::value...};

      std::sort(passedKeys.begin(), passedKeys.end());
      int valid = 0;
      int offset = 0;

      for (int iArg = 0; iArg < nbFunctionKeys; ++iArg)
      {
        if (iArg >= nbAssignedKeys+offset && !functionKeyIsOptional[iArg])
        {
          valid = 1;
          break;
        }
        else if (functionKeys[iArg] > passedKeys[iArg-offset])
        {
          valid = 2;
          break;
        }
        else if (functionKeys[iArg] < passedKeys[iArg-offset] && !functionKeyIsOptional[iArg])
        {
          valid = 3;
          break;
        }
        else if (functionKeys[iArg] < passedKeys[iArg-offset] && functionKeyIsOptional[iArg])
        {
          offset++;
        }
      }

      if (valid == 1)
      {
        std::cout << "A REQUIRED KEY IS MISSING";
      }

      if (valid == 2)
      {
        std::cout << "INVALID KEY IN FUNCTION";
      }

      if (valid == 3)
      {
        std::cout << "A REQUIRED KEY IS MISSING";
      }

      return valid;
    }

    template <class... AssignedKeyPack, std::enable_if_t<
    eval<AssignedKeyPack...>() == 0,
    int> = 0>
    typename KeyFunctionTraits::ResultType operator()(AssignedKeyPack... _args) const 
    {
      return internal<AssignedKeyPack...>(_args..., std::make_index_sequence<sizeof...(FunctionKeys)>{});
    }

    template <class... AssignedKeyPack, size_t... Is>
    typename KeyFunctionTraits::ResultType internal(AssignedKeyPack&... _args, std::index_sequence<Is...> const &) const
    {

      std::array<AssignedKey<void>,sizeof...(AssignedKeyPack)> passedKeys = {_args.moveToVoid()...};

      std::sort(passedKeys.begin(), passedKeys.end(), 
        [](const AssignedKey<void>& _a0, const AssignedKey<void>& _a1)
        {
          return _a0.getKeyID() < _a1.getKeyID();
        }
      );

      //std::array<int,sizeof...(_args)> passedKeyIDs = {_args.getKeyID()...};
      //std::sort(passedKeyIDs.begin(), passedKeyIDs.end());
      std::tuple<FunctionKeys...> functionKeyTypes;
      int offset = 0;

      auto process = [&] <class KeyType> (KeyType& _key, int _idx) -> KeyType::type
      { 
        // no more passed keys left, freturn empty optional
        if constexpr (IsOptional<typename KeyType::type>::value)
        {
          if (_idx > sizeof...(_args) + offset)
          {
            typename KeyType::type out = std::nullopt;
            return out;
          }
          // function paramter nr. _idx not present, fill woth nullopt
          if (passedKeys[_idx - offset].getKeyID() > _key.ID) 
          {
            ++offset;
            typename KeyType::type out = std::nullopt;
            return out;
          } 
        }
        // same key, transfer ownership
        if (passedKeys[_idx - offset].getKeyID() == _key.ID)
        {
          std::cout << "CONVERTING KEY " << _key.ID << std::endl;
          return any_cast<typename KeyType::type>(passedKeys[_idx - offset]);
        }

        // SHOULD NOT HAPPEN
        throw std::runtime_error("What happened here???");

      };

      std::tuple<typename FunctionKeys::type...> paddedParameters =
        {process(std::get<Is>(functionKeyTypes), Is)...};

      std::cout << "DONE, PASSING TO FUNCTION" << std::endl;

      return m_baseFunction(std::get<Is>(paddedParameters)...);

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
