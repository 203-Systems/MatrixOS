MICROPYTHON_TOP = ../../../Library/micropython
PACKAGE_DIR = ../MicroPythonEmbed
USER_C_MODULES = usermod

.PHONY: all
all: matrixos-micropython-embed-package

include $(MICROPYTHON_TOP)/ports/embed/embed.mk

.PHONY: matrixos-micropython-embed-package
matrixos-micropython-embed-package: micropython-embed-package
	$(ECHO) "- MatrixOS port overrides"
	$(Q)$(CP) port/*.[ch] $(PACKAGE_DIR)/port
