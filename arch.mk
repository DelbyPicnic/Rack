MACHINE = $(shell gcc -dumpmachine)
ifneq (,$(findstring linux,$(MACHINE)))
	# Linux
	ARCH = lin
	CPU = $(shell uname -m)

ifneq (,$(findstring arm,$(CPU)))
	BOARD = $(shell cat /sys/firmware/devicetree/base/model)

ifneq (,$(findstring Raspberry Pi 3,$(BOARD)))
	ARM_CPU_FLAGS += -march=armv8-a+crc -mtune=cortex-a53 -mfpu=neon -mfloat-abi=hard
endif

ifneq (,$(findstring Tinker Board,$(BOARD)))
	ARM_CPU_FLAGS = -march=armv7 -mtune=cortex-a17 -mfpu=neon -mfloat-abi=hard
endif
endif

else ifneq (,$(findstring apple,$(MACHINE)))
	# Mac
	ARCH = mac
else ifneq (,$(findstring mingw,$(MACHINE)))
	# Windows
	ARCH = win
else
	$(error Could not determine machine type. Try hacking around in arch.mk)
endif
