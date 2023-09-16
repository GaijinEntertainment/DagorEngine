#ifndef PS_STDATOMIC_H_
#define PS_STDATOMIC_H_

#if !defined(__cplusplus)

typedef enum {
  memory_order_relaxed,
  memory_order_consume,
  memory_order_acquire,
  memory_order_release,
  memory_order_acq_rel,
  memory_order_seq_cst
} ps_atomic_memory_order;

typedef int atomic_int;
typedef unsigned int atomic_uint;

#define atomic_init(p_a, v)           do { *(p_a) = (v); } while(0)
#define atomic_store(p_a, v)          __atomic_store_n(p_a, v, memory_order_seq_cst)
#define atomic_load(p_a)              __atomic_load_n(p_a, memory_order_seq_cst)
#define atomic_load_explicit(p_a, mo) __atomic_load_n(p_a, mo)
#define atomic_fetch_add(p_a, inc)    __atomic_fetch_add(p_a, inc, memory_order_seq_cst)
#define atomic_fetch_add_explicit(p_a, inc, mo) __atomic_fetch_add(p_a, inc, mo)
#define atomic_fetch_sub(p_a, dec)    __atomic_fetch_sub(p_a, dec, memory_order_seq_cst)
#define atomic_exchange(p_a, v)       __atomic_exchange_n(p_a, v, memory_order_seq_cst)
#define atomic_fetch_or(p_a, v)       __atomic_fetch_or(p_a, v, memory_order_seq_cst)
#define atomic_compare_exchange_strong(p_a, expected, desired) __atomic_compare_exchange_n(p_a, expected, desired, 0, memory_order_seq_cst, memory_order_seq_cst)

#endif /* !defined(__cplusplus) */

#endif /* PS_STDATOMIC_H_ */
