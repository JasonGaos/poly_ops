/*
 * SVE implementation of poly_chknorm
 *
 * 4-vector unrolled version: processes 4 SVE vectors (32 coefficients) per iteration.
 * Assumes VL = 256 bits (8 int32_t per vector).
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
    // x0 = pointer to polynomial coefficients (int32_t *a)
    // w1 = bound B
    mov     z2.s, w1                // Broadcast bound B to all lanes
    mov     x2, #0                  // Index counter
    mov     x3, #256                // Total coefficients
    mov     x4, #0                  // Violation flag

poly_chknorm_loop_sve:
    // Check if we have at least 32 coefficients remaining
    add     x5, x2, #32
    cmp     x5, x3
    b.gt    poly_chknorm_tail       // Handle remaining if less than 32 coeffs

    // Set full predicate for main loop (8 active lanes per vector)
    ptrue   p0.s

    // Load 4 SVE vectors using indexed addressing
    // Each load gets 8 coefficients (VL=8 for 256-bit SVE with 32-bit elements)
    ld1w    {z0.s}, p0/z, [x0, x2, lsl #2]      // coeffs x2 to x2+7
    add     x5, x2, #8
    ld1w    {z3.s}, p0/z, [x0, x5, lsl #2]      // coeffs x2+8 to x2+15
    add     x5, x2, #16
    ld1w    {z4.s}, p0/z, [x0, x5, lsl #2]      // coeffs x2+16 to x2+23
    add     x5, x2, #24
    ld1w    {z5.s}, p0/z, [x0, x5, lsl #2]      // coeffs x2+24 to x2+31

    // Absolute value
    abs     z0.s, p0/m, z0.s
    abs     z3.s, p0/m, z3.s
    abs     z4.s, p0/m, z4.s
    abs     z5.s, p0/m, z5.s

    // Compare with bound and check immediately
    cmpge   p1.s, p0/z, z0.s, z2.s
    b.any   poly_chknorm_found      // Branch if any lane matches
    cmpge   p1.s, p0/z, z3.s, z2.s
    b.any   poly_chknorm_found
    cmpge   p1.s, p0/z, z4.s, z2.s
    b.any   poly_chknorm_found
    cmpge   p1.s, p0/z, z5.s, z2.s
    b.any   poly_chknorm_found

    add     x2, x2, #32             // Advance index by 32 coefficients
    b       poly_chknorm_loop_sve

poly_chknorm_tail:
    // Handle remaining elements
    cmp     x2, x3
    b.ge    poly_chknorm_done

    whilelt p0.s, x2, x3
    ld1w    {z0.s}, p0/z, [x0, x2, lsl #2]
    abs     z0.s, p0/m, z0.s
    cmpge   p1.s, p0/z, z0.s, z2.s
    incp    x4, p1.s
    cbnz    x4, poly_chknorm_done

    incw    x2
    b       poly_chknorm_loop_sve

poly_chknorm_done:
    cmp     x4, #0
    cset    w0, ne
    ret

poly_chknorm_found:
    mov     w0, #1                  // Return 1 (violation found)
    ret

#endif /* MLD_ARITH_BACKEND_AARCH64 && !MLD_CONFIG_MULTILEVEL_NO_SHARED */
