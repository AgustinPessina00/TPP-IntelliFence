################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Core/Src/threads/dispatcherTask.cpp \
../Core/Src/threads/fsmTask.cpp \
../Core/Src/threads/sensorAcqTask.cpp 

OBJS += \
./Core/Src/threads/dispatcherTask.o \
./Core/Src/threads/fsmTask.o \
./Core/Src/threads/sensorAcqTask.o 

CPP_DEPS += \
./Core/Src/threads/dispatcherTask.d \
./Core/Src/threads/fsmTask.d \
./Core/Src/threads/sensorAcqTask.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/threads/%.o Core/Src/threads/%.su Core/Src/threads/%.cyclo: ../Core/Src/threads/%.cpp Core/Src/threads/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WL55xx -c -I../Core/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../Drivers/CMSIS/Include -I../LoRaWAN/App -I../LoRaWAN/Target -I../Utilities/trace/adv_trace -I../Utilities/misc -I../Utilities/timer -I../Utilities/lpm/tiny_lpm -I../Middlewares/Third_Party/LoRaWAN/LmHandler/Packages -I../Middlewares/Third_Party/LoRaWAN/Crypto -I../Middlewares/Third_Party/LoRaWAN/Mac/Region -I../Middlewares/Third_Party/LoRaWAN/Mac -I../Middlewares/Third_Party/LoRaWAN/LmHandler -I../Middlewares/Third_Party/LoRaWAN/Utilities -I../Middlewares/Third_Party/SubGHz_Phy -I../Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver -Os -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-threads

clean-Core-2f-Src-2f-threads:
	-$(RM) ./Core/Src/threads/dispatcherTask.cyclo ./Core/Src/threads/dispatcherTask.d ./Core/Src/threads/dispatcherTask.o ./Core/Src/threads/dispatcherTask.su ./Core/Src/threads/fsmTask.cyclo ./Core/Src/threads/fsmTask.d ./Core/Src/threads/fsmTask.o ./Core/Src/threads/fsmTask.su ./Core/Src/threads/sensorAcqTask.cyclo ./Core/Src/threads/sensorAcqTask.d ./Core/Src/threads/sensorAcqTask.o ./Core/Src/threads/sensorAcqTask.su

.PHONY: clean-Core-2f-Src-2f-threads

