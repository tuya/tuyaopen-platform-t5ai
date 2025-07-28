#pragma once
#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus


#define CONCAT_2(p1, p2)        CONCAT_2_(p1, p2)
#define CONCAT_2_(p1, p2)       p1##p2
#define STRINGIFY_(val)         #val
#define STRINGIFY(val)          STRINGIFY_(val)


#if defined(__CC_ARM)
#define BK_SECTION_DEF(section_name, data_type)                \
    extern data_type * CONCAT_2(section_name, $$Base);          \
    extern void      * CONCAT_2(section_name, $$Limit)

#elif defined(__GNUC__)
#define BK_SECTION_DEF(section_name, data_type)                \
    extern data_type * CONCAT_2(__start_, section_name);        \
    extern void      * CONCAT_2(__stop_,  section_name)

#elif defined(__ICCARM__)
#define BK_SECTION_DEF(section_name, data_type)                \
    _Pragma(STRINGIFY(section = STRINGIFY(section_name)));
#endif

#if defined(__CC_ARM)
#define AVDK_SECTION_END_ADDR(section_name)         &CONCAT_2(section_name, $$Limit)

#elif defined(__GNUC__)
#define AVDK_SECTION_END_ADDR(section_name)         &CONCAT_2(__stop_, section_name)

#elif defined(__ICCARM__)
#define AVDK_SECTION_END_ADDR(section_name)         __section_end(STRINGIFY(section_name))
#endif


#if defined(__CC_ARM)
#define BK_SECTION_ITEM_REGISTER(section_name, section_var) \
    section_var __attribute__ ((section(STRINGIFY(section_name)))) __attribute__((used))

#elif defined(__GNUC__)
#define BK_SECTION_ITEM_REGISTER(section_name, section_var) \
    section_var __attribute__ ((section("." STRINGIFY(section_name)))) __attribute__((used))

#elif defined(__ICCARM__)
#define BK_SECTION_ITEM_REGISTER(section_name, section_var) \
    __root section_var @ STRINGIFY(section_name)
#endif

#if defined(__CC_ARM)
#define AVDK_SECTION_START_ADDR(section_name)       &CONCAT_2(section_name, $$Base)

#elif defined(__GNUC__)
#define AVDK_SECTION_START_ADDR(section_name)       &CONCAT_2(__start_, section_name)

#elif defined(__ICCARM__)
#define AVDK_SECTION_START_ADDR(section_name)       __section_begin(STRINGIFY(section_name))
#endif

#define AVDK_SECTION_LENGTH(section_name)                        \
    ((size_t)AVDK_SECTION_END_ADDR(section_name) -               \
     (size_t)AVDK_SECTION_START_ADDR(section_name))


#define BK_SECTION_ITEM_GET(section_name, data_type, i) \
    ((data_type*)AVDK_SECTION_START_ADDR(section_name) + (i))

#define BK_SECTION_ITEM_COUNT(section_name, data_type) \
    AVDK_SECTION_LENGTH(section_name) / sizeof(data_type)


#ifdef  __cplusplus
}
#endif//__cplusplus

/**
 * @}
 */


