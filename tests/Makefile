CMSIS_ROOT := ../src/libs/CMSIS-DSP
SRC_DIR := ../src
BUILDDIR := build/lib
TESTDIR := build/tests
UNITDIR := ./unit

CXX := g++
CC  := gcc
CFLAGS := -Wall -g -D__GNUC_PYTHON__ -I$(CMSIS_ROOT)/Include -I$(SRC_DIR)/dsp -I.
CXXFLAGS := -Wall -g -D__GNUC_PYTHON__ -I$(CMSIS_ROOT)/Include -I$(SRC_DIR)/dsp -I.

TESTLIB := -lCppUTest -lm -lsndfile

LIBCMSIS := $(BUILDDIR)/libCMSISDSP.a
TESTBIN  := $(TESTDIR)/dsp_tests
UNITBIN  := $(TESTDIR)/unit_tests

SRC     := $(SRC_DIR)/dsp/dsp.c $(SRC_DIR)/dsp/fir_coeffs.c
TESTSRC := test_main.cpp test_dsp.cpp


UNITSRC := $(UNITDIR)/test_main.cpp $(UNITDIR)/testdsp.cpp

.PHONY: all tests run unit unit-run clean help

all: tests


tests: $(TESTBIN)

$(TESTBIN): $(SRC) $(TESTSRC) $(LIBCMSIS)
	@mkdir -p $(TESTDIR)
	$(CXX) $(CXXFLAGS) $(SRC) $(TESTSRC) $(LIBCMSIS) $(TESTLIB) -o $@


unit: $(UNITBIN)

$(UNITBIN): $(SRC) $(UNITSRC) $(LIBCMSIS)
	@mkdir -p $(TESTDIR)
	$(CXX) $(CXXFLAGS) $(SRC) $(UNITSRC) $(LIBCMSIS) $(TESTLIB) -o $@


unit-run: $(UNITBIN)
	./$(UNITBIN)

$(LIBCMSIS):
	@mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake -DHOST=ON ../../$(CMSIS_ROOT)/Source
	$(MAKE) -C $(BUILDDIR)


run: $(TESTBIN)
	./$(TESTBIN)

# 
clean:
	rm -rf $(BUILDDIR) $(TESTDIR)

clean-all: clean
