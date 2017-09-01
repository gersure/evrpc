#ifndef _UTIL_H
#define _UTIL_H

#include <memory>

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


#endif
