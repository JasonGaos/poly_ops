/*
 * Copyright (c) The mldsa-native project authors
 * SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT
 */

#include "common.h"
#if defined(MLD_ARITH_BACKEND_AARCH64) && \
    !defined(MLD_CONFIG_MULTILEVEL_NO_SHARED)

#if defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif

    .text
    .balign 4
    .global MLD_ASM_NAMESPACE(poly_caddq_asm)
MLD_ASM_FN_SYMBOL(poly_caddq_asm)

        .cfi_startproc
        mov     w9, #0xe001             // =57345
        movk    w9, #0x7f, lsl #16      // w9 = 0x7fe001 = 8380417 (MLDSA_Q)
        dup     v4.4s, w9               // Broadcast Q to all 4 lanes
        mov     x1, #0x10               // Loop counter: 16 iterations

Lpoly_caddq_loop:
        // Load 16 coefficients (4 vectors × 4 lanes = 16 int32_t)
        ldr     q0, [x0]
        ldr     q1, [x0, #0x10]
        ldr     q2, [x0, #0x20]
        ldr     q3, [x0, #0x30]

        // For each coefficient: check if negative (sign bit = 1)
        // If negative, add MLDSA_Q
        ushr    v5.4s, v0.4s, #31       // Extract sign bit (0 if >=0, 1 if <0)
        mla     v0.4s, v5.4s, v4.4s     // v0 += v5 * Q (adds Q only if negative)

        ushr    v5.4s, v1.4s, #31
        mla     v1.4s, v5.4s, v4.4s

        ushr    v5.4s, v2.4s, #31
        mla     v2.4s, v5.4s, v4.4s

        ushr    v5.4s, v3.4s, #31
        mla     v3.4s, v5.4s, v4.4s

        // Store results back
        str     q1, [x0, #0x10]
        str     q2, [x0, #0x20]
        str     q3, [x0, #0x30]
        str     q0, [x0], #0x40         // Store and increment pointer

        subs    x1, x1, #0x1            // Decrement loop counter
        b.ne    Lpoly_caddq_loop        // Branch if not zero

        ret
        .cfi_endproc

/****************** REGISTER DEALLOCATIONS *******************/
    .unreq x1

#endif /* MLD_ARITH_BACKEND_AARCH64 && !MLD_CONFIG_MULTILEVEL_NO_SHARED */
