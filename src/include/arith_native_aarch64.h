/*
 * Function declarations for assembly implementations
 *
 * Derived from mldsa-native project
 * SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT
 */

#ifndef ARITH_NATIVE_AARCH64_H
#define ARITH_NATIVE_AARCH64_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * poly_caddq_asm - Conditional addition of MLDSA_Q (NEON version)
 *
 * For each coefficient a[i]:
 *   - If a[i] < 0, set a[i] = a[i] + MLDSA_Q
 *   - If a[i] >= 0, leave a[i] unchanged
 *
 * Parameters:
 *   a: Pointer to array of 256 int32_t coefficients (modified in-place)
 *
 * The coefficients must be in range [-MLDSA_Q+1, MLDSA_Q-1] before calling.
 * After calling, all coefficients will be in range [0, MLDSA_Q-1].
 */
void mldsa_poly_caddq_asm(int32_t *a);

/*
 * poly_caddq_asm_sve - Conditional addition of MLDSA_Q (SVE version)
 *
 * Same functionality as poly_caddq_asm, but uses SVE instructions.
 * Function signature matches for drop-in replacement.
 */
void mldsa_poly_caddq_asm_sve(int32_t *a);

/*
 * poly_chknorm_asm - Check if all coefficients are within bound B (NEON version)
 *
 * Parameters:
 *   a: Pointer to array of 256 int32_t coefficients (read-only)
 *   B: Bound to check against
 *
 * Returns:
 *   0 if |a[i]| < B for all i = 0..255
 *   1 if |a[i]| >= B for any i
 */
int mldsa_poly_chknorm_asm(const int32_t *a, int32_t B);

/*
 * poly_chknorm_asm_sve - Check if all coefficients are within bound B (SVE version)
 *
 * Same functionality as poly_chknorm_asm, but uses SVE instructions.
 * Function signature matches for drop-in replacement.
 */
int mldsa_poly_chknorm_asm_sve(const int32_t *a, int32_t B);

#ifdef __cplusplus
}
#endif

#endif /* ARITH_NATIVE_AARCH64_H */
