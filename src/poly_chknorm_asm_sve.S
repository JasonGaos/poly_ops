/*
 * SVE implementation of poly_chknorm
 *
 * Matches NEON behavior: processes all 256 coefficients without early exit.
 * Optimized 4-vector unrolled version using not for predicate-to-vector conversion.
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
    eor     z6.d, z6.d, z6.d        // Result accumulator (all zeros)
    mov     x2, #8                  // Loop counter: 8 iterations
                                    // (4 vectors per iter × 8 lanes = 32 coeffs/iter)
                                    // 8 × 32 = 256 coefficients total
    mov     x3, x0                  // Working pointer

poly_chknorm_loop_sve:
    // Set full predicate for main loop (all lanes active)
    ptrue   p0.s

    // Load 4 SVE vectors using pointer registers (32 coeffs per iteration)
    mov     x4, x3                  // ptr0 = base
    add     x5, x3, #32             // ptr1 = base + 32 bytes
    add     x6, x3, #64             // ptr2 = base + 64 bytes
    add     x9, x3, #96             // ptr3 = base + 96 bytes

    ld1w    {z0.s}, p0/z, [x4]
    ld1w    {z3.s}, p0/z, [x5]
    ld1w    {z4.s}, p0/z, [x6]
    ld1w    {z5.s}, p0/z, [x9]

    add     x3, x3, #128            // Advance pointer by 128 bytes (32 coeffs)

    // For each vector: abs, compare, convert predicate to mask, OR into accumulator
    // Using "not z.b, p/m, z.b" pattern: zero first, then invert active lanes
    // to get 0xFFFFFFFF where condition true, 0 otherwise
    abs     z0.s, p0/m, z0.s
    cmpge   p1.s, p0/z, z0.s, z2.s      // p1 = (|coeff| >= B)
    eor     z0.d, z0.d, z0.d            // Zero z0
    not     z0.b, p1/m, z0.b            // z0 = 0xFFFFFFFF where p1 true, 0 otherwise
    orr     z6.b, p0/m, z6.b, z0.b      // OR into accumulator

    abs     z3.s, p0/m, z3.s
    cmpge   p1.s, p0/z, z3.s, z2.s
    eor     z3.d, z3.d, z3.d            // Zero z3
    not     z3.b, p1/m, z3.b
    orr     z6.b, p0/m, z6.b, z3.b

    abs     z4.s, p0/m, z4.s
    cmpge   p1.s, p0/z, z4.s, z2.s
    eor     z4.d, z4.d, z4.d            // Zero z4
    not     z4.b, p1/m, z4.b
    orr     z6.b, p0/m, z6.b, z4.b

    abs     z5.s, p0/m, z5.s
    cmpge   p1.s, p0/z, z5.s, z2.s
    eor     z5.d, z5.d, z5.d            // Zero z5
    not     z5.b, p1/m, z5.b
    orr     z6.b, p0/m, z6.b, z5.b

    subs    x2, x2, #0x1            // Decrement loop counter
    b.ne    poly_chknorm_loop_sve   // Branch if not zero

    // Reduce vector to single value: if any lane is non-zero, return 1
    umaxv   s6, p0, z6.s            // Get maximum across all lanes
    fmov    w0, s6                  // Move to general-purpose register
    and     w0, w0, #0x1            // Extract only bit 0 (0 or 1)
    ret

#endif /* MLD_ARITH_BACKEND_AARCH64 && !MLD_CONFIG_MULTILEVEL_NO_SHARED */
