def_$(CONFIG_UCOSII) += __ucosii__
inc_$(CONFIG_UCOSII) += $(UCOSII_BASE)/inc

ifneq ($(CONFIG_UCOSII), 1)
CONFIG_UCOSII_WIN32 := 0
endif

-include $(UCOSII_BASE)/port-win32/subdir.mk
-include $(UCOSII_BASE)/src/subdir.mk
