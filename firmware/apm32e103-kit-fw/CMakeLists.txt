cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tools")
include(arm-none-eabi-gcc)


# 프로젝트 이름은 최종 출력 파일 이름으로 사용된다.
#
set(PRJ_NAME apm32e103-kit-fw)


project(${PRJ_NAME}
  LANGUAGES ASM C CXX
)

set(EXECUTABLE ${PRJ_NAME}.elf)


# 지정한 폴더에 있는 파일만 포함한다.
#
file(GLOB SRC_FILES CONFIGURE_DEPENDS
  src/*.cpp
  src/*.c
  src/bsp/*.c
  src/bsp/device/*.c
  src/bsp/startup/*.s 

  # FatFs
  src/lib/FatFs/src/*.c
  src/lib/FatFs/src/option/syscall.c
  src/lib/FatFs/src/option/unicode.c    

  # USB
  src/lib/geehy/APM32_USB_Library/Device/Core/Src/*.c  
)

# 하위폴더에 있는 파일까지 포함한다.
#
file(GLOB_RECURSE SRC_FILES_RECURSE CONFIGURE_DEPENDS
  src/ap/*.c
  src/bsp/*.s
  src/common/*.c
  src/hw/*.c
  src/lib/geehy/APM32E10x_StdPeriphDriver/src/*.c
)

# 특정 폴더를 빌드에서 제외한다.
#
set(EXCLUDE_PATHS 
)

foreach(TMP_PATH ${SRC_FILES_RECURSE}) 
  foreach(EXCLUDE_PATH ${EXCLUDE_PATHS}) 
    string(FIND ${TMP_PATH} ${EXCLUDE_PATH} RESULT) 
    if(NOT ${RESULT} EQUAL -1) 
      list(REMOVE_ITEM SRC_FILES_RECURSE ${TMP_PATH}) 
    endif() 
  endforeach(EXCLUDE_PATH) 
endforeach(TMP_PATH)


add_executable(${EXECUTABLE} 
  ${SRC_FILES}
  ${SRC_FILES_RECURSE}
)


target_include_directories(${EXECUTABLE} PRIVATE 
  src  
  src/ap
  src/ap/thread
  src/bsp
  src/bsp/device
  src/common 
  src/common/core
  src/common/hw/include 
  src/hw
  src/lib

  src/lib/geehy/CMSIS/Include
  src/lib/geehy/CMSIS/Device/Geehy/APM32E10x/Include
  src/lib/geehy/APM32E10x_StdPeriphDriver/inc

  # FatFs
  src/hw/driver/fatfs
  src/lib/FatFs/src  

  # USB
  src/hw/driver/usb
  src/hw/driver/usb/usb_cdc
  src/hw/driver/usb/usb_msc
  src/lib/geehy/APM32_USB_Library/Device/Core/Inc
)

target_compile_definitions(${EXECUTABLE} PRIVATE
  -DUSB_DEVICE
  )

target_compile_options(${EXECUTABLE} PRIVATE
  -mcpu=cortex-m3
  -mthumb
  # -mfpu=vfpv4-d16
  # -mfloat-abi=hard


  -fdata-sections
  -ffunction-sections
  # -Wstack-usage=2048

  -Wall
  -g3
  -Og
  )

target_link_options(${EXECUTABLE} PRIVATE
  -T../src/bsp/ldscript/APM32E103RE_FLASH.ld
  -mcpu=cortex-m3
  -mthumb
  # -mfpu=vfpv4-d16
  # -mfloat-abi=hard
  -specs=nano.specs
  -lc
  -lm
  -lstdc++
  -lsupc++
  # -lnosys

  # -u _printf_float
  
  -Wl,-Map=${PRJ_NAME}.map,--cref
  -Wl,--gc-sections
  -Xlinker -print-memory-usage -Xlinker
  )

set (CMAKE_CXX_FLAGS "-fno-rtti")

# target_link_libraries( ${EXECUTABLE} PRIVATE
#   ${CMAKE_SOURCE_DIR}/src/lib/ST/CMSIS/DSP-LIB/libCMSISDSPCommon.a 
#   ${CMAKE_SOURCE_DIR}/src/lib/ST/CMSIS/DSP-LIB/libCMSISDSPTransform.a 
#   )

add_custom_command(TARGET ${EXECUTABLE} 
  POST_BUILD
  COMMAND ${CMAKE_SIZE_UTIL}  ${EXECUTABLE} --format=berkeley 
  )  

add_custom_command(TARGET ${EXECUTABLE} 
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} ARGS -O binary ${EXECUTABLE} ${PROJECT_NAME}.bin
  COMMENT "Invoking: Make Binary"
  )  