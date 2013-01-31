#ifndef TSA_VU8_UTIL_SINGLETON_HPP
#define TSA_VU8_UTIL_SINGLETON_HPP

namespace vu8 { namespace detail {

template <class T>
class LazySingleton {
  public:
    typedef T object_type;

    static object_type& Instance() {
        static object_type *obj = 0;
        if (! obj) obj = new object_type();
        return *obj;
    }

    LazySingleton() {}
};

} }
#endif
