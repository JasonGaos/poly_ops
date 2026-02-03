#ifndef COMMON_H
#define COMMON_H

/* ML-DSA parameters needed by assembly */
#define MLDSA_N 256
#define MLDSA_Q 8380417

/* Enable assembly backend */
#define MLD_ARITH_BACKEND_AARCH64

/* Alignment for data structures */
#define MLD_ALIGN __attribute__((aligned(32)))

/* Empty compilation unit macro */
#define MLD_EMPTY_CU(s) extern int empty_cu_##s;

/* Namespace macros - two-level expansion for proper recursion */
#define MLD_CONCAT_(x1, x2) x1##x2
#define MLD_CONCAT(x1, x2) MLD_CONCAT_(x1, x2)
#define MLD_CONFIG_NAMESPACE_PREFIX mldsa
#define MLD_NAMESPACE_PREFIX MLD_CONCAT(MLD_CONFIG_NAMESPACE_PREFIX, _)
#define MLD_NAMESPACE(s) MLD_CONCAT(MLD_NAMESPACE_PREFIX, s)

#if defined(__APPLE__)
#define MLD_ASM_NAMESPACE(sym) MLD_CONCAT(_, MLD_NAMESPACE(sym))
#else
#define MLD_ASM_NAMESPACE(sym) MLD_NAMESPACE(sym)
#endif

#define MLD_ASM_FN_SYMBOL(sym) MLD_ASM_NAMESPACE(sym) :

#endif /* COMMON_H */
