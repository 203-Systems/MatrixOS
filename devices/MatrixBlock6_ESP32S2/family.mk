#DEPS_SUBMODULES +=

MCU = esp32s2
UF2_FAMILY_ID = 0xbfdd4eee

.PHONY: all clean flash bootloader-flash app-flash erase monitor dfu-flash dfu

all:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) $(CMAKE_DEFSYM) -DIDF_TARGET=${MCU} build

build: all

clean:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) $(CMAKE_DEFSYM) clean

# fullclean:
# 	if exist build rmdir build /S /Q
#     if exist _build rmdir _build /S /Q
#     if exist sdkconfig del sdkconfig /S /Q

flash bootloader-flash app-flash erase monitor dfu-flash dfu:
	idf.py -B$(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) $(CMAKE_DEFSYM) $@

uf2: $(BUILD)/$(PROJECT)-$(DEVICE).uf2

$(BUILD)/$(PROJECT)-$(DEVICE).uf2: $(BUILD)/$(PROJECT)-$(DEVICE).bin
	@echo CREATE $@
	python tools/uf2/utils/uf2conv.py -f $(UF2_FAMILY_ID) -b 0x0 -c -o $@ $^
