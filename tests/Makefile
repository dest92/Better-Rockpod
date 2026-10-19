# Host-side unit tests for Better-Rockpod.
#
#   make -C tests          # build + run everything
#   make -C tests build    # build only
#   make -C tests clean
#
# Each test_*.c is an independent host executable built with ASan+UBSan.
# To add a test: create test_<name>.c, add <name> to TESTS, and declare
# test_<name>_SRCS / test_<name>_CFLAGS if it needs rockbox sources.
# Only host-compilable units belong here (no firmware headers) — see
# .claude/skills/unit-tests/SKILL.md.

CC ?= cc
ROOT := ..
BUILD := bin

# Rockbox fixed-point code left-shifts negative values pervasively
# (fp_div and friends) — defined behavior on GCC/two's-complement but
# strict-C UB, so the shift check is off to keep UBSan signal useful.
SAN := -fsanitize=address,undefined -fno-sanitize=shift \
       -fno-sanitize-recover=all -fno-omit-frame-pointer
CFLAGS := -std=gnu99 -W -Wall -Wextra -g -O1 $(SAN) -I.

TESTS := fixedpoint pf_aa_source aa_color_math shuffle_locality lowbatt_debounce battcurve

test_fixedpoint_SRCS := $(ROOT)/lib/fixedpoint/fixedpoint.c
test_fixedpoint_CFLAGS := -I$(ROOT)/lib/fixedpoint

test_pf_aa_source_CFLAGS := -I$(ROOT)/apps/plugins/pictureflow

test_aa_color_math_CFLAGS := -I$(ROOT)/apps/gui/skin_engine

test_shuffle_locality_CFLAGS := -I$(ROOT)/apps

test_lowbatt_debounce_CFLAGS := -I$(ROOT)/firmware/export

test_battcurve_CFLAGS := -I$(ROOT)/apps/plugins/lib

.PHONY: all build run clean

all: run

define TEST_template
$(BUILD)/test_$(1): test_$(1).c $$(test_$(1)_SRCS) rbtest.h Makefile
	@mkdir -p $(BUILD)
	$$(CC) $$(CFLAGS) $$(test_$(1)_CFLAGS) -o $$@ test_$(1).c $$(test_$(1)_SRCS)
endef
$(foreach t,$(TESTS),$(eval $(call TEST_template,$(t))))

build: $(foreach t,$(TESTS),$(BUILD)/test_$(t))

run: build
	@fail=0; \
	for t in $(TESTS); do \
	    printf '== test_%s: ' $$t; \
	    ./$(BUILD)/test_$$t || fail=1; \
	done; \
	exit $$fail

clean:
	rm -rf $(BUILD)
