cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

set(PROJECT "picomouse")
project(${PROJECT} C CXX ASM)

pico_sdk_init()

add_executable(${PROJECT}
	main.c
)

target_include_directories(${PROJECT} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

pico_enable_stdio_usb(${PROJECT} 0)
pico_enable_stdio_uart(${PROJECT} 0)
pico_add_extra_outputs(${PROJECT})

target_link_libraries(${PROJECT}
	pico_stdlib
	tinyusb_host
)
