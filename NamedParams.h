#ifndef NAMED_PARAMS_H
#define NAMED_PARAMS_H

#include <array>
#include <chrono>
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

template <class Iterator, class T>
inline constexpr Iterator _find(Iterator _begin, Iterator _end, T _value)
{
  for (auto iter = _begin; iter < _end; ++iter)
  {
    if (*iter == _value)
    {
      return iter;
    }
  }
  return _end;
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

enum DefaultKeyName 
{
  UNNAMED_KEY
};

template <class T, int64_t N, auto E = DefaultKeyName::UNNAMED_KEY, 
  std::enable_if_t<(N >= 0),bool> = true>
class Key;

template <class T>
struct IsKey : public std::false_type {};

template <class T, int64_t N, auto E>
struct IsKey<Key<T,N,E>> : public std::true_type {};

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
    int64_t m_keyID;

  public:

    AssignedKey() = delete;

    AssignedKey(NoRefType* _pValue, int64_t _keyID)
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

    int64_t getKeyID() const
    {
      return m_keyID;
    }

    typedef TKey keyType;

};

enum class ErrorType
{
  NONE = 0,
  MISSING_KEY = 1,
  INVALID_KEY = 2,
  SAME_KEY_PASSED_MORE_THAN_ONCE = 3,
  POSITIONAL_CANNOT_FOLLOW_KEY_ARGUMENT = 4,
  TOO_MANY_ARGUMENTS_PASSED_TO_FUNCTION = 5,
  KEY_HAS_WRONG_TYPE = 6,
  COULD_NOT_CONVERT_KEY_TYPE_TO_ARGUMENT_TYPE = 7,
  TOO_MANY_ARGUMENTS_PASSED_TO_KEYGEN = 8,
  SAME_KEY_PASSED_MORE_THAN_ONCE_KEYGEN = 9
};

template <ErrorType error, auto... Var>
constexpr void failWithMessage()
{
  static_assert((error != ErrorType::MISSING_KEY));
  static_assert((error != ErrorType::INVALID_KEY));
  static_assert((error != ErrorType::SAME_KEY_PASSED_MORE_THAN_ONCE));
  static_assert((error != ErrorType::SAME_KEY_PASSED_MORE_THAN_ONCE_KEYGEN));
  static_assert((error != ErrorType::POSITIONAL_CANNOT_FOLLOW_KEY_ARGUMENT));
  static_assert((error != ErrorType::TOO_MANY_ARGUMENTS_PASSED_TO_FUNCTION));
  static_assert((error != ErrorType::KEY_HAS_WRONG_TYPE));
  static_assert((error != ErrorType::COULD_NOT_CONVERT_KEY_TYPE_TO_ARGUMENT_TYPE));
  static_assert((error != ErrorType::TOO_MANY_ARGUMENTS_PASSED_TO_KEYGEN));
}

/**
 * The Key class allows to define named parameters that are passed to the KeyGenClass object.
 * Keys can be reused from one function to another.
 * Keys in the same function should not have the same UNIQUE_ID, or everything
 * will break down.
 * The Key class does not keep any record of the variable it is assigned to,
 * but delegates the work to AssignedKey.
 */
template <typename T, int64_t UNIQUE_ID, auto E, std::enable_if_t<(UNIQUE_ID >= 0),bool>> 
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

    static inline const int64_t ID = UNIQUE_ID;

    static auto const name = E;

};

/**
 * FunctionTraits taken and adapted from "https://functionalcpp.wordpress.com/2013/08/05/function-traits/"
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

template <class T, bool B>
struct _RemoveConstIfNotReferenceImpl;

template <class T>
struct _RemoveConstIfNotReferenceImpl<T,false>
{
  typedef T type;
};

template <class T>
struct _RemoveConstIfNotReferenceImpl<T,true>
{
  typedef typename std::remove_const<T>::type type;
};

template <class T>
struct RemoveConstIfNotReference
{
  typedef typename _RemoveConstIfNotReferenceImpl<T,!std::is_reference<T>::value>::type type;
};

template <class TFunctionPtr, class... TFunctionKeys, size_t... Is>
constexpr inline int KeyTypesAreValid(std::index_sequence<Is...> const &)
{
  // passing const by value to a function has no effect, and so decays to non-const
  // in function pointer. We therefore also remove const here
  using KeyTuple = std::tuple<typename RemoveConstIfNotReference<
    typename TFunctionKeys::type>::type...>;
  using TFunctionTraits = FunctionTraits<typename std::remove_pointer<TFunctionPtr>::type>;

  constexpr std::array<bool,sizeof...(TFunctionKeys)> keyTypesCompare = {
    std::is_same<
      typename std::tuple_element<Is, KeyTuple>::type, 
      typename TFunctionTraits::template arg<Is>::type
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
constexpr inline int MultipleIdenticalKeys()
{
  std::array<int64_t, sizeof...(TFunctionKeys)> keyIDs = { TFunctionKeys::ID... };
  _sort(keyIDs.begin(), keyIDs.end());
  for (int i = 1; i < (int)sizeof...(TFunctionKeys); ++i) 
  {
    if (keyIDs[i-1] == keyIDs[i])
    {
      return i-1;
    }
  }
  return -1;
} 

template <class TFunctionPtr, class... TFunctionKeys>
constexpr inline bool KeyGenTemplateIsValid() 
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
      failWithMessage<
        ErrorType::TOO_MANY_ARGUMENTS_PASSED_TO_KEYGEN,
        nbFunctionArgs,nbKeys>();
      return false;
    }

    constexpr int invalidKey = KeyTypesAreValid<TFunctionPtr,TFunctionKeys...>(
      std::make_index_sequence<nbFunctionArgs>());
    if constexpr (invalidKey >= 0)
    {
      failWithMessage<
        ErrorType::KEY_HAS_WRONG_TYPE,
        std::tuple_element<invalidKey, std::tuple<TFunctionKeys...>>::type::name>();
      return false;
    }

    constexpr int duplicateKey = MultipleIdenticalKeys<TFunctionKeys...>();
    if constexpr (duplicateKey >= 0)
    {
      failWithMessage<ErrorType::SAME_KEY_PASSED_MORE_THAN_ONCE_KEYGEN, duplicateKey>();
      return false;
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

    constexpr inline static std::array<int64_t, KeyFunctionTraits::nbArgs> m_functionKeyIDs = { 
      TFunctionKeys::ID... };

    // placeholder for unused optional arguments
    std::nullopt_t* m_nullOpt;

  public:

    template <class DFunctionPtr, class... DFunctionKeys,
      std::enable_if_t<
        !std::is_member_function_pointer<DFunctionPtr>::value
        && KeyGenTemplateIsValid<DFunctionPtr,DFunctionKeys...>()
      , bool> = true>
    KeyGenClass(DFunctionPtr _function, [[maybe_unused]] const DFunctionKeys&... _keys)
      : m_classPtr(nullptr)
      , m_baseFunction(_function)
      , m_nullOpt(new std::nullopt_t(std::nullopt))
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
      , m_nullOpt(new std::nullopt_t(std::nullopt))
    {
    }

    ~KeyGenClass() 
    {
      delete m_nullOpt;
    }

    std::function<TFunctionPtr> getBaseFunction() const
    {
      return m_baseFunction;
    }

    struct EvalReturn
    {
      ErrorType errorType;
      int id;
      int isFuncID; // wether id refers to TFunctionKeys (0), Any (pos) or None (neg)
    };

    enum KeyIdType 
    {
      POSITIONAL = -1,
      UNKNOWN = -2,
      ABSENT = -3
    };

    // get local key IDs for passed arguments
    // unknown arguments are given ID = -2
    // positional arguments are given ID = -1
    // named arguments are assigned its position in the function argument list
    template <class... Any>
    constexpr inline static std::array<int64_t,sizeof...(Any)> getLocalKeyIDs()
    {
      constexpr int nbPassedArgs = sizeof...(Any);
      std::array<int64_t, nbPassedArgs> passedKeyIDs = { GetArgumentID<Any>::ID... };
      std::array<int64_t, nbPassedArgs> passedLocalKeyIDs = {};
      
      for (int i = 0; i < nbPassedArgs; ++i) 
      {
        if (passedKeyIDs[i] < 0)
        {
          passedLocalKeyIDs[i] = KeyIdType::POSITIONAL;
          continue;
        }

        auto iter = _find(m_functionKeyIDs.begin(), m_functionKeyIDs.end(), passedKeyIDs[i]);

        if (iter == m_functionKeyIDs.end())
        {
          passedLocalKeyIDs[i] = KeyIdType::UNKNOWN;
        }
        else 
        {
          int64_t idx = iter - m_functionKeyIDs.begin();
          passedLocalKeyIDs[i] = idx;
        }

      }

      return passedLocalKeyIDs;
    }

    // sort the passed arguments according to order of functionKeys
    // do it here in this function, so we do not need to do it at runtime
    // returns a sorted array where array[idx] gives the original position of 
    // the passed argument
    template <class... Any>
    constexpr inline static std::array<int64_t,sizeof...(Any)> getSortedIndices(
      const std::array<int64_t,sizeof...(Any)>& localKeyIDs)
    {
      constexpr int nbPassedKeys = sizeof...(Any);
      std::array<int64_t, nbPassedKeys> indices = {};
      for (int i = 0; i < nbPassedKeys; ++i)
      {
        indices[i] = i;
      }

      _sort(indices.begin(), indices.end(), 
        [&](const int64_t a, const int64_t b)
        {
          return localKeyIDs[a] < localKeyIDs[b];
        });

      return indices;

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
      const inline static int64_t ID = KeyIdType::POSITIONAL;
    };

    template <class D>
    struct GetArgumentID<AssignedKey<D>>
    {
      const inline static int64_t ID = AssignedKey<D>::keyType::ID;
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
          return EvalReturn{ErrorType::POSITIONAL_CANNOT_FOLLOW_KEY_ARGUMENT, i, 0};
        }
      }
      
      // check if positional arguments are correct types or convertible (Keys are set to always true)
      std::array<bool,nbPositionalArgs> positionalIsConvertible = positionalConversion<Any...>(
        std::make_index_sequence<nbPositionalArgs>());

      for (int i = 0; i < nbPositionalArgs; ++i)
      {
        if (!positionalIsConvertible[i])
        {
          return EvalReturn{ErrorType::COULD_NOT_CONVERT_KEY_TYPE_TO_ARGUMENT_TYPE, i, 0};
        }
      }
      
      // check if keys are all correct types <----
      // maybe in the constructor directly, probably better?
    
      // get relative/local key IDs
      std::array<int64_t, nbFunctionKeys> functionLocalKeyIDs = {};
      
      for (int i = 0; i < nbFunctionKeys; ++i) 
      {
        functionLocalKeyIDs[i] = i;
      }
      
      constexpr std::array<int64_t, nbPassedArgs> passedLocalKeyIDs = getLocalKeyIDs<Any...>();
      
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
        return EvalReturn{ErrorType::MISSING_KEY, 0, 0};
      }

      // if func only has optional arguments, and no args were passed we can return immediately
      // e.g. funcWrapper() -> func(std::optional<int>)
      if (nbRequiredKeys == 0 && nbPassedArgs == 0)
      {
        return EvalReturn{ErrorType::NONE, 0, 0};
      }

      // if all required keys in positional arguments, and no keys -> return
      // e.g. funcWrapper(1,2,3) -> func(int, int, std::optional)
      if (nbRequiredKeys <= nbPositionalArgs && nbAssignedKeys == 0)
      {
        return EvalReturn{ErrorType::NONE, 0, 0};
      }

      // check if unknown key present
      for (int i = nbPositionalArgs; i < nbPassedArgs; ++i)
      {
        if (passedLocalKeyIDs[i] == KeyIdType::UNKNOWN) 
        {
          return EvalReturn{ErrorType::INVALID_KEY,i,1};
        }
      }

      // now sort the Key IDs for checking correct keys 

      constexpr auto sortIndex = getSortedIndices<Any...>(passedLocalKeyIDs);

      std::array<int64_t,nbPassedArgs> passedSortedKeyIDs = {};

      for (int i = 0; i < nbPassedArgs; ++i)
      {
        passedSortedKeyIDs[i] = passedLocalKeyIDs[sortIndex[i]];
      }

      //_sort(passedLocalKeyIDs.begin(), passedLocalKeyIDs.end());

       // check for multiple keys of same type
      for (int i = nbPositionalArgs+1; i < nbPassedArgs; ++i)
      {
        if (passedSortedKeyIDs[i-1] == passedSortedKeyIDs[i])
        {
          return EvalReturn{ErrorType::SAME_KEY_PASSED_MORE_THAN_ONCE, static_cast<int>(sortIndex[i-1]), 1};
        }
      }

      int offset = 0;
      
      for (int iArg = nbPositionalArgs; iArg < nbFunctionKeys; ++iArg)
      {
        // no more passed keys to parse, and function key is NOT optional
        if (iArg >= nbPassedArgs+offset && !functionKeyIsOptional[iArg])
        {
          return EvalReturn{ErrorType::MISSING_KEY, iArg, 0};
        }
        // no more passed keys to parse and function key IS optional 
        else if (iArg >= nbPassedArgs+offset && functionKeyIsOptional[iArg])
        {
          continue;
        }
        // function key should never be larger than the pass key
        else if (functionLocalKeyIDs[iArg] > passedSortedKeyIDs[iArg-offset])
        {
          return EvalReturn{ErrorType::INVALID_KEY, static_cast<int>(sortIndex[iArg-offset]), 1};
        }
        // if the function key is smaller, some key might be missing - check if optional
        else if (functionLocalKeyIDs[iArg] < passedSortedKeyIDs[iArg-offset] 
                 && !functionKeyIsOptional[iArg])
        {
          return EvalReturn{ErrorType::MISSING_KEY, iArg, 0};
        }
        // same as above. but incrementing offset
        else if (functionLocalKeyIDs[iArg] < passedSortedKeyIDs[iArg-offset] 
        && functionKeyIsOptional[iArg])
        {
          offset++;
        }
      }

      return EvalReturn{ErrorType::NONE, 0, 0};
    
    }
    
    template <class... Any>
    constexpr inline static bool evalAnyError()
    {
      // check for too many args to avoid further error messages and clogging up compiler output
      if constexpr (sizeof...(Any) > sizeof...(TFunctionKeys))
      {
        failWithMessage<ErrorType::TOO_MANY_ARGUMENTS_PASSED_TO_FUNCTION, sizeof...(Any),
          sizeof...(TFunctionKeys)>();
        return false;
      } 
      else 
      {

        constexpr EvalReturn error = evalAny<Any...>();
      
        if constexpr (error.errorType != ErrorType::NONE)
        {
          constexpr size_t idx = static_cast<size_t>(error.id);
      
          if constexpr ( error.isFuncID == 0 )
          {
            using KeyType = typename std::tuple_element<idx, std::tuple<TFunctionKeys...> >::type;
            constexpr auto keyName = KeyType::name;
            failWithMessage<error.errorType, keyName>();
          }
          else if constexpr ( error.isFuncID > 0) 
          {
            using KeyType = typename std::tuple_element<idx, std::tuple<Any...> >::type::keyType;
            constexpr auto keyName = KeyType::name;
            failWithMessage<error.errorType, keyName>();
          }
          else 
          {
            failWithMessage<error.errorType, error.id>();
          }
          return false;
        }
      }      

      return true;
    }
    
    template <class... Any, std::enable_if_t<evalAnyError<Any...>(), int> = 0>
    inline typename KeyFunctionTraits::ResultType operator()(Any&&... _args) const 
    {
      return internal3<Any...>(std::forward<Any>(_args)..., std::make_index_sequence<sizeof...(TFunctionKeys)>{});
    }

    template <typename T, std::enable_if_t<IsAssignedKey<T>::value,bool> = true>
    inline void* getAddress(T& _assignedKey) const
    {
      return (void*)_assignedKey.getValue();
    }

    template <typename T, std::enable_if_t<!IsAssignedKey<T>::value,int> = 0>
    inline void* getAddress(T& _value) const
    {
      return (void*)&_value;
    }

    template <int Idx, int KeyID>
    struct ConvertToType
    {
      using type = typename KeyFunctionTraits::template arg<Idx>::type;
    };

    template <int Idx>
    struct ConvertToType<Idx,KeyIdType::ABSENT>
    {
      using type = std::nullopt_t;
    };

    template <class... Any, size_t... Is>
    typename KeyFunctionTraits::ResultType internal3(Any&&... _args, std::index_sequence<Is...> const &) const
    {

      auto constexpr group = getNb<Any...>();
      constexpr int nbPositionals = group.first;
      constexpr int nbPassedArgs = sizeof...(Any);
      constexpr int nbFunctionKeys = sizeof...(TFunctionKeys);

      // potentially unordered local key
      // "local" meaning relative to function keys
      // e.g. for keys passed to KeyGen (1515, 845, 1233) 
      //      keys passed to function (845, 1515) become (1,0)
      constexpr std::array<int64_t,nbPassedArgs> passedLocalKeys = getLocalKeyIDs<Any...>();

      // get indices which will sort the local keys
      // indices[i] will give you the position in the arg list of key with id=i 
      // (positionals still have id=-1)
      constexpr std::array<int64_t,nbPassedArgs> sortedIndices = 
        getSortedIndices<Any...>(passedLocalKeys);

      // sort the local keys to get an idea of which ones are missing more easily
      constexpr auto sortByIndex = [](auto& _indices, const auto& _array)
      {
        std::array<int64_t, nbPassedArgs> out = {};
        for (size_t i = 0; i < _indices.size(); ++i)
        {
          out[i] = _array[_indices[i]];
        }
        return out;
      };

      constexpr std::array<int64_t,nbPassedArgs> sortedPassedLocalKeys 
        = sortByIndex(sortedIndices, passedLocalKeys);
      
      // now form a padded index list
      // paddedList[i] will return the position of key with id=i in the argument list
      // if entry is -1, it is a positional
      // if entry is -3, it is not present in the argument list
      constexpr auto getPaddedList = [](auto& _sortedKeys, auto& _indices)
      {
        std::array<int64_t, nbFunctionKeys> outList = {};
        for (int i = 0; i < nbPositionals; ++i)
        {
          outList[i] = KeyIdType::POSITIONAL;
        }

        int offset = 0;
        for (int i = nbPositionals; i < nbFunctionKeys; ++i)
        {
          if (i-offset < nbPassedArgs && _sortedKeys[i-offset] == i)
          {
            outList[i] = _indices[i-offset];
          }
          else 
          {
            ++offset;
            outList[i] = KeyIdType::ABSENT;
          }
        }

        return outList;
      };

      constexpr std::array<int64_t,nbFunctionKeys> paddedList = 
        getPaddedList(sortedPassedLocalKeys,sortedIndices);

      // now get Addresses
      std::array<void*,nbPassedArgs> addresses = { (void*)getAddress<Any>(_args)... };

      // padd them. put nullptr for absent args
      std::array<void*,nbFunctionKeys> paddedAddresses = {};

      for (int i = 0; i < nbPositionals; ++i)
      {
        paddedAddresses[i] = addresses[i];
      }      

      for (int i = nbPositionals; i < nbFunctionKeys; ++i)
      {
        int idx = paddedList[i];
        paddedAddresses[i] = (idx == KeyIdType::ABSENT) ? m_nullOpt : addresses[paddedList[i]];
      }

      return call(std::forward<typename ConvertToType<Is,paddedList[Is]>::type>
                  (
                    *((typename std::remove_reference<
                       typename ConvertToType<Is,paddedList[Is]>::type>::type*)
                      (paddedAddresses[Is]))
                  )...);
      
    }

    template <typename DFunctionPtr = TFunctionPtr, 
      std::enable_if_t<std::is_member_function_pointer<DFunctionPtr>::value,bool> = true>
    inline typename KeyFunctionTraits::ResultType call(typename TFunctionKeys::type&&... _args) const
    {
      return (m_classPtr->*m_baseFunction)(std::forward<typename TFunctionKeys::type>(_args)...);
    }

    template <typename DFunctionPtr = TFunctionPtr, 
      std::enable_if_t<!std::is_member_function_pointer<DFunctionPtr>::value,bool> = true>
    inline typename KeyFunctionTraits::ResultType call(typename TFunctionKeys::type&&... _args) const
    {
      return m_baseFunction(std::forward<typename TFunctionKeys::type>(_args)...);
    }

};

template <class DFunctionPtr, class... DFunctionKeys>
KeyGenClass(DFunctionPtr _function, const DFunctionKeys&... _keys) 
  -> KeyGenClass<DFunctionPtr,DFunctionKeys...>;

template <class DFunctionPtr, class... DFunctionKeys>
KeyGenClass(typename DFunctionPtr::ClassType _classPtr, DFunctionPtr _function, 
  const DFunctionKeys&... _keys) -> KeyGenClass<DFunctionPtr,DFunctionKeys...>;

#define INT64_T_MAX 9223372036854775807UL
#define UINT64_T_MAX 18446744073709551615UL

constexpr int64_t uniqueID(const char* seed)
{
  uint64_t num = 0;

  const uint64_t nbDigits = 20;
  const uint64_t nbFields = 10;

  uint64_t i = 0;
  while (true) {
    if (seed[i] == '\0')
    {
      break;
    }
    uint64_t idx = i % nbDigits;
    num += (uint64_t)seed[i] * (idx*nbFields) % INT64_MAX;
    ++i;
  }

  int64_t out = static_cast<int64_t>(num);
  return (out < 0) ? out + INT64_T_MAX : out;

}

#define UNPAREN(...) __VA_ARGS__ 
#define KEY(TYPE, ID) inline static const Key< UNPAREN TYPE , ID > 
#define KEYOPT(TYPE, ID) inline static const Key<std::optional< UNPAREN TYPE >, ID >
#define KEYGEN inline static const KeyGenClass

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define UNIQUE(name) uniqueID(#name TOSTRING(__LINE__) __TIME__ __DATE__)

#define PARAM(name, ...) \
  const inline static Key< __VA_ARGS__, UNIQUE(name)> name;

#define PARAM2(name, ...) \
  enum _ENUM_##name {     \
    _KEY_##name           \
  };                      \
  const inline static Key< __VA_ARGS__, UNIQUE(name), _KEY_##name> name;

#define OPTPARAM2(name, ...) \
  enum _ENUM_##name {     \
    _KEY_##name           \
  };                      \
  const inline static Key< std::optional< __VA_ARGS__ >, UNIQUE(name), _KEY_##name> name;

#define OPTPARAM(name, ...) const inline static Key<std::optional< __VA_ARGS__ >, UNIQUE(name)> name;
#define PARAMETRIZE(function, ...) const inline KeyGenClass np##_##function(&function, __VA_ARGS__);

#endif // NAMED_PARAMS_H
