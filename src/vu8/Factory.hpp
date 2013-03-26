#ifndef BOOST_PP_IS_ITERATING
#   ifndef TSA_VU8_FACTORY_HPP
#   define TSA_VU8_FACTORY_HPP
#       include <vu8/config.hpp>

#       include <boost/preprocessor/repetition.hpp>
#       include <boost/preprocessor/arithmetic/sub.hpp>
#       include <boost/preprocessor/punctuation/comma_if.hpp>
#       include <boost/preprocessor/iteration/iterate.hpp>

#       include <boost/mpl/vector.hpp>

#       include <v8.h>


#       ifndef VU8_FACTORY_MAX_SIZE
#         define VU8_FACTORY_MAX_SIZE VU8_PP_ITERATION_LIMIT
#       endif

#       define VU8_FACTORY_header(z, n, data) class BOOST_PP_CAT(T,n) = none

namespace vu8 {

struct none {};

// Factory that calls C++ constructor with v8::Arguments directly
struct V8ArgFactory {};

// Factory that prevents class from being constructed in JavaScript
struct NoFactory {};

// primary template
template <class C, BOOST_PP_ENUM(VU8_FACTORY_MAX_SIZE, VU8_FACTORY_header, ~)>
struct ClassFactory;

template <BOOST_PP_ENUM(VU8_FACTORY_MAX_SIZE, VU8_FACTORY_header, ~)>
struct Factory;

}

#       define BOOST_PP_ITERATION_LIMITS (0, VU8_FACTORY_MAX_SIZE - 1)
#       define BOOST_PP_FILENAME_1       "vu8/Factory.hpp"
#       include BOOST_PP_ITERATE()
#   endif // include guard

#else // BOOST_PP_IS_ITERATING

#  define n BOOST_PP_ITERATION()
#  define VU8_FACTORY_default(z, n, data) data
#  define VU8_FACTORY_args(z, n, data) BOOST_PP_CAT(T,n) BOOST_PP_CAT(arg,n)


namespace vu8 {

template <class C  BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class T)>
struct ClassFactory<
    C
    BOOST_PP_COMMA_IF(n)
    BOOST_PP_ENUM_PARAMS(n, T)
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(VU8_FACTORY_MAX_SIZE,n))
    BOOST_PP_ENUM(BOOST_PP_SUB(VU8_FACTORY_MAX_SIZE,n), VU8_FACTORY_default, none)
>
{
    typedef ClassFactory<C BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n,T)> self;
    typedef boost::mpl::vector<BOOST_PP_ENUM_PARAMS(n,T)> arguments;
    typedef boost::false_type is_selector;
    typedef C*(self::*method_type)(BOOST_PP_ENUM(n, VU8_FACTORY_args, ~));

    typedef C* return_type;

    return_type operator()(BOOST_PP_ENUM(n, VU8_FACTORY_args, ~)) {
        return new C(BOOST_PP_ENUM_PARAMS(n,arg));
    }

    static constexpr method_type method_pointer = &self::operator();
};

#define ClassFactoryType ClassFactory< \
    C \
    BOOST_PP_COMMA_IF(n) \
    BOOST_PP_ENUM_PARAMS(n, T) \
    BOOST_PP_COMMA_IF(BOOST_PP_SUB(VU8_FACTORY_MAX_SIZE,n)) \
    BOOST_PP_ENUM(BOOST_PP_SUB(VU8_FACTORY_MAX_SIZE,n), VU8_FACTORY_default, none) \
>
template <class C  BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n, class T)>
constexpr typename ClassFactoryType::method_type ClassFactoryType::method_pointer;
#undef ClassFactoryType




// specialization pattern
template <BOOST_PP_ENUM_PARAMS(n, class T)>
struct Factory<
    BOOST_PP_ENUM_PARAMS(n,T)
    BOOST_PP_COMMA_IF(n)
    BOOST_PP_ENUM(BOOST_PP_SUB(VU8_FACTORY_MAX_SIZE,n), VU8_FACTORY_default, none)
>
{
    typedef boost::false_type is_selector;

    template <class C>
    struct Construct : ClassFactory<C BOOST_PP_COMMA_IF(n) BOOST_PP_ENUM_PARAMS(n,T)> {};
};

}
#   undef VU8_FACTORY_default
#   undef VU8_FACTORY_args
#   undef n

#endif
