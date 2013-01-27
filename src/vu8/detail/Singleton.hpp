#ifndef TSA_VU8_UTIL_SINGLETON_HPP
#define TSA_VU8_UTIL_SINGLETON_HPP

namespace vu8 { namespace detail {

template <class T>
class Singleton {
  public:
    struct object_creator {
        object_creator() { Singleton<T>::Instance(); }
        inline void do_nothing() const {}
    };
    static object_creator create_object;

    typedef T object_type;

    static object_type& Instance() {
        static object_type obj;
        create_object.do_nothing();
        return obj;
    }

    Singleton() {}
};

template <typename T>
typename Singleton<T>::object_creator
Singleton<T>::create_object;

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
