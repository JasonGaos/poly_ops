# Makefile for poly_ops test suite
# Works on both Linux (AArch64) and macOS (ARM64)
# Supports both NEON and SVE variants

# Detect OS
UNAME_S := $(shell uname -s)

# Variant selection: neon (default) or sve
VARIANT ?= neon

# Compiler settings
CC = gcc
AS = gcc
CFLAGS = -Wall -Wextra -O2 -g
ASFLAGS = -Wall -g
LDFLAGS =

# Platform-specific flags
ifeq ($(UNAME_S),Linux)
    # Linux AArch64
    CFLAGS += -march=armv8-a
    ASFLAGS += -march=armv8-a

    # Add SVE flags if building SVE variant
    ifeq ($(VARIANT),sve)
        CFLAGS += -march=armv8-a+sve -DHAVE_SVE
        ASFLAGS += -march=armv8-a+sve
    endif
else ifeq ($(UNAME_S),Darwin)
    # macOS ARM64
    CFLAGS += -arch arm64
    ASFLAGS += -arch arm64
    LDFLAGS += -arch arm64

    # Add SVE flags if building SVE variant (macOS 13+)
    ifeq ($(VARIANT),sve)
        CFLAGS += -DHAVE_SVE
    endif
endif

# Include paths
INCLUDES = -I. -Isrc/include

# Source files
ASM_NEON_CADDQ_SRC = src/poly_caddq_asm.S
ASM_NEON_CHKNORM_SRC = src/poly_chknorm_asm.S
ASM_SVE_CADDQ_SRC = src/poly_caddq_asm_sve.S
ASM_SVE_CHKNORM_SRC = src/poly_chknorm_asm_sve.S
C_REF_SRC = src/poly_c.c
TEST_SRC = test.c

# Object files
C_REF_OBJ = $(C_REF_SRC:.c=.o)
TEST_OBJ = $(TEST_SRC:.c=.o)

# Select assembly source based on variant
ifeq ($(VARIANT),sve)
    ASM_CADDQ_OBJ = $(ASM_SVE_CADDQ_SRC:.S=.o)
    ASM_CHKNORM_OBJ = $(ASM_SVE_CHKNORM_SRC:.S=.o)
    TARGET_SUFFIX = _sve
else
    ASM_CADDQ_OBJ = $(ASM_NEON_CADDQ_SRC:.S=.o)
    ASM_CHKNORM_OBJ = $(ASM_NEON_CHKNORM_SRC:.S=.o)
    TARGET_SUFFIX = _neon
endif

# Output binary with variant suffix
TARGET = test_poly_ops$(TARGET_SUFFIX)

.PHONY: all clean run help neon sve both

# Default target builds selected variant
all: $(TARGET)

# Build NEON variant
neon:
	$(MAKE) VARIANT=neon

# Build SVE variant
sve:
	$(MAKE) VARIANT=sve

# Build both variants
both: neon sve

$(TARGET): $(ASM_CADDQ_OBJ) $(ASM_CHKNORM_OBJ) $(C_REF_OBJ) $(TEST_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.S
	$(AS) $(ASFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f src/*.o $(TEST_OBJ) test_poly_ops test_poly_ops_neon test_poly_ops_sve

# Help target
help:
	@echo "Available targets:"
	@echo "  make          - Build $(VARIANT) variant (default: neon)"
	@echo "  make neon     - Build NEON variant"
	@echo "  make sve      - Build SVE variant"
	@echo "  make both     - Build both NEON and SVE variants"
	@echo "  make run      - Build and run the tests"
	@echo "  make clean    - Remove built files"
	@echo "  make help     - Show this help message"
	@echo ""
	@echo "Current settings:"
	@echo "  Platform: $(UNAME_S)"
	@echo "  Variant: $(VARIANT)"
	@echo "  Target: $(TARGET)"
