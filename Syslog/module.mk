#Sources
SYSLOG_SRC = Syslog
SYSLOG_FILES = \
			   $(SYSLOG_SRC)/src/syslog.c

FEATURE = $(PROJ_PATH)/$(SYSLOG_SRC)/feature.mk
include $(FEATURE)

CFILES += $(SYSLOG_FILES)

# include path
CFLAGS += -I$(SOURCE_DIR)/Syslog/inc
