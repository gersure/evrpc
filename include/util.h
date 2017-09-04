#ifndef _UTIL_H
#define _UTIL_H

#include <memory>
#include <string>
#include <memory>
#include <typeindex>
#include <glog/logging.h>

class noncopyable
{
  protected:
    noncopyable() {}
    ~noncopyable() {}
  private:  // emphasize the following members are private
    noncopyable( const noncopyable& );
    const noncopyable& operator=( const noncopyable& );
};


template<typename T>
inline T* get_pointer(const std::shared_ptr<T>& ptr)
{
  return ptr.get();
}

template<typename T>
inline T* get_pointer(const std::unique_ptr<T>& ptr)
{
  return ptr.get();
}


template<typename To, typename From>
inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From>& f) {
  return ::std::static_pointer_cast<To>(f);
}



struct Any
{
    Any(void) : m_tpIndex(std::type_index(typeid(void))){}
    Any(const Any& that) : m_ptr(that.Clone()), m_tpIndex(that.m_tpIndex) {}
    Any(Any && that) : m_ptr(std::move(that.m_ptr)), m_tpIndex(that.m_tpIndex) {}

    //创建智能指针时，对于一般的类型，通过std::decay来移除引用和cv符，从而获取原始类型
    template<typename U, class = typename std::enable_if<!std::is_same<typename std::decay<U>::type, Any>::value, U>::type> Any(U && value) :
      m_ptr(new Derived < typename std::decay<U>::type>(std::forward<U>(value))),
      m_tpIndex(std::type_index(typeid(typename std::decay<U>::type))){}

    bool IsNull() const { return !bool(m_ptr); }

    template<class U> bool Is() const
    {
      return m_tpIndex == std::type_index(typeid(U));
    }

    //将Any转换为实际的类型
    template<class U>
    U& AnyCast()
    {
      if (!Is<U>())
      {
//        cout << "can not cast " << typeid(U).name() << " to " << m_tpIndex.name() << endl;
        throw std::runtime_error("any context error!");
      }

      auto derived = dynamic_cast<Derived<U>*> (m_ptr.get());
      return derived->m_value;
    }

    Any& operator=(const Any& a)
    {
      if (m_ptr == a.m_ptr)
        return *this;

      m_ptr = a.Clone();
      m_tpIndex = a.m_tpIndex;
      return *this;
    }

  private:
    struct Base;
    typedef std::unique_ptr<Base> BasePtr;

    struct Base
    {
        virtual ~Base() {}
        virtual BasePtr Clone() const = 0;
    };

    template<typename T>
    struct Derived : Base
    {
        template<typename U>
        Derived(U && value) : m_value(std::forward<U>(value)) { }

        BasePtr Clone() const
        {
          return BasePtr(new Derived<T>(m_value));
        }

        T m_value;
    };

    BasePtr Clone() const
    {
      if (m_ptr != nullptr)
        return m_ptr->Clone();
      return nullptr;
    }

    BasePtr m_ptr;
    std::type_index m_tpIndex;
};

#endif
