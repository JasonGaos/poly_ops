# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
mkdir build && cd build
cmake ..                 # Configure (builds both NEON and SVE)
cmake --build .          # Build single binary with both variants
./test_poly_ops          # Run tests for both NEON and SVE
```

**Note**: The single `test_poly_ops` binary contains both NEON and SVE implementations and runs tests for both, comparing results against the C reference implementation.

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
- `test.c` - Test suite comparing NEON, SVE, and C reference implementations with timing

### Key Patterns

- **Symbol naming**: `MLD_ASM_NAMESPACE` macro handles Linux (no prefix) vs macOS (`_` prefix)
- **Loop unrolling**: NEON processes 16 coefficients per iteration (4x 128-bit vectors)
- **Constant-time**: C reference uses bitwise operations to avoid branches
- **SVE translation**: NEON `dup`→`mov zN.s`, `ldr`→`ld1w`, `mla`→`mla zN.s, p0/m`, predicates via `ptrue`/`whilelt`

### SVE Optimization Learnings

- **Use `addvl` for pointer iteration**: The original SVE implementation used `incw x2` (increment by 1) which only advanced the index by 1 element per iteration, causing redundant processing. Use `addvl x2, x2, #1` to advance by full vector length.
- **4-vector unrolling for poly_caddq not beneficial**: Unlike NEON, unrolling to process 4 vectors per iteration shows no performance improvement on this hardware. The scattered load/store pattern with multiple pointer registers adds overhead that cancels any loop overhead reduction.
- **4-vector unrolling for poly_chknorm**: Processing 4 vectors (32 coefficients) per iteration matches NEON's loop structure. The current implementation processes all coefficients without early exit to match NEON behavior.
- **Predicate handling**: SVE `cmpge` produces predicate registers, requiring conversion to vector masks via `eor` + `not` + `orr` for accumulation.

### Performance Summary

| Function | NEON | SVE |
|----------|------|-----|
| `poly_caddq` | ~361 μs | ~385 μs |
| `poly_chknorm` | ~320 μs | ~407 μs |

**Note**: NEON outperforms SVE on this hardware. For `poly_caddq`, NEON is ~6% faster. For `poly_chknorm`, NEON is ~27% faster. The SVE implementation has overhead from predicate handling and scattered load/store patterns that cancel any benefits from scalable vectors.
