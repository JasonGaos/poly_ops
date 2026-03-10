# ML-DSA Poly Operations Test Repository

This repository contains a standalone test suite for two simple ML-DSA polynomial operations optimized with ARM NEON assembly. The goal is to provide a learning environment for rewriting NEON code to SVE/SVE2.

## Functions Included

### 1. `poly_caddq` - Conditional Addition of Q

**Purpose**: Ensures all polynomial coefficients are non-negative by adding `MLDSA_Q` to negative coefficients.

**Mathematical Operation**:
```
For each coefficient a[i]:
    if a[i] < 0:
        a[i] = a[i] + MLDSA_Q
```

**Function Signature**:
```c
void mldsa_poly_caddq_asm(int32_t *a);
```

**Parameters**:
- `a`: Pointer to array of 256 int32_t coefficients (modified in-place)

**Pre-condition**: Coefficients in range `[-MLDSA_Q+1, MLDSA_Q-1]`
**Post-condition**: Coefficients in range `[0, MLDSA_Q-1]`

### 2. `poly_chknorm` - Check Coefficient Bound

**Purpose**: Verifies that all coefficients have absolute value less than a given bound B.

**Mathematical Operation**:
```
Return 0 if |a[i]| < B for all i = 0..255
Return 1 if |a[i]| >= B for any i
```

**Function Signature**:
```c
int mldsa_poly_chknorm_asm(const int32_t *a, int32_t B);
```

**Parameters**:
- `a`: Pointer to array of 256 int32_t coefficients (read-only)
- `B`: Bound to check against

**Returns**: `0` if all coefficients within bound, `1` otherwise

---

## File Structure

```
├── CMakeLists.txt                      # Build configuration
├── common.h                            # Shared macros for assembly and C
├── test.c                              # Test suite
└── src/
    ├── poly_caddq_asm.S                # NEON implementation of poly_caddq
    ├── poly_caddq_asm_sve.S            # SVE implementation of poly_caddq
    ├── poly_chknorm_asm.S              # NEON implementation of poly_chknorm
    ├── poly_chknorm_asm_sve.S          # SVE implementation of poly_chknorm
    ├── poly_c.c                        # C reference implementations
    └── include/
        └── arith_native_aarch64.h     # Function declarations
```

---

## Building

### Prerequisites

- An AArch64 Linux system or ARM64 macOS system
- GCC compiler with AArch64 support
- CMake

### Build Commands

```bash
# Create build directory and configure (NEON variant, default)
mkdir build && cd build
cmake ..

# Or configure SVE variant
cmake -DBUILD_SVE=ON ..

# Build configured variant
cmake --build .

# Run tests
./test_poly_ops_neon     # NEON variant
./test_poly_ops_sve      # SVE variant
```

**Note**: To switch between NEON and SVE variants, either delete the `build/` directory and reconfigure, or use separate build directories (e.g., `build_neon/` and `build_sve/`).

```bash
# Recommended: separate build directories
mkdir build_neon && cd build_neon
cmake .. && cmake --build .

mkdir build_sve && cd build_sve
cmake -DBUILD_SVE=ON .. && cmake --build .
```

---

## Running the Tests

```bash
# Run NEON variant tests
./test_poly_ops_neon

# Run SVE variant tests
./test_poly_ops_sve
```

### Test Coverage

#### poly_caddq Tests
1. **All positive** - Verifies positive coefficients are unchanged
2. **All negative** - Verifies MLDSA_Q is added to all negative coefficients
3. **Mixed** - Tests combination of positive and negative coefficients
4. **All zeros** - Verifies zero values remain zero

#### poly_chknorm Tests
5. **All within bound** - All coefficients satisfy `|a[i]| < B`
6. **One exceeds (positive)** - Single coefficient exceeds positive bound
7. **One exceeds (negative)** - Single coefficient exceeds negative bound
8. **Many exceed** - Multiple coefficients exceed the bound
9. **Zero bound** - Edge case with B=0

---

## NEON to SVE Translation Guide

### Key NEON Instructions Used

| NEON Instruction | Purpose | SVE Equivalent |
|------------------|---------|----------------|
| `dup v4.4s, w9` | Broadcast scalar to all lanes | `mov z4.s, w9` + `dup` or `index` |
| `ldr q0, [x0]` | Load 128-bit (4×int32) | `ld1w {z0.s}, p0/z, [x0]` |
| `ushr v5.4s, v0.4s, #31` | Unsigned shift right (extract sign) | `lsr z5.s, z0.s, #31` |
| `mla v0.4s, v5.4s, v4.4s` | v0 += v5 * v4 (multiply-add) | `mad z0.s, p0/m, z5.s, z4.s` |
| `abs v1.4s, v1.4s` | Absolute value | `abs z1.s, p0/m, z1.s` |
| `cmge v1.4s, v1.4s, v20.4s` | Compare greater-or-equal | `cmpge p1.s, p0/z, z1.s, z20.s` |
| `orr v21.16b, v21.16b, v1.16b` | Bitwise OR | `orr z21.b, p0/m, z21.b, z1.b` |
| `umaxv s21, v21.4s` | Horizontal maximum (reduce) | `umaxv s21, p0, z21.s` |
| `str q0, [x0], #0x40` | Store and increment | `st1w {z0.s}, p0, [x0, #0x40, MVL #4]` |

### SVE Implementation Strategy

1. **Predicate Setup**: Always start with `ptrue p0.s` to set predicate for all active lanes
2. **Vector Length**: SVE uses variable vector length (VL), use `whilelo` for exact bounds
3. **Load/Store**: Use `ld1w`/`st1w` with predicate-gathering for exact element count
4. **Conditional Operations**: Use predicates with `sel` instruction for conditional adds
5. **Pointer iteration**: Use `addvl` for pointer advancement, not `incw`

---

## Performance Results

Test environment: AArch64 Linux, 10,000 iterations per measurement

| Function | NEON | SVE (optimized) |
|----------|------|-----------------|
| `poly_caddq` | ~360 μs | ~360 μs |
| `poly_chknorm` | ~320 μs | ~350 μs |

**Note**: SVE matches NEON performance for `poly_caddq`. For `poly_chknorm`, SVE is ~10% slower due to predicate handling overhead when processing all coefficients without early exit.

### Analysis

1. **poly_caddq**: The original SVE implementation had a bug where it processed one scalable vector width per iteration but only incremented the index by 1 (`incw x2`). After fixing to use pointer-based iteration with `addvl` (add vector length), SVE matches NEON performance.

   **Why 4-vector unrolling shows no improvement**:
   - Actual hardware may have VL > 256 bits, reducing benefit of unrolling
   - Additional pointer register management adds overhead
   - Scattered load/store pattern may be less efficient than sequential access
   - The single-vector SVE already achieves full memory bandwidth utilization

2. **poly_chknorm**: Both NEON and SVE process all 256 coefficients without early exit.
   - NEON uses `cmge` which directly produces bitmask results per lane
   - SVE uses `cmpge` which produces predicates, requiring conversion via `not` + `orr`
   - SVE's predicate handling adds ~10% overhead compared to NEON's simpler approach

**Key insight**: For workloads that must process all data (no early exit), NEON's simpler instruction model can be more efficient than SVE's predicate-based approach. SVE's advantage lies in scenarios where early termination is allowed.

---

## Learning Path

1. **Study the NEON implementation** in `src/poly_caddq_asm.S` and `src/poly_chknorm_asm.S`
2. **Understand the algorithm** by reading the comments and C reference in `src/poly_c.c`
3. **Read the SVE implementation** in `src/poly_caddq_asm_sve.S` and `src/poly_chknorm_asm_sve.S`
4. **Build and run tests** to verify correctness:

```bash
mkdir build && cd build
cmake .. && cmake --build .
./test_poly_ops_neon
```

---

## ML-DSA Context

These functions are used in ML-DSA (Module-Lattice-Based Digital Signature Algorithm), standardized in FIPS 204. They operate on polynomials with 256 coefficients in the ring `Z_q[x]/(x^n+1)` where:
- `n = 256` (polynomial degree)
- `q = 8380417` (modulus)

**Why these functions matter**:
- `poly_caddq`: Ensures coefficients are in canonical range after subtraction operations
- `poly_chknorm`: Verifies boundedness during signature generation (affects security)

---

## Cross-Platform Notes

- **Linux**: No underscore prefix on symbols
- **macOS**: Symbols get a leading underscore prefix
- The `common.h` header handles this automatically via `MLD_ASM_NAMESPACE` macro

---

## License

This code is derived from the mldsa-native project and is licensed under Apache-2.0 OR ISC OR MIT.
