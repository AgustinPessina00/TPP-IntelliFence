################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/LoRaWAN/LmHandler/LmHandler.c \
../Middlewares/Third_Party/LoRaWAN/LmHandler/NvmDataMgmt.c 

C_DEPS += \
./Middlewares/Third_Party/LoRaWAN/LmHandler/LmHandler.d \
./Middlewares/Third_Party/LoRaWAN/LmHandler/NvmDataMgmt.d 

OBJS += \
./Middlewares/Third_Party/LoRaWAN/LmHandler/LmHandler.o \
./Middlewares/Third_Party/LoRaWAN/LmHandler/NvmDataMgmt.o 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/LoRaWAN/LmHandler/%.o Middlewares/Third_Party/LoRaWAN/LmHandler/%.su Middlewares/Third_Party/LoRaWAN/LmHandler/%.cyclo: ../Middlewares/Third_Party/LoRaWAN/LmHandler/%.c Middlewares/Third_Party/LoRaWAN/LmHandler/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_NUCLEO_64 -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WL55xx -c -I../Core/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../Drivers/BSP/STM32WLxx_Nucleo -I../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../Drivers/CMSIS/Include -I../LoRaWAN/App -I../LoRaWAN/Target -I../Utilities/trace/adv_trace -I../Utilities/misc -I../Utilities/timer -I../Utilities/lpm/tiny_lpm -I../Middlewares/Third_Party/LoRaWAN/LmHandler/Packages -I../Middlewares/Third_Party/LoRaWAN/Crypto -I../Middlewares/Third_Party/LoRaWAN/Mac/Region -I../Middlewares/Third_Party/LoRaWAN/Mac -I../Middlewares/Third_Party/LoRaWAN/LmHandler -I../Middlewares/Third_Party/LoRaWAN/Utilities -I../Middlewares/Third_Party/SubGHz_Phy -I../Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Middlewares-2f-Third_Party-2f-LoRaWAN-2f-LmHandler

clean-Middlewares-2f-Third_Party-2f-LoRaWAN-2f-LmHandler:
	-$(RM) ./Middlewares/Third_Party/LoRaWAN/LmHandler/LmHandler.cyclo ./Middlewares/Third_Party/LoRaWAN/LmHandler/LmHandler.d ./Middlewares/Third_Party/LoRaWAN/LmHandler/LmHandler.o ./Middlewares/Third_Party/LoRaWAN/LmHandler/LmHandler.su ./Middlewares/Third_Party/LoRaWAN/LmHandler/NvmDataMgmt.cyclo ./Middlewares/Third_Party/LoRaWAN/LmHandler/NvmDataMgmt.d ./Middlewares/Third_Party/LoRaWAN/LmHandler/NvmDataMgmt.o ./Middlewares/Third_Party/LoRaWAN/LmHandler/NvmDataMgmt.su

.PHONY: clean-Middlewares-2f-Third_Party-2f-LoRaWAN-2f-LmHandler

