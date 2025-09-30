################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpCompliance.c \
../Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpPackagesRegistration.c 

C_DEPS += \
./Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpCompliance.d \
./Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpPackagesRegistration.d 

OBJS += \
./Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpCompliance.o \
./Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpPackagesRegistration.o 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/%.o Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/%.su Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/%.cyclo: ../Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/%.c Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_NUCLEO_64 -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WL55xx -c -I../Core/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc -I../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../Drivers/BSP/STM32WLxx_Nucleo -I../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../Drivers/CMSIS/Include -I../LoRaWAN/App -I../LoRaWAN/Target -I../Utilities/trace/adv_trace -I../Utilities/misc -I../Utilities/timer -I../Utilities/lpm/tiny_lpm -I../Middlewares/Third_Party/LoRaWAN/LmHandler/Packages -I../Middlewares/Third_Party/LoRaWAN/Crypto -I../Middlewares/Third_Party/LoRaWAN/Mac/Region -I../Middlewares/Third_Party/LoRaWAN/Mac -I../Middlewares/Third_Party/LoRaWAN/LmHandler -I../Middlewares/Third_Party/LoRaWAN/Utilities -I../Middlewares/Third_Party/SubGHz_Phy -I../Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Middlewares-2f-Third_Party-2f-LoRaWAN-2f-LmHandler-2f-Packages

clean-Middlewares-2f-Third_Party-2f-LoRaWAN-2f-LmHandler-2f-Packages:
	-$(RM) ./Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpCompliance.cyclo ./Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpCompliance.d ./Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpCompliance.o ./Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpCompliance.su ./Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpPackagesRegistration.cyclo ./Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpPackagesRegistration.d ./Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpPackagesRegistration.o ./Middlewares/Third_Party/LoRaWAN/LmHandler/Packages/LmhpPackagesRegistration.su

.PHONY: clean-Middlewares-2f-Third_Party-2f-LoRaWAN-2f-LmHandler-2f-Packages

