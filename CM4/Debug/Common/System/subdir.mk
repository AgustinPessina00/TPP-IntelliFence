################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
C:/Users/nacho/OneDrive/Documentos/FIUBA/TPP/SW/TPP-IntelliFence/Common/System/sys_debug.cpp 

C_SRCS += \
C:/Users/nacho/OneDrive/Documentos/FIUBA/TPP/SW/TPP-IntelliFence/Common/System/system_stm32wlxx.c 

C_DEPS += \
./Common/System/system_stm32wlxx.d 

OBJS += \
./Common/System/sys_debug.o \
./Common/System/system_stm32wlxx.o 

CPP_DEPS += \
./Common/System/sys_debug.d 


# Each subdirectory must supply rules for building sources it contributes
Common/System/sys_debug.o: C:/Users/nacho/OneDrive/Documentos/FIUBA/TPP/SW/TPP-IntelliFence/Common/System/sys_debug.cpp Common/System/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WL55xx -c -I../Core/Inc -I../LoRaWAN/App -I../LoRaWAN/Target -I../MbMux -I../../Common/System -I../../Common/MbMux -I../../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../../Utilities/trace/adv_trace -I../../Utilities/misc -I../../Utilities/timer -I../../Utilities/lpm/tiny_lpm -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../../Middlewares/Third_Party/LoRaWAN/Mac/Region -I../../Middlewares/Third_Party/LoRaWAN/Mac -I../../Middlewares/Third_Party/LoRaWAN/LmHandler -I../../Middlewares/Third_Party/LoRaWAN/Utilities -I../../Middlewares/Third_Party/SubGHz_Phy -I../../Drivers/CMSIS/Include -I../../Drivers/STM32WLxx_HAL_Driver -I/TPP-IntelliFence/Drivers/STM32WLxx_HAL_Driver/Inc -I../../Drivers/STM32WLxx_HAL_Driver/Inc -I../../CM4/Modules -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Common/System/system_stm32wlxx.o: C:/Users/nacho/OneDrive/Documentos/FIUBA/TPP/SW/TPP-IntelliFence/Common/System/system_stm32wlxx.c Common/System/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WL55xx -c -I../Core/Inc -I../LoRaWAN/App -I../LoRaWAN/Target -I../MbMux -I../../Common/System -I../../Modules -I../../Common/MbMux -I../../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../../Utilities/trace/adv_trace -I../../Utilities/misc -I../../Utilities/timer -I../../Utilities/lpm/tiny_lpm -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../../Middlewares/Third_Party/LoRaWAN/Mac/Region -I../../Middlewares/Third_Party/LoRaWAN/Mac -I../../Middlewares/Third_Party/LoRaWAN/LmHandler -I../../Middlewares/Third_Party/LoRaWAN/Utilities -I../../Middlewares/Third_Party/SubGHz_Phy -I../../Drivers/CMSIS/Include -I../../Drivers/STM32WLxx_HAL_Driver -I../../../Drivers/STM32WLxx_HAL_Driver/Inc -I"C:/Users/nacho/OneDrive/Documentos/FIUBA/TPP/SW/TPP-IntelliFence/Drivers/STM32WLxx_HAL_Driver/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Common-2f-System

clean-Common-2f-System:
	-$(RM) ./Common/System/sys_debug.cyclo ./Common/System/sys_debug.d ./Common/System/sys_debug.o ./Common/System/sys_debug.su ./Common/System/system_stm32wlxx.cyclo ./Common/System/system_stm32wlxx.d ./Common/System/system_stm32wlxx.o ./Common/System/system_stm32wlxx.su

.PHONY: clean-Common-2f-System

