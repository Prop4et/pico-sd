add_executable(bmerawdata
    main.c
    hw_config.c
)

target_include_directories(bmerawdata PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Tell CMake where to find other source code
SET_TARGET_PROPERTIES(bsec PROPERTIES LINKER_LANGUAGE C)

add_library(algobsec STATIC IMPORTED)
set_property(TARGET algobsec PROPERTY IMPORTED_LOCATION ${ALGOBSEC_PATH}/libalgobsec.a) 

target_link_libraries(bmerawdata 
    bme68x
    bme_api
    bsec
    FatFs_SPI
    pico_stdlib
    hardware_i2c
    algobsec
)


#enable usb output
pico_enable_stdio_usb(bmerawdata 1)
pico_enable_stdio_uart(bmerawdata 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(bmerawdata)

