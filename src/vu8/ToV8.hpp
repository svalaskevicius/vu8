#ifndef TSA_VU8_TO_V8_HPP
#define TSA_VU8_TO_V8_HPP

#include <v8.h>

#include <string>

#include <stdint.h>

namespace vu8 {

typedef v8::Handle<v8::Value> ValueHandle;

static inline ValueHandle ToV8(ValueHandle src) { return src; }

static inline ValueHandle ToV8(std::string const& src) {
    return v8::String::New(src.c_str());
}

static inline ValueHandle ToV8(char const *src) {
    return v8::String::New(src);
}

static inline ValueHandle ToV8(double const src) {
    return v8::Number::New(src);
}

static inline ValueHandle ToV8(float const src) {
    return v8::Number::New(src);
}

static inline ValueHandle ToV8(int32_t const src) {
    return v8::Int32::New(src);
}

static inline ValueHandle ToV8(uint32_t const src) {
    return v8::Uint32::New(src);
}

static inline ValueHandle ToV8(int64_t const src) {
#ifdef VU8_CHECK_NUMERIC_BOUNDS
    // not supported yet
#else
    return v8::Number::New(src);
#endif
}

static inline ValueHandle ToV8(uint64_t const src) {
#ifdef VU8_CHECK_NUMERIC_BOUNDS
    // not supported yet
#else
    return v8::Number::New(src);
#endif
}

static inline ValueHandle ToV8(long long const src) {
#ifdef VU8_CHECK_NUMERIC_BOUNDS
    // not supported yet
#else
    return v8::Number::New(src);
#endif
}

static inline ValueHandle ToV8(unsigned long long const src) {
#ifdef VU8_CHECK_NUMERIC_BOUNDS
    // not supported yet
#else
    return v8::Number::New(src);
#endif
}


template <class T>
static inline typename
    boost::enable_if<boost::is_enum<T>,
    ValueHandle >::type ToV8(T const src) {
    return ToV8( (typename std::underlying_type<T>::type) src);
}

static inline ValueHandle ToV8(bool const src) {
    return v8::Boolean::New(src);
}

}
#endif

