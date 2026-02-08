cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Compiler options
set(STM32_MCU_FLAGS  "-mcpu=cortex-m4 " )

#Linker options
set(STM32_LINKER_SCRIPT STM32WL55XX_FLASH_CM4.ld)
set(STM32_LINKER_OPTION  )

# Set the project name
set(CMAKE_PROJECT_NAME TPP-Intellifence_CM4)
project(${CMAKE_PROJECT_NAME})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Include mx-generated
include("mx-generated.cmake")


# In order to use CMake for cross-compiling
set(CMAKE_CXX_COMPILER_FORCED "true")

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# =============================================================================
# TEST MODE CONFIGURATION
# =============================================================================
# Option to enable test mode with mock sensors
option(ENABLE_TEST_MODE "Enable test mode with mock sensor data" OFF)

if(ENABLE_TEST_MODE)
    message(STATUS "TEST MODE ENABLED - Using mock sensor data")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ENABLE_TEST_MODE)
else()
    message(STATUS "PRODUCTION MODE - Using real sensors")
endif()

# Enable CMake support for ASM, C and C++ languages
enable_language(C CXX ASM)

# Setup C++ compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/Modules/GPS
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/Modules/I2C
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/Modules/UART
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/Modules/INA
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/Modules/IMU
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/Modules/Messages
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/Modules/Buzzer
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/Threads
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/Types
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/Test
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    # Embedded Message System Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/Messages/EmbeddedMessage.c
    #${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/Messages/MessageTest.c
    # RTOS Printf (Thread-Safe Logging)
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/rtos_printf.c
    # GPS Module Sources (Original)
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/GPS/sam_m10q.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/GPS/sam_m10q_KEYID.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/GPS/gps_test_wrapper.cpp

    # I2C Thread-Safe Sources  
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/I2C/I2CBus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/I2C/I2CManager.cpp
    # UART Thread-Safe Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/UART/UARTBus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/UART/UARTManager.cpp
    # INA226 Current/Power Sensor Sources
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/INA/ina226.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/INA/ina226_test_wrapper.cpp
    # LSM6DSO IMU Sensor Sources (Thread-Safe)
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/IMU/lsm6dso.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/IMU/lsm6dso_test_wrapper.cpp
    # Buzzer Module Sources (PWM Control)
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/Buzzer/buzzer.c
    #${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/Buzzer/buzzer_example.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/Buzzer/buzzer_alarm.c
    #${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Modules/Buzzer/buzzer_alarm_example.c
    # Data Model Sources (Cow, Fence, and Zone definitions)
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Types/cow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Types/fence.cpp
    # Test Sources (Validation and Unit Tests)
    #${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Types/cow_fence_test.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Types/fence_test.cpp
    # Helper Functions
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/getZone.cpp
    # Thread Sources (FreeRTOS Tasks)
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Threads/dispatcherTask.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Threads/fsmTask.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Threads/fsm_helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Threads/startupRoutineFsm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Threads/normalOperationFsm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Threads/fenceTransitionFsm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Threads/sensorAcqTask.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Threads/stimulusTask.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Threads/loraTask.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/system_init.cpp

    # Test Sources (Mock Sensors for FSM Testing)
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Test/test_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Test/test_sensor_mock.cpp
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined libraries
)

# Enable printf/scanf with floating point support
# DISABLED to save ROM space (~8-10KB)
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
   -u_printf_float
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()

# Debug configuration - Aggressive size optimization
# -Os: Optimize for size 
# -flto: Link Time Optimization (further size reduction)
# -g1: Minimal debug info (saves ROM)
# -ffunction-sections -fdata-sections: Enable dead code elimination
set(CMAKE_C_FLAGS_DEBUG "-Os -flto -g3 -ffunction-sections -fdata-sections" CACHE STRING "C Debug flags" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-Os -flto -g3 -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions" CACHE STRING "C++ Debug flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-Os -flto -g3 -Wl,--gc-sections" CACHE STRING "Linker Debug flags" FORCE)









