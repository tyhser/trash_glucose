#Sources

PROJECT_SRC = project
PROJECT_FILES = $(PROJECT_SRC)/src/main.c \
				$(PROJECT_SRC)/src/gpio.c \
				$(PROJECT_SRC)/src/iwdg.c \
				$(PROJECT_SRC)/src/tim.c \
				$(PROJECT_SRC)/src/dma.c \
				$(PROJECT_SRC)/src/usart.c \
				$(PROJECT_SRC)/src/stm32f4xx_it.c \
				$(PROJECT_SRC)/src/stm32f4xx_hal_msp.c \
				$(PROJECT_SRC)/src/system_stm32f4xx.c \
				$(PROJECT_SRC)/src/syslog.c



FEATURE = $(PROJ_PATH)/$(PROJECT_SRC)/feature.mk
include $(FEATURE)

ifeq ($(FEATURE_CALIBRATE_ERROR_ENABLE), y)
	CFLAGS += -DFEATURE_CALIBRATE_ERROR_ENABLE
endif

CFILES += $(PROJECT_FILES)

# include path
CFLAGS += -I$(SOURCE_DIR)/$(PROJECT_SRC)/inc
