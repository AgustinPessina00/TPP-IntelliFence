################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Core/Src/TinyGPS++.cpp \
../Core/Src/adc_if.cpp \
../Core/Src/app_freertos.cpp \
../Core/Src/cow.cpp \
../Core/Src/fence.cpp \
../Core/Src/flash_if.cpp \
../Core/Src/ina226.cpp \
../Core/Src/ipcc_if.cpp \
../Core/Src/lsm6dso.cpp \
../Core/Src/main.cpp \
../Core/Src/messages.cpp \
../Core/Src/sam_m10q.cpp \
../Core/Src/stm32_lpm_if.cpp \
../Core/Src/stm32wlxx_hal_msp.cpp \
../Core/Src/stm32wlxx_it.cpp \
../Core/Src/sys_app.cpp \
../Core/Src/sys_sensors.cpp \
../Core/Src/timer_if.cpp \
../Core/Src/usart_if.cpp 

C_SRCS += \
../Core/Src/stm32wlxx_hal_timebase_tim.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c 

C_DEPS += \
./Core/Src/stm32wlxx_hal_timebase_tim.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d 

OBJS += \
./Core/Src/TinyGPS++.o \
./Core/Src/adc_if.o \
./Core/Src/app_freertos.o \
./Core/Src/cow.o \
./Core/Src/fence.o \
./Core/Src/flash_if.o \
./Core/Src/ina226.o \
./Core/Src/ipcc_if.o \
./Core/Src/lsm6dso.o \
./Core/Src/main.o \
./Core/Src/messages.o \
./Core/Src/sam_m10q.o \
./Core/Src/stm32_lpm_if.o \
./Core/Src/stm32wlxx_hal_msp.o \
./Core/Src/stm32wlxx_hal_timebase_tim.o \
./Core/Src/stm32wlxx_it.o \
./Core/Src/sys_app.o \
./Core/Src/sys_sensors.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/timer_if.o \
./Core/Src/usart_if.o 

CPP_DEPS += \
./Core/Src/TinyGPS++.d \
./Core/Src/adc_if.d \
./Core/Src/app_freertos.d \
./Core/Src/cow.d \
./Core/Src/fence.d \
./Core/Src/flash_if.d \
./Core/Src/ina226.d \
./Core/Src/ipcc_if.d \
./Core/Src/lsm6dso.d \
./Core/Src/main.d \
./Core/Src/messages.d \
./Core/Src/sam_m10q.d \
./Core/Src/stm32_lpm_if.d \
./Core/Src/stm32wlxx_hal_msp.d \
./Core/Src/stm32wlxx_it.d \
./Core/Src/sys_app.d \
./Core/Src/sys_sensors.d \
./Core/Src/timer_if.d \
./Core/Src/usart_if.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.cpp Core/Src/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WL55xx -c -I../Core/Inc -I../LoRaWAN/App -I../LoRaWAN/Target -I../MbMux -I../../Common/System -I../../Common/MbMux -I../../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../../Utilities/trace/adv_trace -I../../Utilities/misc -I../../Utilities/timer -I../../Utilities/lpm/tiny_lpm -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../../Middlewares/Third_Party/LoRaWAN/Mac/Region -I../../Middlewares/Third_Party/LoRaWAN/Mac -I../../Middlewares/Third_Party/LoRaWAN/LmHandler -I../../Middlewares/Third_Party/LoRaWAN/Utilities -I../../Middlewares/Third_Party/SubGHz_Phy -I../../Drivers/CMSIS/Include -I../../Drivers/STM32WLxx_HAL_Driver -I../../Drivers/STM32WLxx_HAL_Driver/Inc -I../../CM4/Modules/GPS -I../../CM4/Modules/GPS/TinyGPSPlus -I../../CM4/Modules/INA -I../../CM4/Modules/IMU -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WL55xx -c -I../Core/Inc -I../LoRaWAN/App -I../LoRaWAN/Target -I../MbMux -I../../Common/System -I../../Common/MbMux -I../../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../../Utilities/trace/adv_trace -I../../Utilities/misc -I../../Utilities/timer -I../../Utilities/lpm/tiny_lpm -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../../Middlewares/Third_Party/LoRaWAN/Mac/Region -I../../Middlewares/Third_Party/LoRaWAN/Mac -I../../Middlewares/Third_Party/LoRaWAN/LmHandler -I../../Middlewares/Third_Party/LoRaWAN/Utilities -I../../Middlewares/Third_Party/SubGHz_Phy -I../../Drivers/CMSIS/Include -I../../Drivers/STM32WLxx_HAL_Driver -I../../Drivers/STM32WLxx_HAL_Driver/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/TinyGPS++.cyclo ./Core/Src/TinyGPS++.d ./Core/Src/TinyGPS++.o ./Core/Src/TinyGPS++.su ./Core/Src/adc_if.cyclo ./Core/Src/adc_if.d ./Core/Src/adc_if.o ./Core/Src/adc_if.su ./Core/Src/app_freertos.cyclo ./Core/Src/app_freertos.d ./Core/Src/app_freertos.o ./Core/Src/app_freertos.su ./Core/Src/cow.cyclo ./Core/Src/cow.d ./Core/Src/cow.o ./Core/Src/cow.su ./Core/Src/fence.cyclo ./Core/Src/fence.d ./Core/Src/fence.o ./Core/Src/fence.su ./Core/Src/flash_if.cyclo ./Core/Src/flash_if.d ./Core/Src/flash_if.o ./Core/Src/flash_if.su ./Core/Src/ina226.cyclo ./Core/Src/ina226.d ./Core/Src/ina226.o ./Core/Src/ina226.su ./Core/Src/ipcc_if.cyclo ./Core/Src/ipcc_if.d ./Core/Src/ipcc_if.o ./Core/Src/ipcc_if.su ./Core/Src/lsm6dso.cyclo ./Core/Src/lsm6dso.d ./Core/Src/lsm6dso.o ./Core/Src/lsm6dso.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/messages.cyclo ./Core/Src/messages.d ./Core/Src/messages.o ./Core/Src/messages.su ./Core/Src/sam_m10q.cyclo ./Core/Src/sam_m10q.d ./Core/Src/sam_m10q.o ./Core/Src/sam_m10q.su ./Core/Src/stm32_lpm_if.cyclo ./Core/Src/stm32_lpm_if.d ./Core/Src/stm32_lpm_if.o ./Core/Src/stm32_lpm_if.su ./Core/Src/stm32wlxx_hal_msp.cyclo ./Core/Src/stm32wlxx_hal_msp.d ./Core/Src/stm32wlxx_hal_msp.o ./Core/Src/stm32wlxx_hal_msp.su ./Core/Src/stm32wlxx_hal_timebase_tim.cyclo ./Core/Src/stm32wlxx_hal_timebase_tim.d ./Core/Src/stm32wlxx_hal_timebase_tim.o ./Core/Src/stm32wlxx_hal_timebase_tim.su ./Core/Src/stm32wlxx_it.cyclo ./Core/Src/stm32wlxx_it.d ./Core/Src/stm32wlxx_it.o ./Core/Src/stm32wlxx_it.su ./Core/Src/sys_app.cyclo ./Core/Src/sys_app.d ./Core/Src/sys_app.o ./Core/Src/sys_app.su ./Core/Src/sys_sensors.cyclo ./Core/Src/sys_sensors.d ./Core/Src/sys_sensors.o ./Core/Src/sys_sensors.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/timer_if.cyclo ./Core/Src/timer_if.d ./Core/Src/timer_if.o ./Core/Src/timer_if.su ./Core/Src/usart_if.cyclo ./Core/Src/usart_if.d ./Core/Src/usart_if.o ./Core/Src/usart_if.su

.PHONY: clean-Core-2f-Src

