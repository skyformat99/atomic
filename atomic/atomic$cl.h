﻿#if defined(_MSC_VER) && !defined(_H_ATOMIC$CL)
#define _H_ATOMIC$CL

#include <windows.h>

typedef ptrdiff_t ssize_t;

#define ATOMIC_INIT(...) {__VA_ARGS__}

typedef enum {
    atomic_memory_order_relaxed,
    atomic_memory_order_acquire,
    atomic_memory_order_release,
    atomic_memory_order_acq_rel,
    atomic_memory_order_seq_cst,
} atomic_memory_order_t;

typedef char    atomic_repr_0_t;
typedef short   atomic_repr_1_t;
typedef long    atomic_repr_2_t;
typedef __int64 atomic_repr_3_t;

static inline void atomic_fence(atomic_memory_order_t mo) {
    _ReadWriteBarrier();
#   if defined(_M_ARM) || defined(_M_ARM64)
    /* ARM needs a barrier for everything but relaxed. */
    if (mo != atomic_memory_order_relaxed) {
        MemoryBarrier();
    }
#   elif defined(_M_IX86) || defined(_M_X64)
    /* x86 needs a barrier only for seq_cst. */
    if (mo == atomic_memory_order_seq_cst) {
        MemoryBarrier();
    }
#   else
#   error "Don't know how to create atomics for this platform for CL."
#   endif
    _ReadWriteBarrier();
}

#define ATOMIC_INTERLOCKED_REPR(lg_size) atomic_repr_ ## lg_size ## _t

#define ATOMIC_CONCAT(a, b)     ATOMIC_RAW_CONCAT(a, b)
#define ATOMIC_RAW_CONCAT(a, b) a ## b

#define ATOMIC_INTERLOCKED_NAME(base_name, lg_size) \
ATOMIC_CONCAT(base_name, ATOMIC_INTERLOCKED_SUFFIX(lg_size))

#define ATOMIC_INTERLOCKED_SUFFIX(lg_size) \
ATOMIC_CONCAT(ATOMIC_INTERLOCKED_SUFFIX_, lg_size)

#define ATOMIC_INTERLOCKED_SUFFIX_0 8
#define ATOMIC_INTERLOCKED_SUFFIX_1 16
#define ATOMIC_INTERLOCKED_SUFFIX_2 
#define ATOMIC_INTERLOCKED_SUFFIX_3 64

#define GENERATE_ATOMICS(type, short_type, lg_size)                     \
typedef struct {                                                        \
	ATOMIC_INTERLOCKED_REPR(lg_size) repr;                              \
} atomic_##short_type##_t;                                              \
                                                                        \
static inline type atomic_load_##short_type(                            \
    const atomic_##short_type##_t * a, atomic_memory_order_t mo) {      \
    ATOMIC_INTERLOCKED_REPR(lg_size) ret = a->repr;                     \
    if (mo != atomic_memory_order_relaxed) {                            \
        atomic_fence(atomic_memory_order_acquire);                      \
    }                                                                   \
    return (type)ret;                                                   \
}                                                                       \
                                                                        \
static inline void atomic_store_##short_type(                           \
    atomic_##short_type##_t * a, type val, atomic_memory_order_t mo) {  \
    if (mo != atomic_memory_order_relaxed) {                            \
        atomic_fence(atomic_memory_order_release);                      \
    }                                                                   \
    a->repr = (ATOMIC_INTERLOCKED_REPR(lg_size)) val;                   \
    if (mo == atomic_memory_order_seq_cst) {                            \
        atomic_fence(atomic_memory_order_seq_cst);                      \
    }                                                                   \
}                                                                       \
                                                                        \
static inline type atomic_exchange_##short_type(                        \
    atomic_##short_type##_t * a, type val, atomic_memory_order_t mo) {  \
    return (type)ATOMIC_INTERLOCKED_NAME(_InterlockedExchange,          \
        lg_size)(&a->repr, (ATOMIC_INTERLOCKED_REPR(lg_size))val);      \
}                                                                       \
                                                                        \
static inline _Bool atomic_compare_exchange_weak_##short_type(          \
    atomic_##short_type##_t * a,                                        \
    type * expected, type desired, atomic_memory_order_t success_mo,    \
    atomic_memory_order_t failure_mo) {                                 \
    ATOMIC_INTERLOCKED_REPR(lg_size) e =                                \
        (ATOMIC_INTERLOCKED_REPR(lg_size))*expected;                    \
    ATOMIC_INTERLOCKED_REPR(lg_size) d =                                \
        (ATOMIC_INTERLOCKED_REPR(lg_size))desired;                      \
    ATOMIC_INTERLOCKED_REPR(lg_size) old =                              \
        ATOMIC_INTERLOCKED_NAME(_InterlockedCompareExchange,            \
        lg_size)(&a->repr, d, e);                                       \
    if (old != e) {                                                     \
        *expected = (type)old;                                          \
    }                                                                   \
    return old == e;                                                    \
}                                                                       \
                                                                        \
static inline _Bool atomic_compare_exchange_strong_##short_type(        \
    atomic_##short_type##_t * a,                                        \
    type * expected, type desired, atomic_memory_order_t success_mo,    \
    atomic_memory_order_t failure_mo) {                                 \
    /* We implement the weak version with strong semantics. */          \
	return atomic_compare_exchange_weak_##short_type(a, expected,       \
	    desired, success_mo, failure_mo);                               \
}


#define GENERATE_INT_ATOMICS(type, short_type, lg_size)                 \
GENERATE_ATOMICS(type, short_type, lg_size)                             \
                                                                        \
static inline type atomic_fetch_add_##short_type(                       \
    atomic_##short_type##_t * a, type val, atomic_memory_order_t mo) {  \
	return (type)ATOMIC_INTERLOCKED_NAME(_InterlockedExchangeAdd,       \
	    lg_size)(&a->repr, (ATOMIC_INTERLOCKED_REPR(lg_size))val);      \
}                                                                       \
                                                                        \
static inline type atomic_fetch_sub_##short_type(                       \
    atomic_##short_type##_t * a, type val, atomic_memory_order_t mo) {  \
    /*                                                                  \
     * CL warns on negation of unsigned operands, but for us it         \
     * gives exactly the right semantics (MAX_TYPE + 1 - operand).      \
     */                                                                 \
    __pragma(warning(push))                                             \
    __pragma(warning(disable: 4146))                                    \
    return atomic_fetch_add_##short_type(a, -val, mo);                  \
    __pragma(warning(pop))                                              \
}                                                                       \
                                                                        \
static inline type atomic_fetch_and_##short_type(                       \
    atomic_##short_type##_t * a, type val, atomic_memory_order_t mo) {  \
    return (type)ATOMIC_INTERLOCKED_NAME(_InterlockedAnd, lg_size)(     \
        &a->repr, (ATOMIC_INTERLOCKED_REPR(lg_size))val);               \
}                                                                       \
                                                                        \
static inline type atomic_fetch_or_##short_type(                        \
    atomic_##short_type##_t * a, type val, atomic_memory_order_t mo) {  \
    return (type)ATOMIC_INTERLOCKED_NAME(_InterlockedOr, lg_size)(      \
        &a->repr, (ATOMIC_INTERLOCKED_REPR(lg_size))val);               \
}                                                                       \
                                                                        \
static inline type atomic_fetch_xor_##short_type(                       \
    atomic_##short_type##_t *a, type val, atomic_memory_order_t mo) {   \
    return (type)ATOMIC_INTERLOCKED_NAME(_InterlockedXor, lg_size)(     \
        &a->repr, (ATOMIC_INTERLOCKED_REPR(lg_size))val);               \
}

#endif//_H_ATOMIC$CL