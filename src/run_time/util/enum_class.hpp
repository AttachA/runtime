#ifndef SRC_RUN_TIME_UTIL_ENUM_CLASS
#define SRC_RUN_TIME_UTIL_ENUM_CLASS
#include <boost/preprocessor.hpp>
#include <string>
#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(r, data, elem)    \
    case data::elem : return BOOST_PP_STRINGIZE(elem);

#define basic_enum_class(name, enumerators, aliases)\
class name{\
    public:\
    enum _##name {\
        BOOST_PP_SEQ_ENUM(enumerators)\
        BOOST_PP_SEQ_ENUM(aliases)\
    };\
    std::string to_string()\
    {\
        switch (v)\
        {\
            BOOST_PP_SEQ_FOR_EACH(\
                X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,\
                name,\
                enumerators\
            )\
            default: return '[' + std::to_string((size_t)v) + " invalid " BOOST_PP_STRINGIZE(name) "]";\
        }\
    }\
    name(_##name v):v(v){}\
    name(const name&)=default;\
    name& operator=(const name&)=default;\
    name(name&&)=default;\
    name& operator=(name&&)=default;\
    bool operator==(const name& other)const{return v==other.v;}\
    bool operator!=(const name& other)const{return v!=other.v;}\
private:\
    _##name v;\
};

#define enum_class(name, enumerators, aliases)\
namespace __internal{\
    class name{\
        public:\
        enum _##name {\
            BOOST_PP_SEQ_ENUM(enumerators)\
            BOOST_PP_SEQ_ENUM(aliases)\
        };\
        std::string to_string()\
        {\
            switch (value)\
            {\
                BOOST_PP_SEQ_FOR_EACH(\
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,\
                    _##name,\
                    enumerators\
                )\
                default: return '[' + std::to_string((size_t)value) + " invalid " BOOST_PP_STRINGIZE(name) "]";\
            }\
        }\
        name(_##name v):value(v){}\
        name(const name&)=default;\
        name& operator=(const name&)=default;\
        name(name&&)=default;\
        name& operator=(name&&)=default;\
        bool operator==(const name& other)const{return value==other.value;}\
        bool operator!=(const name& other)const{return value!=other.value;}\
    protected:\
        _##name value;\
    };\
}\
class name:public __internal::name


#define enum_class_t(name, type, enumerators, aliases)\
namespace __internal{\
    class name{\
        public:\
        enum _##name : type{\
            BOOST_PP_SEQ_ENUM(enumerators)\
            BOOST_PP_SEQ_ENUM(aliases)\
        };\
        std::string to_string()\
        {\
            switch (value)\
            {\
                BOOST_PP_SEQ_FOR_EACH(\
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,\
                    _##name,\
                    enumerators\
                )\
                default: return '[' + std::to_string((size_t)value) + " invalid " BOOST_PP_STRINGIZE(name) "]";\
            }\
        }\
        name(_##name v):value(v){}\
        name(const name&)=default;\
        name& operator=(const name&)=default;\
        name(name&&)=default;\
        name& operator=(name&&)=default;\
        bool operator==(const name& other)const{return value==other.value;}\
        bool operator!=(const name& other)const{return value!=other.value;}\
    protected:\
        _##name value;\
    };\
}\
class name:public __internal::name

#endif /* SRC_RUN_TIME_UTIL_ENUM_CLASS */
