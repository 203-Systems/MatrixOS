#DEPS_SUBMODULES +=

MCU = esp32s3
UF2_FAMILY_ID = 0xc47e5767

include $(FAMILY_PATH)/Variants/$(DEVICE)/Device.mk

.PHONY: clean flash bootloader-flash app-flash erase monitor dfu-flash dfu

build-common:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) $(CMAKE_DEFSYM) -DMODE=$(MODE) -DIDF_TARGET=${MCU} build

# Build commands for different modes
build-release: MODE = RELEASE
build-release: build-common

build-rc: MODE = RELEASECANDIDATE
build-rc: build-common

build-beta: MODE = BETA
build-beta: build-common

build-nightly: MODE = NIGHTLY
build-nightly: build-common

build-dev: MODE = DEVELOPMENT
build-dev: build-common

build: MODE = UNDEFINED
build: build-common

clean:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) $(CMAKE_DEFSYM) clean
	

fullclean:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) $(CMAKE_DEFSYM) fullclean
# 	if exist build rmdir build /S /Q
#     if exist _build rmdir _build /S /Q
#     if exist sdkconfig del sdkconfig /S /Q

flash bootloader-flash app-flash erase monitor dfu-flash dfu:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) $(CMAKE_DEFSYM) $@

uf2: $(BUILD)/$(PROJECT)-$(DEVICE).uf2

$(BUILD)/$(PROJECT)-$(DEVICE).uf2: $(BUILD)/$(PROJECT)-$(DEVICE).bin
	@echo CREATE $@
	python tools/uf2/utils/uf2conv.py -f $(UF2_FAMILY_ID) -b 0x0 -c -o $@ $^

upload:
	python tools/uf2/utils/uf2upload.py -f $(BUILD)/$(PROJECT)-$(DEVICE).uf2 -d "$(UF2_MODEL)" -l

uf2-upload: $(BUILD)/$(PROJECT)-$(DEVICE).uf2 upload

menuconfig:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) menuconfig