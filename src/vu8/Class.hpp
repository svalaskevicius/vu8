#ifndef TSA_VU8_CLASS_HPP
#define TSA_VU8_CLASS_HPP

#include <vu8/ToV8.hpp>
#include <vu8/FromV8.hpp>
#include <vu8/Throw.hpp>
#include <vu8/Factory.hpp>
#include <vu8/CallFromV8.hpp>
#include <vu8/detail/Proto.hpp>
#include <vu8/detail/Singleton.hpp>
#include <vu8/detail/Class.hpp>
#include <vu8/detail/TypeTraits.hpp>
#include <vu8/detail/TypeSafety.hpp>

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/include/push_front.hpp>
#include <boost/fusion/include/join.hpp>

#include <boost/type_traits/is_void.hpp>

#include <iostream>
#include <stdexcept>

namespace vu8 {

namespace fu = boost::fusion;
namespace mpl = boost::mpl;

template <class T, class P, typename detail::MemFunProto<T, P>::method_type Ptr>
struct Method : detail::MemFun<T, P, Ptr> {};

template <class T>
class ClassSingleton;

template <class... Methods> struct Selector;

template <class M, class... Methods>
struct Selector<M, Methods...> {

    typedef boost::true_type is_selector;
    typedef boost::false_type is_empty;
    
    template <class T>
    static inline ValueHandle ForwardReturn(T *obj, const v8::Arguments& args) {
        if (suitable(args)) {
            return ClassSingleton<T>::template ForwardReturn<M>(obj, args);
        }
        return callNext<T, Selector<Methods...>>(obj, args);
    }
private:
    static inline bool suitable(const v8::Arguments& args) {
        return detail::ArgValidator<typename M::arguments>::test(args);
    }

    template <class T, class Next>
    static inline typename boost::enable_if<typename Next::is_empty, ValueHandle>::type
    callNext(T *, const v8::Arguments&) {
        throw std::runtime_error("no matches found for the function paramters");
    }

    template <class T, class Next>
    static inline typename boost::disable_if<typename Next::is_empty::type, ValueHandle>::type
    callNext(T *obj, const v8::Arguments& args) {
        return Next::template ForwardReturn<T>(obj, args);
    }
};

template <>
struct Selector<> {
    typedef boost::true_type is_selector;
    typedef boost::true_type is_empty;
};


template <class... Factories> struct FactorySelector;

template <class F, class... Factories>
struct FactorySelector<F, Factories...> {

    typedef boost::true_type is_selector;
    typedef boost::false_type is_empty;
    
    template <class C>
    static inline C *New(const v8::Arguments& args) {
        if (suitable<C>(args)) {
            return detail::ArgFactory<C, F>::New(args);
        }
        return callNext<C, FactorySelector<Factories...>>(args);
    }
private:
    template <class C> static inline bool
    suitable(const v8::Arguments& args) {
        return detail::ArgValidator<typename F::template Construct<C>::arguments>::test(args);
    }

    template <class C, class Next>
    static inline typename boost::enable_if<typename Next::is_empty, C*>::type
    callNext(const v8::Arguments&) {
        throw std::runtime_error("no matches found for the function paramters");
    }

    template <class C, class Next>
    static inline typename boost::disable_if<typename Next::is_empty, C*>::type
    callNext(const v8::Arguments& args) {
        return Next::template New<C>(args);
    }
};

template <>
struct FactorySelector<> {
    typedef boost::true_type is_selector;
    typedef boost::true_type is_empty;
};


template < class T >
struct Class;

template <class T>
class ClassSingleton
  : public detail::LazySingleton< ClassSingleton<T> > 
{
    typedef ClassSingleton<T> self;
    typedef ValueHandle (T::*MethodCallback)(const v8::Arguments& args);

    template <class Factory>
    static inline ValueHandle ConstructorFunction(const v8::Arguments& args) {
        try {
            return self::Instance().template WrapObject<Factory>(args);
        } catch (std::runtime_error const& e) {
            return Throw(e.what());
        }
    }

    // invoke passing javascript object argument directly
    template <class P>
    static inline typename boost::enable_if<
        detail::PassDirectIf<P>, typename P::return_type >::type
    Invoke(T *obj, const v8::Arguments& args) {
        return (obj->* P::method_pointer)(args);
    }

    template <class P>
    static inline typename boost::disable_if<
        detail::PassDirectIf<P>, typename P::return_type >::type
    Invoke(T *obj, const v8::Arguments& args) {
        return CallFromV8<P>(*obj, args);
    }
    
    static inline T* retrieveNativeObjectPtr(v8::Local<v8::Value> value)
    {
        while (value->IsObject()) {
            v8::Local<v8::Object> obj = value->ToObject();
            T * native = static_cast<T *>(obj->GetPointerFromInternalField(0));
            if (native) {
                return native;
            }
            value = obj->GetPrototype();
        }
        return NULL;
    }

    v8::Persistent<v8::FunctionTemplate> _classFunc;
    v8::Persistent<v8::FunctionTemplate> _constructorFunc; 

public:

    template <class P>
    static inline typename
    boost::enable_if<vu8::is_to_v8_convertible<typename P::return_type>,
    ValueHandle >::type ForwardReturn (T *obj, const v8::Arguments& args) {
        return ToV8(Invoke<P>(obj, args));
    }

    template <class P>
    static inline typename
    boost::enable_if<boost::is_void<typename P::return_type>,
    ValueHandle >::type ForwardReturn (T *obj, const v8::Arguments& args) {
        Invoke<P>(obj, args);
        return v8::Undefined();
    }

    template <class P>
    static inline typename
    boost::enable_if<
        typename t_logical_and<
            typename P::IS_RETURN_WRAPPED_CLASS,
            typename t_negate< typename vu8::is_to_v8_convertible<typename P::return_type>::type >::type
        >::type,
    ValueHandle>::type ForwardReturn (T *obj, const v8::Arguments& args) {
        typedef typename vu8::ClassSingleton<typename remove_reference_and_const<typename P::return_type>::type> LocalSelf;
        typedef typename remove_reference_and_const<typename P::return_type>::type ReturnType;
// TODO check if we already have the given ref if returned by T&
        v8::HandleScope scope;
        ReturnType* return_value = new ReturnType(Invoke<P>(obj, args));

        detail::Instance::set(return_value);

        v8::Local<v8::Object> localObj =
            LocalSelf::Instance().ClassFunctionTemplate()->GetFunction()->NewInstance();
        v8::Persistent<v8::Object> persistentObj =
            v8::Persistent<v8::Object>::New(localObj);
        persistentObj->SetInternalField(0, v8::External::New(return_value));
        persistentObj.MakeWeak(return_value, &LocalSelf::MadeWeak);
        return scope.Close(localObj);
    }

    template <class P>
    static inline typename
    boost::enable_if<typename P::is_selector,
    ValueHandle>::type ForwardReturn (T *obj, const v8::Arguments& args) {
        return P::template ForwardReturn<T>(obj, args);
    }

    ClassSingleton()
      : _classFunc(v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New()))
    {
        _classFunc->InstanceTemplate()->SetInternalFieldCount(1);
        _constructorFunc = _classFunc;
    }

    template<class Factory>
    v8::Handle<v8::Object> WrapObject(const v8::Arguments& args) {
        v8::HandleScope scope;
        T *wrap = detail::ArgFactory<T, Factory>::New(args);
        v8::Local<v8::Object> localObj = _classFunc->GetFunction()->NewInstance();
        v8::Persistent<v8::Object> obj =
            v8::Persistent<v8::Object>::New(localObj);

        obj->SetPointerInInternalField(0, wrap);
        obj.MakeWeak(wrap, &self::MadeWeak);
        return scope.Close(obj);
    }

    v8::Persistent<v8::FunctionTemplate>& ClassFunctionTemplate() {
        return _classFunc;
    }
    
    v8::Persistent<v8::FunctionTemplate>& ConstructorFunctionTemplate() {
        return _constructorFunc;
    }

    static inline void MadeWeak(v8::Persistent<v8::Value> object,
                                void                     *parameter)
    {
        T *obj = static_cast<T *>(parameter);
        detail::Instance::del(obj);
        delete(obj);
        object.Dispose();
        object.Clear();
    }

    // every method is run inside a handle scope
    template <class P>
    static inline ValueHandle Forward(const v8::Arguments& args) {
        v8::HandleScope scope;

        // this will kill without zero-overhead exception handling
        try {
            return scope.Close(
                ForwardReturn<P>(
                    retrieveNativeObjectPtr(args.Holder()),
                    args
                )
            );
        } catch (std::runtime_error const& e) {
            return Throw(e.what());
        }
    }

    template <class P>
    inline void Method(char const *name) {
        _classFunc->PrototypeTemplate()->Set(
            v8::String::New(name),
            v8::FunctionTemplate::New(&self::template Forward<P>)
        );
    }

    template <class S>
    inline void Constructor() {
        _constructorFunc = v8::Persistent<v8::FunctionTemplate>::New(
            v8::FunctionTemplate::New(&self::template ConstructorFunction<S>)
        );
        _classFunc->Inherit(_constructorFunc);
    }
};

// Interface for registering C++ classes with v8
// T = class
// Factory = factory for allocating c++ object
//           by default Class uses zero-argument constructor
template <class T>
struct Class {
    typedef ClassSingleton<T>  singleton_t;

    template <class P>
    inline Class &Method(char const *name) {
        Instance().template Method<P>(name);
        return *this;
    }

    template <class S>
    inline Class& Constructor() {
        Instance().template Constructor<S>();
        return *this;
    }
  public:

    static inline singleton_t& Instance() { return singleton_t::Instance(); }

    template <class To>
    inline Class& ConvertibleTo() {
        detail::register_convertible_type<T, To>();
        return *this;
    }

    // method with any prototype
    template <class P, typename detail::MemFunProto<T, P>::method_type Ptr>
    inline Class& Set(char const *name) {
        return Method< detail::MemFun<T, P, Ptr> >(name);
    }

    template <class B, class P, typename detail::MemFunProto<B, P>::method_type Ptr>
    inline typename boost::enable_if<typename boost::is_base_of<B, T>::type, Class&>::type Set(char const *name) {
        return Method< detail::MemFun<B, P, Ptr> >(name);
    }

    // method with selector
    template <class S>
    inline Class& Set(char const *name) {
        return Method< S >(name);
    }

    // passing v8::Arguments directly but modify return type
    template <class R, R (T::*Ptr)(const v8::Arguments&)>
    inline Class& Set(char const *name) {
        return Set<R(const v8::Arguments&), Ptr>(name);
    }

    // passing v8::Arguments and return ValueHandle directly
    template <ValueHandle (T::*Ptr)(const v8::Arguments&)>
    inline Class& Set(char const *name) {
        return Method<ValueHandle(const v8::Arguments&), Ptr>(name);
    }

    inline v8::Persistent<v8::FunctionTemplate>& ClassFunctionTemplate() {
        return Instance().ClassFunctionTemplate();
    }

    inline v8::Persistent<v8::FunctionTemplate>& ConstructorFunctionTemplate() {
        return Instance().ConstructorFunctionTemplate();
    }

    // create javascript object which references externally created C++
    // class. It will not take ownership of the C++ pointer.
    static inline v8::Handle<v8::Object> ReferenceExternal(T *ext) {
        v8::HandleScope scope;
        v8::Local<v8::Object> obj = Instance()._classFunc->GetFunction()->NewInstance();
        obj->SetPointerInInternalField(0, ext);
        return scope.Close(obj);
    }

    // As ReferenceExternal but delete memory for C++ object when javascript
    // object is deleted. You must use "new" to allocate ext.
    static inline v8::Handle<v8::Object> ImportExternal(T *ext) {
        v8::HandleScope scope;
        v8::Local<v8::Object> localObj = Instance()._classFunc->GetFunction()->NewInstance();

        v8::Persistent<v8::Object> obj = v8::Persistent<v8::Object>::New(localObj);

        obj->SetPointerInInternalField(0, ext);
        obj.MakeWeak(ext, &singleton_t::MadeWeak);

        return scope.Close(obj);
    }

    Class() {}
};

}
#endif
