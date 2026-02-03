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
    .global MLD_ASM_NAMESPACE(poly_chknorm_asm)
MLD_ASM_FN_SYMBOL(poly_chknorm_asm)

        .cfi_startproc
        dup     v20.4s, w1              // Broadcast bound B to all lanes
        movi    v21.4s, #0x0            // Initialize result accumulator to 0
        mov     x2, #0x10               // Loop counter: 16 iterations

Lpoly_chknorm_loop:
        // Load 16 coefficients (4 vectors × 4 lanes = 16 int32_t)
        ldr     q1, [x0, #0x10]
        ldr     q2, [x0, #0x20]
        ldr     q3, [x0, #0x30]
        ldr     q0, [x0], #0x40         // Load and increment pointer

        // For each coefficient: check if |coeff| >= B
        abs     v1.4s, v1.4s            // Absolute value
        cmge    v1.4s, v1.4s, v20.4s    // Compare: |coeff| >= B
        orr     v21.16b, v21.16b, v1.16b // OR into result accumulator

        abs     v2.4s, v2.4s
        cmge    v2.4s, v2.4s, v20.4s
        orr     v21.16b, v21.16b, v2.16b

        abs     v3.4s, v3.4s
        cmge    v3.4s, v3.4s, v20.4s
        orr     v21.16b, v21.16b, v3.16b

        abs     v0.4s, v0.4s
        cmge    v0.4s, v0.4s, v20.4s
        orr     v21.16b, v21.16b, v0.16b

        subs    x2, x2, #0x1            // Decrement loop counter
        b.ne    Lpoly_chknorm_loop      // Branch if not zero

        // Reduce vector to single value: if any lane is non-zero, return 1
        umaxv   s21, v21.4s             // Get maximum across all lanes
        fmov    w0, s21                 // Move to general-purpose register
        and     w0, w0, #0x1            // Extract only bit 0 (0 or 1)
        ret
        .cfi_endproc

/****************** REGISTER DEALLOCATIONS *******************/
    .unreq x1
    .unreq x2

#endif /* MLD_ARITH_BACKEND_AARCH64 && !MLD_CONFIG_MULTILEVEL_NO_SHARED */
