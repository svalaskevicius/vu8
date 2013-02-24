#ifndef TSA_VU8_DETAIL_FROM_V8_HPP
#define TSA_VU8_DETAIL_FROM_V8_HPP

#include <vu8/detail/ConvertibleString.hpp>
#include <vu8/detail/TypeSafety.hpp>

#include <v8.h>

#include <string>
#include <stdexcept>
#include <vector>
#include <map>
#include <iostream>

#include <boost/utility.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/type_traits/is_class.hpp>

namespace vu8 { namespace detail {

typedef v8::Handle<v8::Value> ValueHandle;

template <class T>
struct FromV8Base {
    typedef T result_type;
};

template <class T, class Enable = void>
struct FromV8;

template <>
struct FromV8<std::string> : FromV8Base<std::string> {
    static inline bool test(const ValueHandle & value) {
        return value->IsString();
    }
    static inline std::string exec(const ValueHandle & value) {
        if (! test(value))
            throw std::runtime_error("cannot make string from non-string type");

        v8::String::Utf8Value str(value);
        return *str;
    }
};

template <>
struct FromV8<ConvertibleString> : FromV8Base<ConvertibleString> {
    static inline bool test(const ValueHandle & value) {
        return value->IsString();
    }
    static inline ConvertibleString exec(const ValueHandle & value) {
        if (! test(value))
            throw std::runtime_error("cannot make string from non-string type");

        v8::String::Utf8Value str(value);
        return *str;
    }
};

// char const * and char const * have to be copied immediately otherwise
// the underlying memory will die due to the way v8 strings work.
template <>
struct FromV8<char const *> : FromV8<ConvertibleString> {};

template <>
struct FromV8<char const * const> : FromV8<ConvertibleString> {};

template <>
struct FromV8< v8::Handle<v8::Function> >
  : FromV8Base< v8::Handle<v8::Function> >
{
    static inline bool test(const ValueHandle & value) {
        return value->IsFunction();
    }
    static inline v8::Handle<v8::Function> exec(ValueHandle value) {
        if (! test(value))
            throw std::runtime_error("expected javascript function");

        return value.As<v8::Function>();
    }
};

template <>
struct FromV8<bool> : FromV8Base<bool> {
    static inline bool test(const ValueHandle &) {
        return true;
    }
    static inline bool exec(const ValueHandle & value) {
        return value->ToBoolean()->Value();
    }
};

template <>
struct FromV8<int32_t> : FromV8Base<int32_t> {
    static inline bool test(const ValueHandle & value) {
        return value->IsNumber();
    }
    static inline int32_t exec(const ValueHandle & value) {
        if (! test(value))
            throw std::runtime_error("expected javascript number");

        return value->ToInt32()->Value();
    }
};

template <>
struct FromV8<uint32_t> : FromV8Base<uint32_t> {
    static inline bool test(const ValueHandle & value) {
        return value->IsNumber();
    }
    static inline uint32_t exec(ValueHandle value) {
        if (! test(value))
            throw std::runtime_error("expected javascript number");

        return value->ToUint32()->Value();
    }
};

template <class T>
struct FromV8<T, typename boost::enable_if<boost::is_integral<T> >::type> : FromV8Base<T> {
    static inline bool test(const ValueHandle & value) {
        return value->IsNumber();
    }
    static inline T exec(ValueHandle value) {
        if (! test(value))
            throw std::runtime_error("expected javascript number");

        return static_cast<T>(value->ToNumber()->Value());
    }
};

template <class T>
struct FromV8<T, typename boost::enable_if<boost::is_enum<T> >::type> : FromV8Base<T> {
    static inline bool test(const ValueHandle & value) {
        return value->IsNumber();
    }
    static inline T exec(ValueHandle value) {
        if (! test(value))
            throw std::runtime_error("expected javascript number");

        return static_cast<T>(value->ToNumber()->Value());
    }
};

template <class T>
struct FromV8<T, typename boost::enable_if<boost::is_floating_point<T> >::type> : FromV8Base<T> {
    static inline bool test(const ValueHandle & value) {
        return value->IsNumber();
    }
    static inline T exec(ValueHandle value) {
        if (! test(value))
            throw std::runtime_error("expected javascript number");

        return static_cast<T>(value->ToNumber()->Value());
    }
};


template <class T, class A>
struct FromV8< std::vector<T, A> > : FromV8Base< std::vector<T, A> > {
    static inline bool test(const ValueHandle & value) {
        return value->IsArray();
    }
    static inline std::vector<T, A> exec(ValueHandle value) {
        if (! test(value))
            throw std::runtime_error("expected javascript array");

        v8::Array *array = v8::Array::Cast(*value);
        std::vector<T, A> result;
        for (uint32_t i = 0; i < array->Length(); ++i) {
            v8::Local<v8::Value> obj = array->Get(i);
            result.push_back(FromV8<T>::exec(obj));
        }
        return result;
    }
};

template <>
struct FromV8<ValueHandle> : FromV8Base<ValueHandle> {
    static inline bool test(const ValueHandle &) {
        return true;
    }
    static inline ValueHandle exec(ValueHandle value) {
        return value;
    }
};

////////////////////////////////////////////////////////////////////////////
// extracting classes

static inline void* retrieveNativeObjectPtr(v8::Handle<v8::Value> value)
{
    while (value->IsObject()) {
        v8::Local<v8::Object> obj = value->ToObject();
        void * native = obj->GetPointerFromInternalField(0);
        if (native) {
            return native;
        }
        value = obj->GetPrototype();
    }
    return NULL;
}

template <class T>
static inline bool test_v8_object(const ValueHandle & value) {
    if (!value->IsObject()) {
        return false;
    }
    bool ret = (bool) Instance::get<T>(retrieveNativeObjectPtr(value));
    if (!ret) {
        std::cerr << "failed to match value for type "<<typeid(T*).name()<<std::endl;
    } else {
        std::cerr << "ptr matched "<<typeid(T*).name()<<std::endl;
    }
    return ret;
}

template <class T>
struct FromV8Ptr : FromV8Base<T> {
    static inline bool test(const ValueHandle & value) {
        if (value->IsNull()) {
            return true;
        }
        return test_v8_object<typename std::remove_pointer<T>::type>(value);
    }
    static inline T exec(ValueHandle value) {
        if (!test(value)) {
            throw std::runtime_error("expected object or null");
        }
        if (value->IsNull()) {
            return nullptr;
        }

        return static_cast<T>(retrieveNativeObjectPtr(value));
    }
};

template <class T>
struct FromV8<T *> : FromV8Ptr<T *> {};

template <class T>
struct FromV8<T const *> : FromV8Ptr<T const *> {};

//////////////////////////////////////////////////////////////////////////////
// deal with references
template <class T, class U>
struct FromV8Ref {
    typedef U result_type;

    static inline bool test(const ValueHandle & value) {
        return test_v8_object<T>(value);
    }
    static inline U exec(ValueHandle value) {
        if (! test(value)) {
            throw std::runtime_error("expected object");
        }

        return *static_cast<T *>(retrieveNativeObjectPtr(value));
    }
};

template <class U>
struct FromV8Ref<std::string, U> : FromV8<std::string> {};

template <class T, class U, class R>
struct FromV8Ref<std::vector<T, U>, R> : FromV8< std::vector<T, U> > {};

template <class T>
struct FromV8<T const &> : FromV8Ref<T, T const&> {};

template <class T>
struct FromV8<T&> : FromV8Ref<T, T&> {};

template <class T>
struct FromV8<T, typename boost::enable_if<boost::is_class<T> >::type > : FromV8Ref<T, T&> {};

} }
#endif
