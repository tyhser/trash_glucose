#Sources
DRIVERS_SRC = Drivers
DRIVERS_FILES =     \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_iwdg.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c \
					$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \

FEATURE = $(PROJ_PATH)/$(DRIVERS_SRC)/feature.mk
include $(FEATURE)

ifeq ($(FEATURE_XX), y)
	CFLAGS += -DFEATURE_XX
endif

CFILES += $(DRIVERS_FILES)

# include path
CFLAGS += -I$(SOURCE_DIR)/$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Inc
CFLAGS += -I$(SOURCE_DIR)/$(DRIVERS_SRC)/STM32F4xx_HAL_Driver/Inc/Legacy
CFLAGS += -I$(SOURCE_DIR)/$(DRIVERS_SRC)/CMSIS/Device/ST/STM32F4xx/Include
CFLAGS += -I$(SOURCE_DIR)/$(DRIVERS_SRC)/CMSIS/Include
