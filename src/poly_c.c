/*
 * Reference C implementations for poly_caddq and poly_chknorm
 * These are used for correctness verification of assembly implementations.
 *
 * Derived from mldsa-native project
 * SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT
 */

#include <stdint.h>
#include <stdlib.h>

#define MLDSA_N 256
#define MLDSA_Q 8380417

/*
 * poly_caddq_c - Conditional addition of MLDSA_Q
 *
 * For each coefficient a[i]:
 *   - If a[i] < 0, set a[i] = a[i] + MLDSA_Q
 *   - If a[i] >= 0, leave a[i] unchanged
 *
 * This ensures all coefficients are in the range [0, MLDSA_Q-1]
 * assuming they were originally in range [-MLDSA_Q+1, MLDSA_Q-1]
 */
void poly_caddq_c(int32_t *a) {
    for (int i = 0; i < MLDSA_N; i++) {
        if (a[i] < 0) {
            a[i] += MLDSA_Q;
        }
    }
}

/*
 * Constant-time version of poly_caddq_c
 * Avoids branching for better side-channel resistance
 */
void poly_caddq_c_ct(int32_t *a) {
    for (int i = 0; i < MLDSA_N; i++) {
        // Extract sign bit: 0xFFFFFFFF if negative, 0 if non-negative
        uint32_t sign = (a[i] >> 31) & 1;
        uint32_t mask = 0 - sign;
        // Add MLDSA_Q masked by sign bit
        a[i] += MLDSA_Q & mask;
    }
}

/*
 * poly_chknorm_c - Check if all coefficients are within bound B
 *
 * Returns:
 *   - 0 if |a[i]| < B for all i
 *   - 1 if |a[i]| >= B for any i
 */
int poly_chknorm_c(const int32_t *a, int32_t B) {
    for (int i = 0; i < MLDSA_N; i++) {
        if (a[i] >= B || a[i] < -B) {
            return 1;
        }
    }
    return 0;
}

/*
 * Constant-time version of poly_chknorm_c
 * Avoids early termination for better side-channel resistance
 */
int poly_chknorm_c_ct(const int32_t *a, int32_t B) {
    uint32_t result = 0;

    for (int i = 0; i < MLDSA_N; i++) {
        int32_t abs_a = (a[i] < 0) ? -a[i] : a[i];
        // Set bit 0 if |a[i]| >= B
        result |= (abs_a >= B) ? 1 : 0;
    }

    return (int)result;
}

/*
 * Helper function: compute absolute value of int32_t
 */
static inline int32_t ct_abs_i32(int32_t a) {
    int32_t mask = a >> 31;
    return (a + mask) ^ mask;
}

/*
 * More compact constant-time version of poly_chknorm_c
 */
int poly_chknorm_c_ct2(const int32_t *a, int32_t B) {
    uint32_t t = 0;

    for (int i = 0; i < MLDSA_N; i++) {
        // Compute absolute value using constant-time method
        int32_t abs_a = ct_abs_i32(a[i]);
        // Create mask: 0xFFFFFFFF if |a[i]| >= B, else 0
        t |= ((uint32_t)(B - 1 - abs_a) >> 31);
    }

    return (int)t;
}
