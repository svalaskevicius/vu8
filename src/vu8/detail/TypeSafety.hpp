
#pragma once

#include <map>
#include <list>
#include <memory>
#include <iostream>


namespace vu8 { namespace detail {


struct any {
    any() : content(0) {}

    template<typename ValueType>
    any(const ValueType & value) : content(new holder<ValueType>(value)) {}

    any(const any & other) : content(other.content ? other.content->clone() : 0) {}

    ~any() {
        delete content;
    }

    any & swap(any & rhs) {
        std::swap(content, rhs.content);
        return *this;
    }

    template<typename ValueType>
    any & operator=(const ValueType & rhs) {
        any(rhs).swap(*this);
        return *this;
    }

    any & operator=(any rhs) {
        rhs.swap(*this);
        return *this;
    }

    bool empty() const {
        return !content;
    }

    const std::type_info & type() const {
        return content ? content->type() : typeid(void);
    }

    struct placeholder {
        virtual ~placeholder() {
        }
        virtual const std::type_info & type() const = 0;
        virtual placeholder * clone() const = 0;
    };

    template<typename ValueType>
    struct holder : placeholder {
        holder(const ValueType & value)
            : held(value) {
        }
        virtual const std::type_info & type() const {
            return typeid(ValueType);
        }
        virtual placeholder * clone() const {
            return new holder(held);
        }
        ValueType held;
        holder & operator=(const holder &);
    };
    placeholder * content;
};


struct TypeInfo {
    virtual size_t id() const = 0;
    virtual const std::type_info & type() const = 0;
    virtual std::list<size_t> & convertible_from() = 0;
    virtual void register_convertible_from(size_t) = 0;

    static std::map<size_t, std::shared_ptr<TypeInfo> > &typeMap() {
        static std::map<size_t, std::shared_ptr<TypeInfo> > typemap;
        return typemap;
    }
};


template <class T>
struct TypeSpecifier : TypeInfo {
    struct Info {
        static inline size_t id() {
            return type().hash_code();
        }
        static inline const std::type_info & type() {
            return typeid(T*);
        }
    };
private:
    std::list<size_t> convertible_type_ids;
public:
    virtual size_t id() const {
        return Info::id();
    }
    virtual const std::type_info & type() const {
        return Info::type();
    }

    virtual std::list<size_t> & convertible_from() {
        return convertible_type_ids;
    }
    
    virtual void register_convertible_from(size_t c) {
        convertible_type_ids.push_back(c);
    }
};


template <class T>
static inline T *any_cast_ptr(const any &v)
{
    typedef typename remove_reference_and_const<T>::type TargetType;
    std::list<size_t> parents;
std::cout << "searching for type <"<<TypeSpecifier<TargetType>::Info::type().name()<<"> .. ";
    parents.push_back(TypeSpecifier<TargetType>::Info::id());
std::cout << "looking at "<<  v.type().name() << " .. ";
    while (!parents.empty()) {
        size_t p = parents.front();
        auto it = TypeInfo::typeMap().find(p);
        if (it != TypeInfo::typeMap().end()) {
            auto ti = it->second;
std::cout << "\n\ttesting "<<p<< " : " <<ti->type().name()<<" .. ";
            if (ti->type() == v.type()) {
std::cout << "found\n";
                return static_cast<any::holder<T*> *>(v.content)->held;
            }
            std::copy(
                ti->convertible_from().begin(), ti->convertible_from().end(),
                std::back_insert_iterator<std::list<size_t> >(parents)
            );
        } else {
std::cout << "\nWARNING: unregistered type "<<p<<"\n";
        }
        parents.pop_front();
    }
std::cout << "miss!\n";
    return nullptr;
};

template <class T>
static inline std::shared_ptr<TypeInfo> get_typeinfo() {
    size_t id = TypeSpecifier<T>::Info::id();
    auto class_it = TypeInfo::typeMap().find(id);
    if (class_it == TypeInfo::typeMap().end()) {
std::cout << "Adding new type " << id << " " <<TypeSpecifier<T>::Info::type().name()<<std::endl;
        return ((TypeInfo::typeMap().insert(std::make_pair(id, std::shared_ptr<TypeInfo>(new TypeSpecifier<T>())))).first)->second;
    }
    return class_it->second;
};

template <class From, class To>
static inline void register_convertible_type()
{
std::cout << "Registering "<<typeid(From).name()<<" < "<<typeid(To).name()<<std::endl;
    get_typeinfo<To>()
        ->register_convertible_from(
            get_typeinfo<From>()->id()
        );
};

class Instance {
    static std::map<void *, any> &instanceMap() {
        static std::map<void *, any> instance;
        return instance;
    }
public:
    template <typename T>
    static inline void set(T * ptr) {
        instanceMap().insert(std::make_pair(ptr, ptr));
        std::cout << "registering variable at "<<ptr<<" type: "<<typeid(T).name()<<" = "<<instanceMap()[ptr].type().name()<<std::endl;
    }
    
    template <typename T>
    static inline T * get(void * ptr) {
        auto it = instanceMap().find(ptr);
        if (instanceMap().end() == it) {
            std::cerr << "failed to match value for type (no instance) "<<ptr<<" "<<typeid(T*).name()<<std::endl;
            for (auto iterator : instanceMap()) {
                std::cerr << "\t key: " << iterator.first << std::endl;
            }
            return nullptr;
        }
        std::cerr <<"DBG: "<<it->second.type().name()<<std::endl;
        return any_cast_ptr<T>(it->second); 
    }

    static inline void del(void * ptr) {
        instanceMap().erase(ptr);
    }
};

} }

