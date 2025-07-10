MCU = esp32s3
UF2_FAMILY_ID = 0xc47e5767

-include $(FAMILY_PATH)/Variants/$(DEVICE)/Device.mk

.PHONY: clean flash bootloader-flash app-flash erase monitor dfu-flash dfu

build:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) -DMODE=$(MODE) -DIDF_TARGET=${MCU} build

clean:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) clean
	
fullclean:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) fullclean

flash bootloader-flash app-flash erase monitor dfu-flash dfu:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) $@

uf2: $(BUILD)/$(PROJECT)-$(DEVICE).uf2

$(BUILD)/$(PROJECT)-$(DEVICE).uf2: $(BUILD)/$(PROJECT)-$(DEVICE).bin
	@echo CREATE $@
	python tools/uf2/utils/uf2conv.py -f $(UF2_FAMILY_ID) -b 0x0 -c -o $@ $^

upload:
	python tools/uf2/utils/uf2upload.py -f $(BUILD)/$(PROJECT)-$(DEVICE).uf2 -d "$(UF2_MODEL)" -l

uf2-upload: $(BUILD)/$(PROJECT)-$(DEVICE).uf2 upload

menuconfig:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) menuconfig