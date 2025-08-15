################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Core/Src/threads/dispatcherTask.cpp \
../Core/Src/threads/distanceTask.cpp \
../Core/Src/threads/fenceUpdateTask.cpp \
../Core/Src/threads/fsmTask.cpp \
../Core/Src/threads/gpsTask.cpp \
../Core/Src/threads/loraRxTask.cpp \
../Core/Src/threads/loraTxTask.cpp \
../Core/Src/threads/sensorAcqTask.cpp \
../Core/Src/threads/stimulusTask.cpp 

OBJS += \
./Core/Src/threads/dispatcherTask.o \
./Core/Src/threads/distanceTask.o \
./Core/Src/threads/fenceUpdateTask.o \
./Core/Src/threads/fsmTask.o \
./Core/Src/threads/gpsTask.o \
./Core/Src/threads/loraRxTask.o \
./Core/Src/threads/loraTxTask.o \
./Core/Src/threads/sensorAcqTask.o \
./Core/Src/threads/stimulusTask.o 

CPP_DEPS += \
./Core/Src/threads/dispatcherTask.d \
./Core/Src/threads/distanceTask.d \
./Core/Src/threads/fenceUpdateTask.d \
./Core/Src/threads/fsmTask.d \
./Core/Src/threads/gpsTask.d \
./Core/Src/threads/loraRxTask.d \
./Core/Src/threads/loraTxTask.d \
./Core/Src/threads/sensorAcqTask.d \
./Core/Src/threads/stimulusTask.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/threads/%.o Core/Src/threads/%.su Core/Src/threads/%.cyclo: ../Core/Src/threads/%.cpp Core/Src/threads/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m4 -std=gnu++14 -g3 -DDEBUG -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WL55xx -c -I../Core/Inc -I../LoRaWAN/App -I../LoRaWAN/Target -I../MbMux -I../../Common/System -I../../Modules -I../../Common/MbMux -I../../Drivers/STM32WLxx_HAL_Driver/Inc/Legacy -I../../Utilities/trace/adv_trace -I../../Utilities/misc -I../../Utilities/timer -I../../Utilities/lpm/tiny_lpm -I../../Middlewares/Third_Party/FreeRTOS/Source/include -I../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../../Middlewares/Third_Party/LoRaWAN/Mac/Region -I../../Middlewares/Third_Party/LoRaWAN/Mac -I../../Middlewares/Third_Party/LoRaWAN/LmHandler -I../../Middlewares/Third_Party/LoRaWAN/Utilities -I../../Middlewares/Third_Party/SubGHz_Phy -I../../Drivers/CMSIS/Include -I../../Drivers/STM32WLxx_HAL_Driver -I/TPP-IntelliFence/Drivers/STM32WLxx_HAL_Driver/Inc -I../../Drivers/STM32WLxx_HAL_Driver/Inc -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-threads

clean-Core-2f-Src-2f-threads:
	-$(RM) ./Core/Src/threads/dispatcherTask.cyclo ./Core/Src/threads/dispatcherTask.d ./Core/Src/threads/dispatcherTask.o ./Core/Src/threads/dispatcherTask.su ./Core/Src/threads/distanceTask.cyclo ./Core/Src/threads/distanceTask.d ./Core/Src/threads/distanceTask.o ./Core/Src/threads/distanceTask.su ./Core/Src/threads/fenceUpdateTask.cyclo ./Core/Src/threads/fenceUpdateTask.d ./Core/Src/threads/fenceUpdateTask.o ./Core/Src/threads/fenceUpdateTask.su ./Core/Src/threads/fsmTask.cyclo ./Core/Src/threads/fsmTask.d ./Core/Src/threads/fsmTask.o ./Core/Src/threads/fsmTask.su ./Core/Src/threads/gpsTask.cyclo ./Core/Src/threads/gpsTask.d ./Core/Src/threads/gpsTask.o ./Core/Src/threads/gpsTask.su ./Core/Src/threads/loraRxTask.cyclo ./Core/Src/threads/loraRxTask.d ./Core/Src/threads/loraRxTask.o ./Core/Src/threads/loraRxTask.su ./Core/Src/threads/loraTxTask.cyclo ./Core/Src/threads/loraTxTask.d ./Core/Src/threads/loraTxTask.o ./Core/Src/threads/loraTxTask.su ./Core/Src/threads/sensorAcqTask.cyclo ./Core/Src/threads/sensorAcqTask.d ./Core/Src/threads/sensorAcqTask.o ./Core/Src/threads/sensorAcqTask.su ./Core/Src/threads/stimulusTask.cyclo ./Core/Src/threads/stimulusTask.d ./Core/Src/threads/stimulusTask.o ./Core/Src/threads/stimulusTask.su

.PHONY: clean-Core-2f-Src-2f-threads

