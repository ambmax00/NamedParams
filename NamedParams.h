#ifndef NAMED_PARAMS_H
#define NAMED_PARAMS_H

#include <array>
#include <functional>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <tuple>

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

template <class T>
constexpr void swap(T& _a, T& _b)
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
inline constexpr Iterator partition(Iterator _begin, Iterator _end, Compare _comp)
{
  Iterator pivot = _end - 1; 
  Iterator smallest = _begin - 1;

  for (auto it = _begin; it < pivot; ++it)
  {
    if (_comp(*it,*pivot))
    {
      smallest++;
      swap(*smallest,*it);
    }
  }

  swap(*(smallest+1), *(_end-1));
  return smallest+1;
}

template <class Iterator, class Compare>
inline constexpr void sort(Iterator _begin, Iterator _end, Compare _comp)
{
  if (_begin == _end)
  {
    return;
  }

  if (_begin < _end-1)
  {
    auto pivot = partition(_begin, _end, _comp);
    sort(_begin, pivot, _comp);
    sort(pivot, _end, _comp);
  }

  return;
}

template <class Iterator>
inline constexpr void sort(Iterator _begin, Iterator _end)
{
  auto comp = [](const decltype(*_begin)& _a, const decltype(*_end)& _b)
  {
    return _a < _b;
  };
  sort(_begin, _end, comp);
}

template <class KeyType, std::enable_if_t<
  (IsKey<KeyType>::value || std::is_same<KeyType,void>::value),bool> = true>
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

    typedef KeyType Type;

};

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
      auto k = AssignedKey<Key>::template build<T>(_any);
      return k;
    }

    typedef T type;

    static const int ID = UNIQUE_ID;

};

template <int i>
class ConstExprError;

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

    struct EvalReturn
    {
      bool success;
      int type;
      int id;
    };

    template <class... AssignedKeyPack>
    constexpr static inline EvalReturn eval()
    {
      constexpr int nbAssignedKeys = sizeof...(AssignedKeyPack);
      constexpr int nbFunctionKeys = sizeof...(FunctionKeys);

      std::array<int,nbAssignedKeys> passedKeys = {AssignedKeyPack::Type::ID...};
      std::array<int,nbFunctionKeys> functionKeys = {FunctionKeys::ID...};

      std::array<bool,nbFunctionKeys> functionKeyIsOptional = {
        IsOptional<typename FunctionKeys::type>::value...};
      std::array<bool,nbAssignedKeys> passedKeyIsOptional = {
        IsOptional<typename AssignedKeyPack::Type::type>::value...};

      // check for empty assigned keys
      int nbRequiredKeys = 0;
      for (auto b : functionKeyIsOptional)
      {
        nbRequiredKeys += (b) ? 0 : 1;
      }

      if (nbRequiredKeys != 0 && nbAssignedKeys == 0)
      {
        return EvalReturn{false, 2, 0};
      }    

      if (nbRequiredKeys == 0 && nbAssignedKeys == 0) 
      {
        return EvalReturn{true, 0, 0};
      } 

      sort(passedKeys.begin(), passedKeys.end());
      EvalReturn evalReturn = {true, 0, 0};
      int offset = 0;

      for (int iArg = 0; iArg < nbFunctionKeys; ++iArg)
      {
        if (iArg >= nbAssignedKeys+offset && !functionKeyIsOptional[iArg])
        {
          evalReturn.success = false;
          evalReturn.type = 0;
          evalReturn.id = functionKeys[iArg];
          break;
        }
        else if (functionKeys[iArg] > passedKeys[iArg-offset])
        {
          evalReturn.success = false;
          evalReturn.type = 1;
          evalReturn.id = passedKeys[iArg-offset];
          break;
        }
        else if (functionKeys[iArg] < passedKeys[iArg-offset] && !functionKeyIsOptional[iArg])
        {
          evalReturn.success = false;
          evalReturn.type = 0;
          evalReturn.id = functionKeys[iArg];
          break;
        }
        else if (functionKeys[iArg] < passedKeys[iArg-offset] && functionKeyIsOptional[iArg])
        {
          offset++;
        }
      }

      return evalReturn;
    }

    template <class... AssignedKeyPack>
    constexpr static inline int evalError()
    {
      constexpr EvalReturn error = eval<AssignedKeyPack...>();

      if constexpr (!error.success && error.type == 0) 
      {
        ConstExprError<error.id> MISSING_KEY;
      }

      if constexpr (!error.success && error.type == 1)
      {
        ConstExprError<error.id> INVALID_KEY;
      }

      if constexpr (!error.success && error.type == 2)
      {
        ConstExprError<error.id> MISSING_KEY;
      }
    
      return 0;
    }


    template <class... AssignedKeyPack, std::enable_if_t<
    evalError<AssignedKeyPack...>() == 0,
    int> = 0>
    typename KeyFunctionTraits::ResultType operator()(AssignedKeyPack... _args) const 
    {
      return internal<AssignedKeyPack...>(_args..., std::make_index_sequence<sizeof...(FunctionKeys)>{});
    }

    struct AnyKey 
    {
      int id;
      void* ptr;
    };

    template <class... AssignedKeyPack, size_t... Is>
    typename KeyFunctionTraits::ResultType internal(AssignedKeyPack&... _args, std::index_sequence<Is...> const &) const
    {
      std::array<AnyKey,sizeof...(AssignedKeyPack)> passedKeys = {
        AnyKey{_args.getKeyID(), (void*)&_args}...};

      sort(passedKeys.begin(), passedKeys.end(), 
        [](const auto& _p0, const auto& _p1)
        {
          return _p0.id < _p1.id;
        }
      );

      std::tuple<FunctionKeys...> functionKeyTypes;
      int offset = 0;

      auto process = [&] <class KeyType> (KeyType& _key, int _idx) -> typename KeyType::type
      { 
        
        if constexpr (IsOptional<typename KeyType::type>::value)
        {
          // no more passed keys left, return empty optional
          if (_idx >= sizeof...(_args) + offset)
          {
            typename KeyType::type out = std::nullopt;
            return out;
          }
          // function paramter nr. _idx not present, fill woth nullopt
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

        // SHOULD NOT HAPPEN
        throw std::runtime_error("This should not happen");

      };

      std::tuple<typename FunctionKeys::type...> paddedParameters =
        {process(std::get<Is>(functionKeyTypes), Is)...};

      return m_baseFunction(std::get<Is>(paddedParameters)...);

    }

};

#if 0
class Register
{
  private: 
    
    inline static std::map<void*,std::vector<int>> m_functionKeyMap;

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
