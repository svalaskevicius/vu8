#ifndef TYPETRAITS_HPP_
#define TYPETRAITS_HPP_

#include <string>

#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include <boost/utility/enable_if.hpp>

#include <v8.h>

namespace vu8 {

template <class T> struct remove_reference_and_const {
    typedef typename boost::remove_const<
        typename boost::remove_reference<T>::type
    >::type type;
};

template <class T>
struct t_negate;

template<>
struct t_negate<boost::true_type> : boost::false_type {};
template<>
struct t_negate<boost::false_type> : boost::true_type {};


template <class T, class O>
struct t_logical_and : boost::false_type {};
template <>
struct t_logical_and<boost::true_type, boost::true_type> : boost::true_type {};

template <class T, class O>
struct t_logical_or : boost::true_type {};
template <>
struct t_logical_or<boost::false_type, boost::false_type> : boost::false_type {};

template <class T, class Enable = void>
struct is_to_v8_convertible : public boost::false_type { };

template <class T>
struct is_to_v8_convertible<T, typename boost::enable_if<
    boost::is_arithmetic<T>
>::type> : public boost::true_type { };

template <class T>
struct is_to_v8_convertible<T, typename boost::enable_if<
    boost::is_enum<T>
>::type> : public boost::true_type { };

template <class T>
struct is_to_v8_convertible<T, typename boost::enable_if<
    boost::is_pointer<T>
>::type> : public boost::true_type { };

template <class T>
struct is_to_v8_convertible<T &> : public is_to_v8_convertible<T> { };

#define VU8_TO_V8_CONV_TYPE_TRAIT_SPEC(T,M,spec) \
template <class T > \
struct is_to_v8_convertible<T, typename boost::enable_if< \
    boost::is_same<M, spec > \
>::type> : public boost::true_type { }; \
/**/

VU8_TO_V8_CONV_TYPE_TRAIT_SPEC(T,T,v8::Handle<v8::Value>);
VU8_TO_V8_CONV_TYPE_TRAIT_SPEC(T,typename remove_reference_and_const<T>::type, std::string);
}

#endif /* TYPETRAITS_HPP_ */
