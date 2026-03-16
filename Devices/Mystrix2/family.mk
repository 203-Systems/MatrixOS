MCU = esp32s3
UF2_FAMILY_ID = 0xc47e5767

.PHONY: build flash bootloader-flash app-flash erase monitor dfu-flash dfu

build:
	cmake -B $(BUILD) -Wno-dev . -DCMAKE_TOOLCHAIN_FILE=$(IDF_PATH)/tools/cmake/toolchain-${MCU}.cmake -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) -DMODE=$(MODE) -GNinja
	cmake --build $(BUILD) 

$(BUILD)/$(PROJECT)-$(DEVICE).bin:
	cmake -B $(BUILD) -Wno-dev . -DCMAKE_TOOLCHAIN_FILE=$(IDF_PATH)/tools/cmake/toolchain-${MCU}.cmake -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) -DMODE=$(MODE) -GNinja
	cmake --build $(BUILD) 

flash bootloader-flash app-flash erase monitor dfu-flash dfu:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) $@

uf2: $(BUILD)/$(PROJECT)-$(DEVICE).uf2

$(BUILD)/$(PROJECT)-$(DEVICE).uf2: $(BUILD)/$(PROJECT)-$(DEVICE).bin
	@echo CREATE $@
	python Tools/uf2/utils/uf2conv.py -f $(UF2_FAMILY_ID) -b 0x0 -c -o $@ $^

upload:
	python Tools/uf2/utils/uf2upload.py -f $(BUILD)/$(PROJECT)-$(DEVICE).uf2 -d "$(UF2_MODEL)" -l

uf2-upload: $(BUILD)/$(PROJECT)-$(DEVICE).uf2 upload

menuconfig:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) menuconfig