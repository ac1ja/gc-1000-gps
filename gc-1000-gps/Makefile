# A makefile for building nick's GPS time sync
# Uses arduino-cli, adapted from Steven Hale and others arduino make file practices
#
# 2021

# What board to build for and its core
CORE     ?= arduino:avr
FQBN     ?= arduino:avr:mega

# Tools, their links and versions
ARDUINO_CLI_VERSION :=
ARDUINO_CLI_URL := https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh
ARDUINO_LINT_VERSION :=
ARDUINO_LINT_URL := https://raw.githubusercontent.com/arduino/arduino-lint/main/etc/install.sh

# The version and time we're compiling from
GIT_VERSION := "$(shell git describe --abbrev=4 --dirty --always --tags)"
COMPILED_DATE := "$(shell date)"

# Treat all warnings as errors.
BUILDPROP := compiler.warning_flags.all='-Wall -Wextra -Werror'

# Locations
ROOT := $(PWD)
BINDIR := $(ROOT)/_build/bin
BUILDDIR := $(ROOT)/_build

# What port to build on
ifndef SERIAL_DEV
  ifneq (,$(wildcard /dev/ttyUSB0))
    SERIAL_DEV = /dev/ttyUSB0
  else ifneq (,$(wildcard /dev/ttyACM0))
    SERIAL_DEV = /dev/ttyACM0
  else
    SERIAL_DEV = unknown
  endif
endif


.PHONY: all help build upload version release requirements

# Do everything
all: tools requirements version build


help:
	@echo
	@echo "Targets:"
	@echo "   lint       	De-lint the software."
	@echo "   build      	Build the software."
	@echo "   upload     	Upload the software."
	@echo "   requirements	Fetch all requirements."
	@echo


# Set VERBOSE=1 to not run in --silent

ifndef VERBOSE
.SILENT:
endif

# Only build the code
build: version gc-1000-gps.ino
	$(BINDIR)/arduino-cli core install $(CORE)
	$(BINDIR)/arduino-cli compile -b $(FQBN) gc-1000-gps

# Only upload the software
upload: version build gc-1000-gps.ino
	$(BINDIR)/arduino-cli upload -b $(FQBN) gc-1000-gps -p $(SERIAL_DEV)

# Only update the version
version:
	echo "#define VERSION \"$(GIT_VERSION)\"" > version.h
	echo "#define COMPILED_ON \"$(COMPILED_DATE)\"" >> version.h

# Generate an artifact
release: build gc-1000-gps.ino
	$(BINDIR)/arduino-cli compile -b $(FQBN) gc-1000-gps --output-dir $(BUILDDIR)

# Only fetch the requirements
requirements:
	@if [ -e requirements.txt ]; \
	then while read -r i ; do echo ; \
		echo "---> Installing " '"'$$i'"' ; \
		$(BINDIR)/arduino-cli lib install "$$i" ; \
	done < requirements.txt ; \
	else echo "---> MISSING requirements.txt file"; \
	fi

lint:
	$(BINDIR)/arduino-lint --compliance strict

# Fetches all the tools required
tools:
	mkdir -p $(BINDIR)
	curl -fsSL $(ARDUINO_CLI_URL) | BINDIR=$(BINDIR) sh -s $(ARDUINO_CLI_VERSION)
	curl -fsSL $(ARDUINO_LINT_URL) | BINDIR=$(BINDIR) sh -s $(ARDUINO_LINT_VERSION)
