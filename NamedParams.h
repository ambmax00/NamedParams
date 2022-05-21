#ifndef NAMED_PARAMS_H
#define NAMED_PARAMS_H

#include <array>
#include <functional>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <tuple>

#include <iostream>

#if 1

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
  Iterator smallest = _begin - 1;

  for (auto it = _begin; it < pivot; ++it)
  {
    if (_comp(*it,*pivot))
    {
      smallest++;
      _swap(*smallest,*it);
    }
  }

  _swap(*(smallest+1), *(_end-1));
  return smallest+1;
}

template <class Iterator, class Compare>
inline constexpr void _sort(Iterator _begin, Iterator _end, Compare _comp)
{
  if (_begin == _end)
  {
    return;
  }

  if (_begin < _end-1)
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
#define UNIQUE *unique_id([]{})

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

template <class KeyType, std::enable_if_t<IsKey<KeyType>::value,bool> = true>
class AssignedKey;

template <class T>
struct IsAssignedKey : public std::false_type {};

template <class T>
struct IsAssignedKey<AssignedKey<T>> : public std::true_type {};

template <class... AssignedKeyPack>
constexpr inline bool areAllAssignedKeys() 
{
  const std::array<bool,sizeof...(AssignedKeyPack)> isKey = 
  {
    IsAssignedKey<AssignedKeyPack>::value...  
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
template <class KeyType, std::enable_if_t<IsKey<KeyType>::value,bool>>
class AssignedKey
{
  private:

    typedef typename std::remove_reference<typename KeyType::type>::type NoRefType;

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
      return AssignedKey(&_value, KeyType::ID);
    }

    template <class T, std::enable_if_t<!std::is_reference<T>::value, bool> = true>
    static AssignedKey build(T _value)
    {
      return AssignedKey(new T(_value), KeyType::ID);
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
      if (m_pValue && !std::is_reference<typename KeyType::type>::value) 
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

    typedef KeyType keyType;

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

/*
// member function pointer
template<class C, class R, class... Args>
struct FunctionTraits<R(C::*)(Args...)> : public FunctionTraits<R(C&,Args...)>
{};
 
// const member function pointer
template<class C, class R, class... Args>
struct FunctionTraits<R(C::*)(Args...) const> : public FunctionTraits<R(C&,Args...)>
{};
 
// member object pointer
template<class C, class R>
struct FunctionTraits<R(C::*)> : public FunctionTraits<R(C&)>
{};*/

template <class FunctionPtr, class... FunctionKeys> 
/// CHECK THAT OPTIONAL ARGUMENTS ONLY AFTER REQUIRED?
/// AND CHECK THAT NUMBER OF KEYS EQUALS NUMBER OF FUNCTION ARGUMENTS
/// AND CHECK THAT KEYS HAVE CORRECT TYPE, AND CHECK THAT THEY ARE UNIQUE
class KeyGenClass
{
  private:

    typedef FunctionTraits<typename std::remove_pointer<FunctionPtr>::type> KeyFunctionTraits;

    typename KeyFunctionTraits::ClassType* m_classPtr;

    FunctionPtr m_baseFunction; 

    std::array<int, KeyFunctionTraits::nbArgs> m_keyIDs;

  public:

    template <class _FunctionPtr, class... _FunctionKeys,
      std::enable_if_t<!std::is_member_function_pointer<_FunctionPtr>::value,bool> = true>
    KeyGenClass(_FunctionPtr _function, const _FunctionKeys&... _keys)
      : m_classPtr(nullptr)
      , m_baseFunction(_function)
    {
    }

    template <class _FunctionPtr, class... _FunctionKeys,
      std::enable_if_t<std::is_member_function_pointer<_FunctionPtr>::value,bool> = true>
    KeyGenClass(KeyFunctionTraits::ClassType* _classPtr, _FunctionPtr _function, const _FunctionKeys&... _keys)
      : m_classPtr(_classPtr)
      , m_baseFunction(_function)
    {
    }

    ~KeyGenClass() {}

    std::function<FunctionPtr> getBaseFunction() const
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

    template <class... AssignedKeyPack>
    constexpr static inline EvalReturn evalKeys()
    {
      constexpr int nbAssignedKeys = sizeof...(AssignedKeyPack);
      constexpr int nbFunctionKeys = sizeof...(FunctionKeys);

      std::array<int,nbAssignedKeys> passedKeys = {AssignedKeyPack::keyType::ID...};
      std::array<int,nbFunctionKeys> functionKeys = {FunctionKeys::ID...};

      std::array<bool,nbFunctionKeys> functionKeyIsOptional = {
        IsOptional<typename FunctionKeys::type>::value...};
      std::array<bool,nbAssignedKeys> passedKeyIsOptional = {
        IsOptional<typename AssignedKeyPack::keyType::type>::value...};

      // check for empty assigned keys
      int nbRequiredKeys = 0;
      for (auto b : functionKeyIsOptional)
      {
        nbRequiredKeys += (b) ? 0 : 1;
      }

      if (nbRequiredKeys != 0 && nbAssignedKeys == 0)
      {
        return EvalReturn{ErrorType::MISSING_KEY, 0};
      }    

      if (nbRequiredKeys == 0 && nbAssignedKeys == 0) 
      {
        return EvalReturn{ErrorType::NONE, 0};
      } 

      // check for multiple keys of same type
      for (int i = 1; i < nbAssignedKeys; ++i)
      {
        if (passedKeys[i-1] == passedKeys[i])
        {
          return EvalReturn{ErrorType::MULTIPLE_KEYS, passedKeys[i-1]};
        }
      }

      _sort(passedKeys.begin(), passedKeys.end());
      EvalReturn evalReturn = {ErrorType::NONE, 0};
      int offset = 0;

      for (int iArg = 0; iArg < nbFunctionKeys; ++iArg)
      {
        // no more passed keys to parse, and function key is NOT optional
        if (iArg >= nbAssignedKeys+offset && !functionKeyIsOptional[iArg])
        {
          evalReturn.errorType = ErrorType::MISSING_KEY;
          evalReturn.id = functionKeys[iArg];
          break;
        }
        // no more passed keys to parse and function key IS optional 
        else if (iArg >= nbAssignedKeys+offset && functionKeyIsOptional[iArg])
        {
          continue;
        }
        // function key should never be larger than the pass key
        else if (functionKeys[iArg] > passedKeys[iArg-offset])
        {
          evalReturn.errorType = ErrorType::INVALID_KEY;
          evalReturn.id = passedKeys[iArg-offset];
          break;
        }
        // if the function key id is smaller, some key might be missing - check if optional
        else if (functionKeys[iArg] < passedKeys[iArg-offset] && !functionKeyIsOptional[iArg])
        {
          evalReturn.errorType = ErrorType::MISSING_KEY;
          evalReturn.id = functionKeys[iArg];
          break;
        }
        // same as above. but incrementing offset
        else if (functionKeys[iArg] < passedKeys[iArg-offset] && functionKeyIsOptional[iArg])
        {
          offset++;
        }
      }

      return evalReturn;
    }

    template <class... AssignedKeyPack, 
      std::enable_if_t<areAllAssignedKeys<AssignedKeyPack...>(),bool> = true>
    constexpr static inline bool evalKeysError()
    {
      constexpr EvalReturn error = evalKeys<AssignedKeyPack...>();

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
      else
      {
        return true;
      }

    }

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
          typename KeyFunctionTraits::arg<Is>::type
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
      constexpr int nbFunctionKeys = sizeof...(FunctionKeys);
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
      std::array<int,nbFunctionKeys> functionKeyIDs = { FunctionKeys::ID... };

      std::array<bool,nbFunctionKeys> functionKeyIsOptional = {
        IsOptional<typename FunctionKeys::type>::value...};

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
      EvalReturn constexpr error = evalAny<Any...>();

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
        ConstExprError<error.id> POSITIONAL_CANNOT_FOLLOW_OPTIONAL_ARGUMENT;
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
      return internal2<Any...>(std::forward<Any>(_args)..., std::make_index_sequence<sizeof...(FunctionKeys)>{});
    }

    struct AnyKey 
    {
      int id;
      void* ptr;
    };

    template <class... Any, size_t... Is>
    typename KeyFunctionTraits::ResultType internal2(Any&&... _args, std::index_sequence<Is...> const &) const
    {
      auto constexpr group = getNb<Any...>();
      constexpr int nbPositionals = group.first;
      constexpr int nbAssignedKeys = group.second;
      constexpr int nbPassedArgs = sizeof...(Any);
      constexpr int nbFunctionKeys = sizeof...(FunctionKeys);

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

      std::tuple<FunctionKeys...> functionKeyTypes;
      int offset = 0;

      auto process = [&] <class KeyType> (KeyType& _key, int _idx) -> typename KeyType::type
      { 
        if (_idx >= nbPositionals) 
        {
          if constexpr (IsOptional<typename KeyType::type>::value)
          {
            // no more passed keys left, return empty optional
            if (_idx >= sizeof...(_args) + offset)
            {
              typename KeyType::type out = std::nullopt;
              return out;
            }
            // function paramter nr. _idx not present, fill with nullopt
            if (passedKeys[_idx - offset].id > _key.ID) 
            {
              ++offset;
              typename KeyType::type out = std::nullopt;
              return out;
            } 
          }
          // same key, transfer ownership
          if (passedKeys[_idx - offset].id == _key.ID)
          {
            auto pAssignedKey = (AssignedKey<KeyType>*)passedKeys[_idx-offset].ptr;
            return *pAssignedKey->getValue();
          }
        }
        else 
        {
          // positionals are guaranteed to be in order and not skip any arguments
          // remove reference to avoid pointer to reference
          return *((typename std::remove_reference<typename KeyType::type>::type*)passedKeys[_idx-offset].ptr);
        }

        // SHOULD NOT HAPPEN
        throw std::runtime_error("This should not happen");

      };

      std::tuple<typename FunctionKeys::type...> paddedParameters =
        {process(std::get<Is>(functionKeyTypes), Is)...};

      return call(std::get<Is>(paddedParameters)...);

    }

    template <typename _FunctionPtr = FunctionPtr, 
      std::enable_if_t<std::is_member_function_pointer<_FunctionPtr>::value,bool> = true>
    typename KeyFunctionTraits::ResultType call(FunctionKeys::type... _args) const
    {
      return (m_classPtr->*m_baseFunction)(_args...);
    }

    template <typename _FunctionPtr = FunctionPtr, 
      std::enable_if_t<!std::is_member_function_pointer<_FunctionPtr>::value,bool> = true>
    typename KeyFunctionTraits::ResultType call(FunctionKeys::type... _args) const
    {
      return m_baseFunction(_args...);
    }

};

template <class _FunctionPtr, class... _FunctionKeys>
KeyGenClass(_FunctionPtr _function, const _FunctionKeys&... _keys) -> KeyGenClass<_FunctionPtr,_FunctionKeys...>;

template <class T*, class _FunctionPtr, class... _FunctionKeys>
KeyGenClass(T* _classPtr, _FunctionPtr _function, const _FunctionKeys&... _keys) -> KeyGenClass<_FunctionPtr,_FunctionKeys...>;

#endif // NAMED_PARAMS_H
