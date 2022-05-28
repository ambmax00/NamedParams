#ifndef NAMED_PARAMS_H
#define NAMED_PARAMS_H

#include <array>
#include <functional>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <tuple>

#include <iostream>

#ifndef USE_CXX_20

/**
  * We need to define our own constexpr swap and sort functions for C++17 
*/
template <class T>
constexpr void _swap(T& _a, T& _b)
{
  if (&_a == &_b) 
  {
    return;
  }
  T tmp(std::move(_a));
  _a = std::move(_b);
  _b = std::move(tmp);
}

template <class Iterator, class Compare>
inline constexpr Iterator _partition(Iterator _begin, Iterator _end, Compare _comp)
{
  Iterator pivot = _end - 1; 
  int idxSmallest = -1;

  for (auto it = _begin; it < pivot; ++it)
  {
    if (_comp(*it,*pivot))
    {
      ++idxSmallest;
      _swap(*(_begin + idxSmallest),*it);
    }
  }

  Iterator it = _begin + (idxSmallest + 1);

  _swap(*it, *(_end-1));
  return it;
  
}

template <class Iterator, class Compare>
inline constexpr void _sort(Iterator _begin, Iterator _end, Compare _comp)
{
  if (_begin == _end)
  {
    return;
  }

  if (_begin < _end-1) // skip if only one element
  {
    auto pivot = _partition(_begin, _end, _comp);
    _sort(_begin, pivot, _comp);
    _sort(pivot+1, _end, _comp);
  }

  return;
}

template <class Iterator>
inline constexpr void _sort(Iterator _begin, Iterator _end)
{
  auto comp = [](const decltype(*_begin)& _a, const decltype(*_end)& _b)
  {
    return _a < _b;
  };
  _sort(_begin, _end, comp);
}

#else // CXX20

/* Use inbuilt sort function */

template <class Iterator, class Comp>
inline constexpr void _sort(Iterator _begin, Iterator _end, Comp _comp)
{
  std::sort(_begin, _end, _comp);
}

template <class Iterator>
inline constexpr void _sort(Iterator _begin, Iterator _end)
{
  std::sort(_begin, _end);
}

#endif // CXX20

/**
  * Define stucts for identifying optional and key
*/

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

template <class TKey, std::enable_if_t<IsKey<TKey>::value,bool> = true>
class AssignedKey;

template <class T>
struct IsAssignedKey : public std::false_type {};

template <class T>
struct IsAssignedKey<AssignedKey<T>> : public std::true_type {};

template <class... TAssignedKeyPack>
constexpr inline bool areAllAssignedKeys() 
{
  const std::array<bool,sizeof...(TAssignedKeyPack)> isKey = 
  {
    IsAssignedKey<TAssignedKeyPack>::value...  
  };

  for (auto a : isKey)
  {
    if (!a) 
    {
      return false;
    }
  }

  return true;
}

/**
 * AssignedKey is the result of assigning(=) a Key to a value.
 * If the Keytype is a reference, it contains a pointer to the variable 
 * the key was assigned to. If not, it contains a copy of the variable
 */
template <class TKey, std::enable_if_t<IsKey<TKey>::value,bool>>
class AssignedKey
{
  private:

    typedef typename std::remove_reference<typename TKey::type>::type NoRefType;

    NoRefType* m_pValue;
    int m_keyID;

  public:

    AssignedKey() = delete;

    AssignedKey(NoRefType* _pValue, int _keyID)
      : m_pValue(_pValue)
      , m_keyID(_keyID)
    {
    }

    template <class T, std::enable_if_t<std::is_reference<T>::value, int> = 0>
    static AssignedKey build(T _value)
    {
      return AssignedKey(&_value, TKey::ID);
    }

    template <class T, std::enable_if_t<!std::is_reference<T>::value, bool> = true>
    static AssignedKey build(T _value)
    {
      return AssignedKey(new T(_value), TKey::ID);
    }

    AssignedKey(const AssignedKey& _input) = delete;

    AssignedKey(AssignedKey&& _input)
    {
      m_pValue = _input.m_pValue;
      m_keyID = _input.m_keyID;
      _input.m_pValue = nullptr;
    }

    AssignedKey& operator=(const AssignedKey& _input) = delete;

    AssignedKey& operator=(AssignedKey&& _input) 
    {
      m_pValue = _input.m_pValue;
      m_keyID = _input.m_keyID;
      _input.m_pValue = nullptr;
      return *this;
    }

    ~AssignedKey()
    {
      if (m_pValue && !std::is_reference<typename TKey::type>::value) 
      {
        delete m_pValue;
        m_pValue = nullptr;
      }

    }

    NoRefType* getValue() 
    {
      return m_pValue;
    }

    int getKeyID() const
    {
      return m_keyID;
    }

    typedef TKey keyType;

};

/**
 * The Key class allows to define named parameters that are passed to the KeyGenClass object.
 * Keys can be reused from one function to another.
 * Keys in the same function should not have the same UNIQUE_ID, or everything
 * will break down.
 * The Key class does not keep any record of the variable it is assigned to,
 * but delegates the work to AssignedKey.
 */
template <typename T, int UNIQUE_ID>  // disable negative numbers
class Key
{
  private:

  public:
    Key() {}

    Key(const Key& _other) = delete;

    Key(Key&& _other) = delete;

    auto operator=(T _any) const
    {
      auto k = AssignedKey<Key>::template build<T>(_any);
      return k;
    }

    typedef T type;

    static const int ID = UNIQUE_ID;

};

/**
 * Helper class to force a compile error
 */
template <int i, typename T = void, typename D = void>
class ConstExprError;

/**
 * FunctionTraits taken from "https://functionalcpp.wordpress.com/2013/08/05/function-traits/"
 * A helper class to get the variable types of a function
 */

template <typename T>
struct FunctionTraitsBase;

template <typename T> 
struct FunctionTraits;  

template<typename R, typename ...Args> 
struct FunctionTraitsBase<R(Args...)>
{
    static const size_t nbArgs = sizeof...(Args);

    typedef R ResultType;

    template <size_t i>
    struct arg
    {
        typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
    };
};

template <class R, typename ...Args>
struct FunctionTraits<R(Args...)> : public FunctionTraitsBase<R(Args...)> 
{
  typedef void ClassType;
};

template <class C, class R, typename... Args>
struct FunctionTraits<R(C::*)(Args...)> : public FunctionTraitsBase<R(Args...)>
{
  typedef C ClassType;
};

template <class TFunctionPtr, class... TFunctionKeys, size_t... Is>
constexpr inline static int KeyTypesAreValid(std::index_sequence<Is...> const &)
{
  constexpr std::array<bool,sizeof...(TFunctionKeys)> keyTypesCompare = {
    std::is_same<
      typename std::tuple_element<Is, std::tuple<typename TFunctionKeys::type...>>::type, 
      typename FunctionTraits<typename std::remove_pointer<TFunctionPtr>::type>::template arg<Is>::type
    >::value...};

  for (int i = 0; i < (int)sizeof...(TFunctionKeys); ++i)
  {
    if (!keyTypesCompare[i])
    {
      return i;
    }
  }

  return -1;

}

template <class... TFunctionKeys>
constexpr inline static int MultipleIdenticalKeys()
{
  std::array<int, sizeof...(TFunctionKeys)> keyIDs = { TFunctionKeys::ID... };
  _sort(keyIDs.begin(), keyIDs.end());
  for (int i = 1; i < (int)sizeof...(TFunctionKeys); ++i) 
  {
    if (keyIDs[i-1] == keyIDs[i])
    {
      return i;
    }
  }
  return -1;
} 

template <class TFunctionPtr, class... TFunctionKeys>
constexpr inline static bool KeyGenTemplateIsValid() 
{
  // Check if TFunctionPtr is really a (member) function pointer
  // This is necessary because templates are checked for both constructors
  // in KeyGenClass, even if only one is enabled. This leads to errors 
  // in this function. 
  if constexpr (std::is_function<typename std::remove_pointer<TFunctionPtr>::type>::value
    || std::is_member_function_pointer<TFunctionPtr>::value)
  {
    constexpr int nbFunctionArgs = FunctionTraits< 
      typename std::remove_pointer<TFunctionPtr>::type>::nbArgs;

    constexpr int nbKeys = sizeof...(TFunctionKeys);
    if constexpr (nbFunctionArgs != nbKeys)
    {
      ConstExprError<nbKeys> TOO_MANY_KEYS_PASSED_TO_KEYGEN;
      return false;
    }

    constexpr int invalidKey = KeyTypesAreValid<TFunctionPtr,TFunctionKeys...>(
      std::make_index_sequence<nbFunctionArgs>());
    if constexpr (invalidKey >= 0)
    {
      ConstExprError<invalidKey> KEY_TYPE_FUNCTION_ARGUMENT_TYPE_MISMATCH;
      return false;
    }

    constexpr int duplicateKey = MultipleIdenticalKeys<TFunctionKeys...>();
    if constexpr (duplicateKey >= 0)
    {
      ConstExprError<duplicateKey> MULTIPLE_KEYS_WITH_SAME_KEY_ID;
    }

    // TODO: ENFORCE OPTIONAL KEYS AFTER NON-OPTIONAL KEYS (?)
    return true;
  }

  return false;

}

template <class TFunctionPtr, class... TFunctionKeys>
class KeyGenClass
{
  private:

    typedef FunctionTraits<typename std::remove_pointer<TFunctionPtr>::type> KeyFunctionTraits;

    typename KeyFunctionTraits::ClassType* m_classPtr;

    TFunctionPtr m_baseFunction; 

    std::array<int, KeyFunctionTraits::nbArgs> m_keyIDs;

  public:

    template <class DFunctionPtr, class... DFunctionKeys,
      std::enable_if_t<
        !std::is_member_function_pointer<DFunctionPtr>::value
        && KeyGenTemplateIsValid<DFunctionPtr,DFunctionKeys...>()
      , bool> = true>
    KeyGenClass(DFunctionPtr _function, [[maybe_unused]] const DFunctionKeys&... _keys)
      : m_classPtr(nullptr)
      , m_baseFunction(_function)
    {
    }

    template <class DFunctionPtr, class... DFunctionKeys, 
      std::enable_if_t<
        std::is_member_function_pointer<DFunctionPtr>::value
        && KeyGenTemplateIsValid<DFunctionPtr,DFunctionKeys...>()
      , bool> = true>
    KeyGenClass(typename KeyFunctionTraits::ClassType* _classPtr, DFunctionPtr _function, 
      [[maybe_unused]] const DFunctionKeys&... _keys)
      : m_classPtr(_classPtr)
      , m_baseFunction(_function)
    {
    }

    ~KeyGenClass() {}

    std::function<TFunctionPtr> getBaseFunction() const
    {
      return m_baseFunction;
    }

    enum class ErrorType
    {
      NONE = 0,
      MISSING_KEY = 1,
      INVALID_KEY = 2,
      MULTIPLE_KEYS = 3,
      POSITION = 4,
      TOO_MANY_ARGS = 5,
      CONVERSION = 6
    };

    struct EvalReturn
    {
      ErrorType errorType;
      int id;
    };

    template <class... Any>
    constexpr inline static std::pair<int,int> getNb() 
    {
      std::array<bool,sizeof...(Any)> types = 
      {
        IsAssignedKey<Any>::value...
      };

      std::pair<int,int> group = std::make_pair(0,0);
      for (auto t : types)
      {
        if (!t)
        {
          group.first++;
        }
        else 
        {
          group.second++;
        }
      }

      return group;

    }

    template <class D>
    struct GetArgumentID 
    {
      const inline static int ID = -1;
    };

    template <class D>
    struct GetArgumentID<AssignedKey<D>>
    {
      const inline static int ID = AssignedKey<D>::keyType::ID;
    };

    template <class... Any, size_t... Is>
    constexpr inline static std::array<bool,sizeof...(Is)> positionalConversion(std::index_sequence<Is...> const &) 
    {
      constexpr std::array<bool,sizeof...(Is)> out = 
      { 
        std::is_convertible<
          typename std::tuple_element<Is, std::tuple<Any...>>::type, 
          typename KeyFunctionTraits::template arg<Is>::type
        >::value...
      };

      // BETTER ERROR HANDLING: SHOW WHICH TYPES CANNOT BE CONVERTED

      return out;
    }
    
    template <class... Any>
    constexpr inline static EvalReturn evalAny()
    {

      // get number of different arguments
      constexpr auto group = getNb<Any...>();
      constexpr int nbPositionalArgs = group.first;
      constexpr int nbAssignedKeys = group.second;
      constexpr int nbFunctionKeys = sizeof...(TFunctionKeys);
      constexpr int nbPassedArgs = sizeof...(Any);

      // check for too many args
      if (nbPassedArgs > nbFunctionKeys)
      {
        return EvalReturn{ErrorType::TOO_MANY_ARGS, 0};
      }

      // check if positional arguments are grouped together
      constexpr std::array<bool,sizeof...(Any)> isKey = 
      {
        IsAssignedKey<Any>::value...
      };

      for (int i = 1; i < nbPassedArgs; ++i)
      {
        // key should not follow positional argument
        if (isKey[i-1] && !isKey[i])
        {
          return EvalReturn{ErrorType::POSITION, i};
        }
      }
      
      // check if positional arguments are correct types or convertible (Keys are set to always true)
      std::array<bool,nbPositionalArgs> positionalIsConvertible = positionalConversion<Any...>(
        std::make_index_sequence<nbPositionalArgs>());

      for (int i = 0; i < nbPositionalArgs; ++i)
      {
        if (!positionalIsConvertible[i])
        {
          return EvalReturn{ErrorType::CONVERSION, i};
        }
      }
      
      // check if keys are all correct types <----
      // maybe in the constructor directly, probably better?

      // get IDs of assigned keys (-1 for non-keys)
      std::array<int,nbPassedArgs> passedKeyIDs = { GetArgumentID<Any>::ID... };
    
      // get IDs of true function keys
      std::array<int,nbFunctionKeys> functionKeyIDs = { TFunctionKeys::ID... };

      std::array<bool,nbFunctionKeys> functionKeyIsOptional = {
        IsOptional<typename TFunctionKeys::type>::value...};

      int nbRequiredKeys = 0;
      for (auto fOpt : functionKeyIsOptional)
      { 
        nbRequiredKeys += (fOpt) ? 0 : 1;
      }
      
      // if no args passed, but function has required keys, throw error 
      // (e.g. funcWrapper() -> func(int))
      if (nbRequiredKeys != 0 && nbPassedArgs == 0)
      {
        return EvalReturn{ErrorType::MISSING_KEY, 0}; //<--- fix number
      }

      // if func only has optional arguments, and no args were passed we can return immediately
      // e.g. funcWrapper() -> func(std::optional<int>)
      if (nbRequiredKeys == 0 && nbPassedArgs == 0)
      {
        return EvalReturn{ErrorType::NONE, 0};
      }

      // if all required keys in positional arguments, and no keys -> return
      // e.g. funcWrapper(1,2,3) -> func(int, int, std::optional)
      if (nbRequiredKeys <= nbPositionalArgs && nbAssignedKeys == 0)
      {
        return EvalReturn{ErrorType::NONE, 0};
      }

      // check for multiple keys of same type
      for (int i = nbPositionalArgs+1; i < nbPassedArgs; ++i)
      {
        if (passedKeyIDs[i-1] == passedKeyIDs[i])
        {
          return EvalReturn{ErrorType::MULTIPLE_KEYS, passedKeyIDs[i-1]};
        }
      }

      // check if unknown key present
      for (int i = nbPositionalArgs; i < nbPassedArgs; ++i)
      {
        bool found = false;
        for (int j = 0; j < nbFunctionKeys; ++j)
        {
          if (passedKeyIDs[i] == functionKeyIDs[j])
          {
            found = true;
          }
        }
        if (!found) 
        {
          return EvalReturn{ErrorType::INVALID_KEY,i};
        }
      }

      // now sort the Key IDs for checking correct keys
      _sort(passedKeyIDs.begin(), passedKeyIDs.end());
      int offset = 0;
      
      for (int iArg = nbPositionalArgs; iArg < nbFunctionKeys; ++iArg)
      {
        // no more passed keys to parse, and function key is NOT optional
        if (iArg >= nbPassedArgs+offset && !functionKeyIsOptional[iArg])
        {
          return EvalReturn{ErrorType::MISSING_KEY, functionKeyIDs[iArg]};
        }
        // no more passed keys to parse and function key IS optional 
        else if (iArg >= nbPassedArgs+offset && functionKeyIsOptional[iArg])
        {
          continue;
        }
        // function key should never be larger than the pass key
        else if (functionKeyIDs[iArg] > passedKeyIDs[iArg-offset])
        {
          return EvalReturn{ErrorType::INVALID_KEY, passedKeyIDs[iArg-offset]};
        }
        // if the function key is smaller, some key might be missing - check if optional
        else if (functionKeyIDs[iArg] < passedKeyIDs[iArg-offset] && !functionKeyIsOptional[iArg])
        {
          return EvalReturn{ErrorType::MISSING_KEY, functionKeyIDs[iArg]};
        }
        // same as above. but incrementing offset
        else if (functionKeyIDs[iArg] < passedKeyIDs[iArg-offset] && functionKeyIsOptional[iArg])
        {
          offset++;
        }
      }

      return EvalReturn{ErrorType::NONE, 0};
    
    }
    
    template <class... Any>
    constexpr inline static bool evalAnyError()
    {
      constexpr EvalReturn error = evalAny<Any...>();

      if constexpr (error.errorType == ErrorType::MISSING_KEY)
      {
        ConstExprError<error.id> MISSING_KEY;
        return false;
      }
      else if constexpr (error.errorType == ErrorType::INVALID_KEY)
      {
        ConstExprError<error.id> INVALID_KEY;
        return false;
      }
      else if constexpr (error.errorType == ErrorType::MULTIPLE_KEYS)
      {
        ConstExprError<error.id> MULTIPLE_KEYS_OF_SAME_TYPE;
        return false;
      }
      else if constexpr (error.errorType == ErrorType::POSITION)
      {
        ConstExprError<error.id> POSITIONAL_CANNOT_FOLLOW_KEY_ARGUMENT;
        return false;
      }
      else if constexpr (error.errorType == ErrorType::CONVERSION)
      {
        ConstExprError<error.id> ARGUMENT_CANNOT_BE_CONVERTED;
        return true;
      }
      else if constexpr (error.errorType == ErrorType::TOO_MANY_ARGS)
      {
        ConstExprError<error.id> TOO_MANY_ARGUMENTS_PASSED_TO_FUNCTION;
        return true;
      }

      return true;
    }
    
    template <class... Any, std::enable_if_t<evalAnyError<Any...>(), int> = 0>
    typename KeyFunctionTraits::ResultType operator()(Any&&... _args) const 
    {
      return internal2<Any...>(std::forward<Any>(_args)..., std::make_index_sequence<sizeof...(TFunctionKeys)>{});
    }

    struct AnyKey 
    {
      int id;
      void* ptr;
    };

    template <class TKey, class TArray>
    typename TKey::type process(TKey& _key, 
                                int _idx, 
                                int _nbPositionals, 
                                int _nbArguments,
                                TArray& _passedKeys,
                                int& _offset) const
    { 
      if (_idx >= _nbPositionals) 
      {
        if constexpr (IsOptional<typename TKey::type>::value)
        {
          // no more passed keys left, return empty optional
          if (_idx >= _nbArguments + _offset)
          {
            typename TKey::type out = std::nullopt;
            return out;
          }
          // function paramter nr. _idx not present, fill with nullopt
          if (_passedKeys[_idx - _offset].id > _key.ID) 
          {
            ++_offset;
            typename TKey::type out = std::nullopt;
            return out;
          } 
        }
        // same key, transfer ownership
        if (_passedKeys[_idx - _offset].id == _key.ID)
        {
          auto pAssignedKey = (AssignedKey<TKey>*)_passedKeys[_idx-_offset].ptr;
          return *pAssignedKey->getValue();
        }
      }
      else 
      {
        // positionals are guaranteed to be in order and not skip any arguments
        // remove reference to avoid pointer to reference
        return *((typename std::remove_reference<typename TKey::type>::type*)
          _passedKeys[_idx-_offset].ptr);
      }

      // SHOULD NOT HAPPEN
      throw std::runtime_error("This should not happen");
    }

    template <class... Any, size_t... Is>
    typename KeyFunctionTraits::ResultType internal2(Any&&... _args, std::index_sequence<Is...> const &) const
    {
      auto constexpr group = getNb<Any...>();
      constexpr int nbPositionals = group.first;
      constexpr int nbPassedArgs = sizeof...(Any);

      std::array<AnyKey,nbPassedArgs> passedKeys = 
      {
        AnyKey{GetArgumentID<Any>::ID, (void*)&_args}...
      };

      _sort(passedKeys.begin() + nbPositionals, passedKeys.end(), 
        [](const auto& _p0, const auto& _p1)
        {
          return _p0.id < _p1.id;
        }
      );

      for (auto s : passedKeys) 
      {
        std::cout << s.id << std::endl;
      }

      std::tuple<TFunctionKeys...> functionKeyTypes;
      int offset = 0;

      std::tuple<typename TFunctionKeys::type...> paddedParameters = {
        process(std::get<Is>(functionKeyTypes), Is, nbPositionals, nbPassedArgs, passedKeys, 
        offset)... };

      return call(std::get<Is>(paddedParameters)...);

    }

    template <typename DFunctionPtr = TFunctionPtr, 
      std::enable_if_t<std::is_member_function_pointer<DFunctionPtr>::value,bool> = true>
    typename KeyFunctionTraits::ResultType call(typename TFunctionKeys::type... _args) const
    {
      return (m_classPtr->*m_baseFunction)(_args...);
    }

    template <typename DFunctionPtr = TFunctionPtr, 
      std::enable_if_t<!std::is_member_function_pointer<DFunctionPtr>::value,bool> = true>
    typename KeyFunctionTraits::ResultType call(typename TFunctionKeys::type... _args) const
    {
      return m_baseFunction(_args...);
    }

};

template <class DFunctionPtr, class... DFunctionKeys>
KeyGenClass(DFunctionPtr _function, const DFunctionKeys&... _keys) 
  -> KeyGenClass<DFunctionPtr,DFunctionKeys...>;

template <class DFunctionPtr, class... DFunctionKeys>
KeyGenClass(typename DFunctionPtr::ClassType _classPtr, DFunctionPtr _function, 
  const DFunctionKeys&... _keys) -> KeyGenClass<DFunctionPtr,DFunctionKeys...>;

#define UNPAREN(...) __VA_ARGS__ 
#define KEY(TYPE, ID) inline static const Key< UNPAREN TYPE , ID > 
#define KEYOPT(TYPE, ID) inline static const Key<std::optional< UNPAREN TYPE >, ID >
#define KEYGEN inline static const KeyGenClass

#endif // NAMED_PARAMS_H
