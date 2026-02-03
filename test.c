/*
 * Test suite for poly_caddq and poly_chknorm functions
 *
 * Tests both NEON and SVE implementations against C reference
 *
 * SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MLDSA_N 256
#define MLDSA_Q 8380417
#define ITERATIONS 10000

// Timing helper with nanosecond precision
static inline uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

#define TIMER_START() uint64_t start_ns = get_time_ns()
#define TIMER_END(label) do { \
    uint64_t end_ns = get_time_ns(); \
    double elapsed_us = (double)(end_ns - start_ns) / 1000.0; \
    printf("  Time (%s): %.3f us (avg over %d iterations)\n", label, elapsed_us, ITERATIONS); \
} while(0)

/* External assembly functions */
extern void mldsa_poly_caddq_asm(int32_t *a);
extern int mldsa_poly_chknorm_asm(const int32_t *a, int32_t B);

#if defined(HAVE_SVE)
extern void mldsa_poly_caddq_asm_sve(int32_t *a);
extern int mldsa_poly_chknorm_asm_sve(const int32_t *a, int32_t B);

// Use SVE function by default when HAVE_SVE is defined
#define POLY_CADDQ_FN mldsa_poly_caddq_asm_sve
#define POLY_CHKNORM_FN mldsa_poly_chknorm_asm_sve
#define VARIANT_STR "SVE"
#else
// Use NEON function otherwise
#define POLY_CADDQ_FN mldsa_poly_caddq_asm
#define POLY_CHKNORM_FN mldsa_poly_chknorm_asm
#define VARIANT_STR "NEON"
#endif

/* Reference C implementations */
extern void poly_caddq_c_ct(int32_t *a);
extern int poly_chknorm_c_ct(const int32_t *a, int32_t B);

/* ========================================
 * Test Helpers
 * ======================================== */

static void init_poly_random(int32_t *a, int32_t min_val, int32_t max_val) {
    for (int i = 0; i < MLDSA_N; i++) {
        // Generate random value in range [min_val, max_val]
        int32_t range = max_val - min_val + 1;
        a[i] = min_val + (rand() % range);
    }
}

static void init_poly_all_negative(int32_t *a) {
    for (int i = 0; i < MLDSA_N; i++) {
        a[i] = -(rand() % (MLDSA_Q - 1)) - 1;  // [-MLDSA_Q+1, -1]
    }
}

static void init_poly_all_positive(int32_t *a) {
    for (int i = 0; i < MLDSA_N; i++) {
        a[i] = rand() % (MLDSA_Q - 1);  // [0, MLDSA_Q-2]
    }
}

static int compare_polys(const int32_t *a, const int32_t *b, const char *msg) {
    for (int i = 0; i < MLDSA_N; i++) {
        if (a[i] != b[i]) {
            printf("  ERROR: %s - Mismatch at index %d: ASM=%d, C=%d\n",
                   msg, i, a[i], b[i]);
            return 1;
        }
    }
    return 0;
}

/* ========================================
 * poly_caddq Tests
 * ======================================== */

static void test_caddq_all_positive(void) {
    printf("Test 1: poly_caddq with all positive coefficients...\n");

    int32_t input[MLDSA_N];
    int32_t output_asm[MLDSA_N];
    int32_t output_c[MLDSA_N];

    init_poly_all_positive(input);

    memcpy(output_asm, input, sizeof(input));
    memcpy(output_c, input, sizeof(input));

    TIMER_START();
    for (int i = 0; i < ITERATIONS; i++) {
        int32_t tmp[MLDSA_N];
        memcpy(tmp, input, sizeof(input));
        POLY_CADDQ_FN(tmp);
    }
    TIMER_END("ASM");

    POLY_CADDQ_FN(output_asm);
    poly_caddq_c_ct(output_c);

    int errors = compare_polys(output_asm, output_c, "positive coefficients");

    if (errors == 0) {
        printf("  PASSED!\n\n");
    } else {
        printf("  FAILED with %d errors!\n\n", errors);
    }
}

static void test_caddq_all_negative(void) {
    printf("Test 2: poly_caddq with all negative coefficients...\n");

    int32_t input[MLDSA_N];
    int32_t output_asm[MLDSA_N];
    int32_t output_c[MLDSA_N];

    init_poly_all_negative(input);

    memcpy(output_asm, input, sizeof(input));
    memcpy(output_c, input, sizeof(input));

    TIMER_START();
    for (int i = 0; i < ITERATIONS; i++) {
        int32_t tmp[MLDSA_N];
        memcpy(tmp, input, sizeof(input));
        POLY_CADDQ_FN(tmp);
    }
    TIMER_END("ASM");

    POLY_CADDQ_FN(output_asm);
    poly_caddq_c_ct(output_c);

    int errors = compare_polys(output_asm, output_c, "negative coefficients");

    // Verify all coefficients are now non-negative
    for (int i = 0; i < MLDSA_N; i++) {
        if (output_asm[i] < 0 || output_asm[i] >= MLDSA_Q) {
            printf("  ERROR: Coefficient %d out of range: %d\n", i, output_asm[i]);
            errors++;
        }
    }

    if (errors == 0) {
        printf("  PASSED!\n\n");
    } else {
        printf("  FAILED with %d errors!\n\n", errors);
    }
}

static void test_caddq_mixed(void) {
    printf("Test 3: poly_caddq with mixed positive/negative coefficients...\n");

    int32_t input[MLDSA_N];
    int32_t output_asm[MLDSA_N];
    int32_t output_c[MLDSA_N];

    // Mix of positive and negative values
    for (int i = 0; i < MLDSA_N; i++) {
        if (i % 2 == 0) {
            input[i] = -(rand() % (MLDSA_Q / 2)) - 1;
        } else {
            input[i] = rand() % (MLDSA_Q / 2);
        }
    }

    memcpy(output_asm, input, sizeof(input));
    memcpy(output_c, input, sizeof(input));

    TIMER_START();
    for (int i = 0; i < ITERATIONS; i++) {
        int32_t tmp[MLDSA_N];
        memcpy(tmp, input, sizeof(input));
        POLY_CADDQ_FN(tmp);
    }
    TIMER_END("ASM");

    POLY_CADDQ_FN(output_asm);
    poly_caddq_c_ct(output_c);

    int errors = compare_polys(output_asm, output_c, "mixed coefficients");

    if (errors == 0) {
        printf("  PASSED!\n\n");
    } else {
        printf("  FAILED with %d errors!\n\n", errors);
    }
}

static void test_caddq_zeros(void) {
    printf("Test 4: poly_caddq with all zero coefficients...\n");

    int32_t input[MLDSA_N] = {0};
    int32_t output_asm[MLDSA_N];
    int32_t output_c[MLDSA_N];

    memcpy(output_asm, input, sizeof(input));
    memcpy(output_c, input, sizeof(input));

    POLY_CADDQ_FN(output_asm);
    poly_caddq_c_ct(output_c);

    int errors = compare_polys(output_asm, output_c, "zero coefficients");

    // Verify all are still zero
    for (int i = 0; i < MLDSA_N; i++) {
        if (output_asm[i] != 0) {
            printf("  ERROR: Zero became non-zero: output_asm[%d] = %d\n", i, output_asm[i]);
            errors++;
        }
    }

    if (errors == 0) {
        printf("  PASSED!\n\n");
    } else {
        printf("  FAILED with %d errors!\n\n", errors);
    }
}

/* ========================================
 * poly_chknorm Tests
 * ======================================== */

static void test_chknorm_all_within(void) {
    printf("Test 5: poly_chknorm with all coefficients within bound...\n");

    int32_t input[MLDSA_N];
    int32_t B = 1000;

    // All values in range (-B, B)
    for (int i = 0; i < MLDSA_N; i++) {
        input[i] = (rand() % (B - 1)) - (B / 2);
    }

    TIMER_START();
    for (int i = 0; i < ITERATIONS; i++) {
        volatile int result = POLY_CHKNORM_FN(input, B);
        (void)result;
    }
    TIMER_END("ASM");

    int result_asm = POLY_CHKNORM_FN(input, B);
    int result_c = poly_chknorm_c_ct(input, B);

    printf("  ASM: %d, C ref: %d (expected 0)\n", result_asm, result_c);

    int errors = 0;
    if (result_asm != 0) {
        printf("  ERROR: Expected 0 (all within bound), got %d\n", result_asm);
        errors++;
    }
    if (result_c != 0) {
        printf("  ERROR: C reference returned %d, expected 0\n", result_c);
        errors++;
    }
    if (result_asm != result_c) {
        printf("  ERROR: ASM and C results differ\n");
        errors++;
    }

    if (errors == 0) {
        printf("  PASSED!\n\n");
    } else {
        printf("  FAILED with %d errors!\n\n", errors);
    }
}

static void test_chknorm_one_exceeds(void) {
    printf("Test 6: poly_chknorm with one coefficient exceeding bound...\n");

    int32_t input[MLDSA_N];
    int32_t B = 1000;

    // All values in range except one
    for (int i = 0; i < MLDSA_N; i++) {
        input[i] = (rand() % (B - 1)) - (B / 2);
    }
    input[123] = B + 5;  // This one exceeds

    TIMER_START();
    for (int i = 0; i < ITERATIONS; i++) {
        volatile int result = POLY_CHKNORM_FN(input, B);
        (void)result;
    }
    TIMER_END("ASM");

    int result_asm = POLY_CHKNORM_FN(input, B);
    int result_c = poly_chknorm_c_ct(input, B);

    printf("  ASM: %d, C ref: %d (expected 1)\n", result_asm, result_c);

    int errors = 0;
    if (result_asm != 1) {
        printf("  ERROR: Expected 1 (one exceeds bound), got %d\n", result_asm);
        errors++;
    }
    if (result_c != 1) {
        printf("  ERROR: C reference returned %d, expected 1\n", result_c);
        errors++;
    }
    if (result_asm != result_c) {
        printf("  ERROR: ASM and C results differ\n");
        errors++;
    }

    if (errors == 0) {
        printf("  PASSED!\n\n");
    } else {
        printf("  FAILED with %d errors!\n\n", errors);
    }
}

static void test_chknorm_negative_exceeds(void) {
    printf("Test 7: poly_chknorm with negative coefficient exceeding bound...\n");

    int32_t input[MLDSA_N];
    int32_t B = 1000;

    // All values in range except one negative
    for (int i = 0; i < MLDSA_N; i++) {
        input[i] = (rand() % (B - 1)) - (B / 2);
    }
    input[45] = -(B + 10);  // This one exceeds (negative)

    TIMER_START();
    for (int i = 0; i < ITERATIONS; i++) {
        volatile int result = POLY_CHKNORM_FN(input, B);
        (void)result;
    }
    TIMER_END("ASM");

    int result_asm = POLY_CHKNORM_FN(input, B);
    int result_c = poly_chknorm_c_ct(input, B);

    printf("  ASM: %d, C ref: %d (expected 1)\n", result_asm, result_c);

    int errors = 0;
    if (result_asm != 1) {
        printf("  ERROR: Expected 1 (one exceeds bound), got %d\n", result_asm);
        errors++;
    }
    if (result_c != 1) {
        printf("  ERROR: C reference returned %d, expected 1\n", result_c);
        errors++;
    }
    if (result_asm != result_c) {
        printf("  ERROR: ASM and C results differ\n");
        errors++;
    }

    if (errors == 0) {
        printf("  PASSED!\n\n");
    } else {
        printf("  FAILED with %d errors!\n\n", errors);
    }
}

static void test_chknorm_many_exceed(void) {
    printf("Test 8: poly_chknorm with many coefficients exceeding bound...\n");

    int32_t input[MLDSA_N];
    int32_t B = 500;

    // Many values exceed bound
    for (int i = 0; i < MLDSA_N; i++) {
        if (i % 4 == 0) {
            input[i] = B + (rand() % 1000);
        } else if (i % 4 == 1) {
            input[i] = -(B + (rand() % 1000));
        } else {
            input[i] = (rand() % (B - 1)) - (B / 2);
        }
    }

    TIMER_START();
    for (int i = 0; i < ITERATIONS; i++) {
        volatile int result = POLY_CHKNORM_FN(input, B);
        (void)result;
    }
    TIMER_END("ASM");

    int result_asm = POLY_CHKNORM_FN(input, B);
    int result_c = poly_chknorm_c_ct(input, B);

    printf("  ASM: %d, C ref: %d (expected 1)\n", result_asm, result_c);

    int errors = 0;
    if (result_asm != 1) {
        printf("  ERROR: Expected 1 (many exceed bound), got %d\n", result_asm);
        errors++;
    }
    if (result_c != 1) {
        printf("  ERROR: C reference returned %d, expected 1\n", result_c);
        errors++;
    }
    if (result_asm != result_c) {
        printf("  ERROR: ASM and C results differ\n");
        errors++;
    }

    if (errors == 0) {
        printf("  PASSED!\n\n");
    } else {
        printf("  FAILED with %d errors!\n\n", errors);
    }
}

static void test_chknorm_zero_bound(void) {
    printf("Test 9: poly_chknorm with B=0 (all non-zero should exceed)...\n");

    int32_t input[MLDSA_N];

    init_poly_random(input, -100, 100);

    int result_asm = POLY_CHKNORM_FN(input, 0);
    int result_c = poly_chknorm_c_ct(input, 0);

    printf("  ASM: %d, C ref: %d (expected 1)\n", result_asm, result_c);

    int errors = 0;
    if (result_asm != 1) {
        printf("  ERROR: Expected 1 (B=0, all non-zero exceed), got %d\n", result_asm);
        errors++;
    }
    if (result_c != 1) {
        printf("  ERROR: C reference returned %d, expected 1\n", result_c);
        errors++;
    }

    if (errors == 0) {
        printf("  PASSED!\n\n");
    } else {
        printf("  FAILED with %d errors!\n\n", errors);
    }
}

/* ========================================
 * Main
 * ======================================== */

int main(void) {
    printf("==============================================\n");
    printf("ML-DSA Poly Operations Test Suite (%s variant)\n", VARIANT_STR);
    printf("Iterations per test: %d\n", ITERATIONS);
    printf("==============================================\n\n");

    // Seed random number generator
    srand((unsigned int)time(NULL));

    // poly_caddq tests
    printf("========== poly_caddq Tests ==========\n\n");
    test_caddq_all_positive();
    test_caddq_all_negative();
    test_caddq_mixed();
    test_caddq_zeros();

    // poly_chknorm tests
    printf("========== poly_chknorm Tests ==========\n\n");
    test_chknorm_all_within();
    test_chknorm_one_exceeds();
    test_chknorm_negative_exceeds();
    test_chknorm_many_exceed();
    test_chknorm_zero_bound();

    printf("==============================================\n");
    printf("All tests completed!\n");
    printf("Variant: %s\n", VARIANT_STR);
    printf("==============================================\n");

    return 0;
}
