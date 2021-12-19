cmake_minimum_required(VERSION 3.10)
project(betaflight)

set(CMAKE_CXX_STANDARD 14)

add_definitions(-D__TARGET__=STM32F745)
add_definitions(-D__REVISION__=norevision)
add_definitions(-DSTM32F7=1)
add_definitions(-DSTM32F745=1)
add_definitions(-DSTM32F745xx=1)
add_definitions(-DTARGET_FLASH_SIZE=2048)

include_directories(src/main)

file(GLOB_RECURSE SOURCES src/main *.c *.h)
list(FILTER SOURCES EXCLUDE REGEX "src/main/target" )

file(GLOB_RECURSE SRC_TARGET src/main/target/STM32_UNIFIED *.c *.h)
include_directories(src/main/target/STM32_UNIFIED)

include_directories(lib/main/STM32F7/Drivers/CMSIS/Device/ST/STM32F7xx/Include)
include_directories(lib/main/STM32F7/Drivers/STM32F7xx_HAL_Driver/Inc)

add_executable(betaflight ${SOURCES} ${SRC_TARGET})