#ifndef TSA_VU8_DETAIL_CLASS_HPP
#define TSA_VU8_DETAIL_CLASS_HPP

#include <vu8/config.hpp>
#include <vu8/detail/MakeArgStorage.hpp>
#include <vu8/detail/FromV8Arguments.hpp>

#include <v8.h>

#include <boost/mpl/front.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/fusion/container/vector.hpp>

#define BOOST_FUSION_INVOKE_MAX_ARITY VU8_PP_ITERATION_LIMIT
#include <boost/fusion/include/invoke.hpp>

#include <boost/any.hpp>
#include <map>

namespace vu8 { namespace detail {

static std::map<void *, boost::any> instanceMap;

template <class P, typename Enable = void>
struct PassDirectIf : boost::false_type {};

template <class P>
struct PassDirectIf<P, typename P::arguments> : boost::is_same<
    const v8::Arguments&,
    typename mpl::front<typename P::arguments>::type> {};

struct ArgValidatorBase 
{
    template <class Arg>
    static inline bool arg_suitable(const v8::Local<v8::Value> &v8arg) {
        return FromV8<Arg>::test(v8arg); 
    }
};

template <class Args, int N = fu::result_of::size<Args>::value - 1>
struct ArgValidator : ArgValidatorBase 
{
    static inline bool test(const v8::Arguments& v8args) {
        if (fu::result_of::size<Args>::value != v8args.Length()) {
            return false;
        }
        return args_suitable(v8args);
    }
    static inline bool args_suitable(const v8::Arguments& v8args) {
        if (!ArgValidatorBase::template arg_suitable<typename mpl::at_c<Args, N>::type>(v8args[N])) {
            return false;
        }
        return ArgValidator<Args, N-1>::args_suitable(v8args);
    }

};

template <class Args>
struct ArgValidator<Args, -1> : ArgValidatorBase 
{
    static inline bool test(const v8::Arguments& v8args) {
        return v8args.Length() == 0;
    }
    static inline bool args_suitable(const v8::Arguments&) {
        return true;
    }
};

template <class T, class F, typename Enable = void>
struct ArgFactory;

template <class T, class F>
struct ArgFactory<T, F, typename boost::disable_if<typename F::is_selector>::type> {
    static inline T * New(const v8::Arguments& args) {
        typedef typename F::template Construct<T>  factory_t;
        factory_t factory;
        return CallFromV8<factory_t, factory_t>(factory, args);
    }
};

template <class T, class F>
struct ArgFactory<T, F, typename boost::enable_if<typename F::is_selector>::type> {
    static inline T * New(const v8::Arguments& args) {
        return F::template New<T>(args);
    }
};

template <class T>
struct ArgFactory<T, V8ArgFactory> {
    static inline T *New(const v8::Arguments& args) {
        return new T(args);
    }
};

} }
#endif
