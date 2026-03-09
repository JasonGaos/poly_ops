/*
 * SVE implementation - testing instructions one by one
 *
 * This file is a skeleton for SVE implementation of poly_chknorm.
 * The NEON implementation is in poly_chknorm_asm.S
 */

#include "common.h"
#if defined(MLD_ARITH_BACKEND_AARCH64) && \
    !defined(MLD_CONFIG_MULTILEVEL_NO_SHARED)

#if defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif

    .text
    .global MLD_ASM_NAMESPACE(poly_chknorm_asm_sve)
    .balign 4
MLD_ASM_FN_SYMBOL(poly_chknorm_asm_sve)
    mov     z2.s, w1
    mov     z1.b, #0
    mov     x2, #0
    mov     x3, #256
    mov     x4, #0

poly_chknorm_loop_sve:
    whilelt p0.s, x2, x3            /* p0 = (x2 < 256) 的谓词 */
    b.none  poly_chknorm_done         /* 如果谓词全空，结束 */

    // load data to z0
    ld1w    {z0.s}, p0/z, [x0, x2, lsl #2]

    // abs
    abs     z0.s, p0/m, z0.s

    // compare with beta
    cmpge   p1.s, p0/z, z0.s, z2.s

    // count violation (return if found)
    incp    x4, p1.s
    cbnz    x4, poly_chknorm_done

    incw    x2
    b       poly_chknorm_loop_sve
poly_chknorm_done:
    cmp     x4,#0
    cset    w0,ne
    ret

#endif /* MLD_ARITH_BACKEND_AARCH64 && !MLD_CONFIG_MULTILEVEL_NO_SHARED */
