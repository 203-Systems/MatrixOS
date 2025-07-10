DEVICE ?= Mystrix
MODE ?= UNDEFINED

ifeq ($(OS),Windows_NT)
    MKDIR = if not exist build\\$(DEVICE) mkdir build\\$(DEVICE)
    CD = cd build\\$(DEVICE)
    SEP = \\
    CHAIN = &
else
    MKDIR = mkdir -p build/$(DEVICE)
    CD = cd build/$(DEVICE)
    SEP = /
    CHAIN = &&
endif

.PHONY: all build build-dev build-release build-beta build-rc build-nightly

all: build

build:
	@$(MKDIR)
	@$(CD) $(CHAIN) cmake -DDEVICE=$(DEVICE) -DMODE=$(MODE) ..$(SEP)..
	@$(CD) $(CHAIN) cmake --build . --verbose

build-dev:
	$(MAKE) MODE=DEVELOPMENT  build

build-release:
	 $(MAKE) MODE=RELEASE build

build-beta:
	$(MAKE) MODE=BETA build

build-rc:
	$(MAKE) MODE=RELEASECANDIDATE build

build-nightly:
	$(MAKE) MODE=NIGHTLY build
