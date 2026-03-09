# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
mkdir build && cd build
cmake ..                 # Configure (NEON variant, default)
cmake -DBUILD_SVE=ON ..  # Configure SVE variant
cmake --build .          # Build configured variant
./test_poly_ops_neon     # Run NEON variant
./test_poly_ops_sve      # Run SVE variant
```

**Note**: CMake caches the configuration. To switch variants, either delete `build/` and reconfigure, or use separate build directories (e.g., `build_neon/` and `build_sve/`).

## Architecture Overview

This is a test suite for two ML-DSA polynomial operations optimized with ARM NEON assembly, with SVE implementations as learning targets:

- **`poly_caddq`**: Ensures polynomial coefficients are non-negative by adding MLDSA_Q to negative values
- **`poly_chknorm`**: Verifies all coefficients have absolute value less than bound B

### File Organization

- `common.h` - Shared macros (namespace, alignment, symbol exports)
- `src/` - Implementations
  - `poly_c.c` - C reference implementations (constant-time variants)
  - `poly_caddq_asm.S` / `poly_caddq_asm_sve.S` - NEON/SVE implementations
  - `poly_chknorm_asm.S` / `poly_chknorm_asm_sve.S` - NEON/SVE implementations
  - `include/arith_native_aarch64.h` - Function declarations
- `test.c` - Test suite comparing assembly vs C reference with timing

### Key Patterns

- **Symbol naming**: `MLD_ASM_NAMESPACE` macro handles Linux (no prefix) vs macOS (`_` prefix)
- **Loop unrolling**: NEON processes 16 coefficients per iteration (4x 128-bit vectors)
- **Constant-time**: C reference uses bitwise operations to avoid branches
- **SVE translation**: NEON `dup`→`mov zN.s`, `ldr`→`ld1w`, `mla`→`mla zN.s, p0/m`, predicates via `ptrue`/`whilelt`

### SVE Optimization Learnings

- **Use `addvl` for pointer iteration**: The original SVE implementation used `incw x2` (increment by 1) which only advanced the index by 1 element per iteration, causing redundant processing. Use `addvl x2, x2, #1` to advance by full vector length.
- **4-vector unrolling for poly_caddq not beneficial**: Unlike NEON, unrolling to process 4 vectors per iteration shows no performance improvement on this hardware. The scattered load/store pattern with multiple pointer registers adds overhead that cancels any loop overhead reduction.
- **4-vector unrolling for poly_chknorm IS beneficial**: Processing 4 vectors (32 coefficients) per iteration with early exit via `b.any` after each compare provides ~2.1x speedup over NEON.
- **Early exit with `b.any`**: Using `cmpge` followed by `b.any` provides zero-cost predicate testing and immediate exit, faster than `incp` + `cbnz`.

### Performance Summary

| Function | NEON | SVE (single-vector) | SVE (4-vector) |
|----------|------|---------------------|----------------|
| `poly_caddq` | ~360 μs | ~360 μs | ~365-370 μs |
| `poly_chknorm` | ~320 μs | ~214 μs (1.5x) | ~152 μs (2.1x) |
