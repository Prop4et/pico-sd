add_executable(only_sd
    main.c
    hw_config.c
)

target_include_directories(only_sd PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Tell CMake where to find other source code

target_link_libraries(only_sd 
    #bme68x
    #bme_api
    #bsec
    FatFs_SPI
    pico_stdlib
)


#enable usb output
pico_enable_stdio_usb(only_sd 1)
pico_enable_stdio_uart(only_sd 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(only_sd)

