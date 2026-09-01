// Header only stb-style reference counting
/*
 * I don't like licenses, because I don't like having to worry about all this
 * legal stuff just for a simple piece of software I don't really mind anyone
 * using. But I also believe that it's important that people share and give
 * back; so I'm placing this work under the following license.
 *
 *
 * BOLA - Buena Onda License Agreement (v1.1)
 * ------------------------------------------
 *
 * This work is provided 'as-is', without any express or implied warranty. In no
 * event will the authors be held liable for any damages arising from the use of
 * this work.
 *
 * To all effects and purposes, this work is to be considered Public Domain.
 *
 *
 * However, if you want to be "buena onda", you should:
 *
 * 1. Not take credit for it, and give proper recognition to the authors.
 * 2. Share your modifications, so everybody benefits from them.
 * 3. Do something nice for the authors.
 * 4. Help someone who needs it: sign up for some volunteer work or help your
 *    neighbour paint the house.
 * 5. Don't waste. Anything, but specially energy that comes from natural
 *    non-renewable resources. Extra points if you discover or invent something
 *    to replace them.
 * 6. Be tolerant. Everything that's good in nature comes from cooperation.
 */

/* EXAMPLE

 #include <assert.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>

 #define CRC_IMPLEMENTATION
 #include "crc.h"

 typedef struct Person {
         char *name;
         int age, height;
 } Person;
 void person_del(void *data);
 void *person_new(char *name, int age, int height)
 {
         Person *p = with_deleter(sizeof(Person), person_del);
         p->name = strdup(name);
         p->age = age;
         p->height = height;

         return p;
 }
 void person_del(void *data)
 {
         Person *p = (Person *)data;
         free(p->name);
 }

 void person_print(Person *p)
 {
         assert(p->name != NULL);
         use(p);
         printf("p.name = %s, p.age = %d, p.height = %d\n", p->name, p->age,
                p->height);
         del(p);
 }

 int main()
 {
         char *rc_str(greeting, "Hello, Seamen!"); // _Generic
         int *rc(len, sizeof(int));
         *len = strlen(greeting);
         Person *rc$(p) = person_new("Sereníssima", 21, 172);

         printf("%.*s\n", *len, greeting);
         person_print(p);

         printf("Terminating...\n");
         return 0;
 }
 */

#ifndef CRefCount_H
#define CRefCount_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#ifdef CRC_THREAD_SAFE
#include <stdatomic.h>
#endif

#ifndef CRC_malloc
#include <stdlib.h>
#define CRC_malloc(size) malloc(size)
#endif

#ifndef CRC_free
#include <stdlib.h>
#define CRC_free(ptr) free(ptr)
#endif

#ifndef CRC_DECL
#define CRC_DECL
#endif

#if defined(__has_feature) && __has_feature(nullability)
#define CRC__NONNULL _Nonnull
#define CRC__NULLABLE _Nullable
#elif defined(__has_extension) && __has_extension(nullability)
#define CRC__NONNULL _Nonnull
#define CRC__NULLABLE _Nullable
#else
#define CRC__NONNULL
#define CRC__NULLABLE
#endif

#pragma region scoping implementation

#define CRC__N_VA_ARGS_FOREACH_(_9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N
#define CRC__N_VA_ARGS_FOREACH(...)                                          \
        CRC__N_VA_ARGS_FOREACH_(__VA_ARGS__ __VA_OPT__(, ) 9, 8, 7, 6, 5, 4, \
                                3, 2, 1, 0)

#define CRC__FOREACH_MACRO_0(FN, ...)
#define CRC__FOREACH_MACRO_1(FN, E, ...) FN(E)
#define CRC__FOREACH_MACRO_2(FN, E, ...) \
        FN(E) CRC__FOREACH_MACRO_1(FN, __VA_ARGS__)
#define CRC__FOREACH_MACRO_3(FN, E, ...) \
        FN(E) CRC__FOREACH_MACRO_2(FN, __VA_ARGS__)
#define CRC__FOREACH_MACRO_4(FN, E, ...) \
        FN(E) CRC__FOREACH_MACRO_3(FN, __VA_ARGS__)
#define CRC__FOREACH_MACRO_5(FN, E, ...) \
        FN(E) CRC__FOREACH_MACRO_4(FN, __VA_ARGS__)
#define CRC__FOREACH_MACRO_6(FN, E, ...) \
        FN(E) CRC__FOREACH_MACRO_5(FN, __VA_ARGS__)
#define CRC__FOREACH_MACRO_7(FN, E, ...) \
        FN(E) CRC__FOREACH_MACRO_6(FN, __VA_ARGS__)
#define CRC__FOREACH_MACRO_8(FN, E, ...) \
        FN(E) CRC__FOREACH_MACRO_7(FN, __VA_ARGS__)
#define CRC__FOREACH_MACRO_9(FN, E, ...) \
        FN(E) CRC__FOREACH_MACRO_8(FN, __VA_ARGS__)

#define CRC__FOREACH_MACRO__(FN, NARGS, ...) \
        CRC__FOREACH_MACRO_##NARGS(FN, __VA_ARGS__)
#define CRC__FOREACH_MACRO_(FN, NARGS, ...) \
        CRC__FOREACH_MACRO__(FN, NARGS, __VA_ARGS__)
#define CRC__FOREACH_MACRO(FN, ...)                                  \
        CRC__FOREACH_MACRO_(FN, CRC__N_VA_ARGS_FOREACH(__VA_ARGS__), \
                            __VA_ARGS__)

#define X1(v) crc_use(v),
#define X2(v) crc_del(v),

#define CRC__CONCAT_IDS__IMPL(a, b) a##b
#define CRC__CONCAT_IDS_(a, b) CRC__CONCAT_IDS__IMPL(a, b)

#pragma endregion

// internal macroslop
#define CRC_scoping__id(id, ...)                                 \
        for (bool CRC__CONCAT_IDS_(_CRC_scoping_run_, id) =      \
                     (CRC__FOREACH_MACRO(X1, __VA_ARGS__) true); \
             CRC__CONCAT_IDS_(_CRC_scoping_run_, id);            \
             (CRC__FOREACH_MACRO(X2, __VA_ARGS__)                \
                      CRC__CONCAT_IDS_(_CRC_scoping_run_, id) = false))
// supports up to 10 variables, automatically uses and dels for you. leaks if
// you return inside it, so it's unsafe.
// Usage:
// scoping(variable1, variable2...)
// {
//      process(variable1, variable2);
// }
#define CRC_scoping(...) CRC_scoping__id(__COUNTER__, __VA_ARGS__)

// stands for rc assign
#define CRC_rca(name) name __attribute__((cleanup(crc__cleanup)))
#define CRC_rc(name, size) \
        name __attribute__((cleanup(crc__cleanup))) = crc_new(size)
#define CRC_rcd(name, size, deleter)                  \
        name __attribute__((cleanup(crc__cleanup))) = \
                crc_with_deleter(size, deleter)
#define CRC_rc_str(name, str) \
        name __attribute__((cleanup(crc__cleanup))) = crc_from_cstr(str)

typedef void (*CRC__Rc_DeleterFn)(void *CRC__NONNULL data);
struct CRC__Rc {
#ifdef CRC_THREAD_SAFE
        atomic_ptrdiff_t atomic_count;
#else
        ptrdiff_t count;
#endif
        CRC__NULLABLE CRC__Rc_DeleterFn deleter;
};

// Count is inited to 0
CRC_DECL void *CRC__NONNULL crc_new(size_t size);
CRC_DECL void *CRC__NONNULL
crc_with_deleter(size_t size, CRC__NONNULL CRC__Rc_DeleterFn deleter);
CRC_DECL void *CRC__NONNULL crc_use(void *CRC__NONNULL rc_ptr);
CRC_DECL void crc_del(void *CRC__NONNULL rc_ptr);

// API QoL

CRC_DECL char *CRC__NONNULL crc_from_cstr(const char *CRC__NONNULL cstr);

// internal
CRC_DECL void crc__cleanup(void *CRC__NONNULL ptr);

#ifndef CRC_LONG_NAMES
#define scoping(...) CRC_scoping(__VA_ARGS__)
#define rca(name) CRC_rca(name)
#define rc(name, size) CRC_rc(name, size)
#define rcd(name, size, deleter) CRC_rcd(name, size, deleter)
#define rc_str(name, str) CRC_rc_str(name, str)

static inline void *CRC__NONNULL new (size_t size)
{
        return crc_new(size);
}
static inline void *CRC__NONNULL
with_deleter(size_t size, CRC__NONNULL CRC__Rc_DeleterFn deleter)
{
        return crc_with_deleter(size, deleter);
}

static inline void *CRC__NONNULL use(void *CRC__NONNULL rc_ptr)
{
        return crc_use(rc_ptr);
}
static inline void del(void *CRC__NONNULL rc_ptr)
{
        crc_del(rc_ptr);
}
static inline char *CRC__NONNULL from_cstr(const char *CRC__NONNULL cstr)
{
        return crc_from_cstr(cstr);
}
#endif // CRC_LONG_NAMES

#endif // CRefCount_H

#ifdef CRC_IMPLEMENTATION
CRC_DECL void *CRC__NONNULL crc_new(size_t size)
{
        assert(size > 0 && "crc_new size must be greater than 0");

        struct CRC__Rc *rc;
        rc = CRC_malloc(sizeof(struct CRC__Rc) + size);
        assert(rc != NULL && "CRC_malloc failed");

#ifdef CRC_THREAD_SAFE
        atomic_store(&rc->atomic_count, 1);
#else
        rc->count = 1;
#endif
        rc->deleter = NULL;
        void *data = rc + 1;

        return data;
}
CRC_DECL void *CRC__NONNULL
crc_with_deleter(size_t size, CRC__NONNULL CRC__Rc_DeleterFn deleter)
{
        assert(size > 0 && "crc_with_deleter size must be greater than 0");
        assert(deleter != NULL && "crc_with_deleter deleter must not be NULL");

        struct CRC__Rc *rc;
        rc = CRC_malloc(sizeof(struct CRC__Rc) + size);
        assert(rc != NULL && "CRC_malloc failed");
#ifdef CRC_THREAD_SAFE
        atomic_store(&rc->atomic_count, 1);
#else
        rc->count = 1;
#endif
        rc->deleter = deleter;
        void *data = rc + 1;

        return data;
}

CRC_DECL void *CRC__NONNULL crc_use(void *CRC__NONNULL rc_ptr)
{
        assert(rc_ptr != NULL && "crc_use cannot use a NULL pointer");

        struct CRC__Rc *rc = (struct CRC__Rc *)rc_ptr - 1;
#ifdef CRC_THREAD_SAFE
        atomic_fetch_add_explicit(&rc->atomic_count, 1, memory_order_relaxed);
#else
        rc->count += 1;
#endif

        return rc_ptr;
}

CRC_DECL void crc_del(void *CRC__NONNULL rc_ptr)
{
        assert(rc_ptr != NULL && "crc_del cannot delete a NULL pointer");

        struct CRC__Rc *rc = (struct CRC__Rc *)rc_ptr - 1;
        bool can_delete = false;
#ifdef CRC_THREAD_SAFE
        can_delete = atomic_fetch_sub_explicit(&rc->atomic_count, 1,
                                               memory_order_acq_rel) <= 1;
#else
        rc->count -= 1;
        can_delete = rc->count <= 0;
#endif
        if (can_delete) {
                if (rc->deleter != NULL) {
                        rc->deleter(rc_ptr);
                }
                CRC_free(rc);
        }
}

// internal
CRC_DECL void crc__cleanup(void *CRC__NONNULL ptr)
{
        crc_del(*(void **)ptr);
}

CRC_DECL char *CRC__NONNULL crc_from_cstr(const char *CRC__NONNULL cstr)
{
        assert(cstr != NULL &&
               "crc_from_cstr cannot operate on a NULL pointer");

        size_t len = strlen(cstr) + 1;
        char *str = crc_new(len); // + 1 cuz null terminator
        strncpy(str, cstr, len);

        return str;
}
#endif // CRC_IMPLEMENTATION
