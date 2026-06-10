#ifndef DYNARR
#define DYNARR

#include <stdlib.h>

#define DYN_STRUCT(TYPE, NAME)                                                 \
    typedef struct dyn_##NAME                                                  \
    {                                                                          \
        size_t len;                                                            \
        size_t capacity;                                                       \
        TYPE *arr;                                                             \
    } dyn_##NAME;                                                              \
                                                                               \
    static inline dyn_##NAME *dyn_##NAME##_init()                              \
    {                                                                          \
        dyn_##NAME *obj = calloc(1, sizeof(dyn_##NAME));                       \
        obj->len = 0;                                                          \
        obj->capacity = 1;                                                     \
        obj->arr = calloc(obj->len, sizeof(TYPE));                             \
        return obj;                                                            \
    }                                                                          \
    static inline void dyn_##NAME##_push(dyn_##NAME *obj, TYPE val)            \
    {                                                                          \
        if (obj->len >= obj->capacity)                                         \
        {                                                                      \
            obj->capacity = obj->capacity << 1;                                \
            obj->arr = reallocarray(obj->arr, obj->capacity, sizeof(TYPE));    \
        }                                                                      \
        obj->arr[obj->len++] = val;                                            \
    }                                                                          \
                                                                               \
    static inline void dyn_##NAME##_pop(dyn_##NAME *obj)                       \
    {                                                                          \
        if (obj->len == 0)                                                     \
        {                                                                      \
            return;                                                            \
        }                                                                      \
        obj->len -= 1;                                                         \
        if (obj->len < (obj->capacity >> 1) && obj->capacity > 1)              \
        {                                                                      \
            obj->capacity = obj->capacity >> 1;                                \
            obj->arr = reallocarray(obj->arr, obj->capacity, sizeof(TYPE));    \
        }                                                                      \
    }

#endif
