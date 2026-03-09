/*
 * SVE implementation - testing instructions one by one
 *
 * This file is a skeleton for SVE implementation of poly_caddq.
 * The NEON implementation is in poly_caddq_asm.S
 */

#include "common.h"
#if defined(MLD_ARITH_BACKEND_AARCH64) && \
    !defined(MLD_CONFIG_MULTILEVEL_NO_SHARED)

#if defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif

    .text
    .global MLD_ASM_NAMESPACE(poly_caddq_asm_sve)
    .balign 4
MLD_ASM_FN_SYMBOL(poly_caddq_asm_sve)
    // x0 = pointer to polynomial coefficients (int32_t *a)
    mov     w9, #0xe001
    movk    w9, #0x7f, lsl #16
    mov     z4.s, w9

    mov     x2, #0
    mov     x3, #256

poly_caddq_loop_sve:
    whilelt p0.s, x2, x3
    b.none poly_caddq_done

    ld1w    {z0.s}, p0/z, [x0,x2,lsl #2]
    lsr     z5.s, z0.s, #31
    mla     z0.s, p0/m, z5.s, z4.S
    st1w    {z0.s}, p0, [x0, x2, lsl #2]

    incw    x2
    b       poly_caddq_loop_sve  

poly_caddq_done:
    ret

#endif /* MLD_ARITH_BACKEND_AARCH64 && !MLD_CONFIG_MULTILEVEL_NO_SHARED */
