add_library(json
    read_json.h
    read_json.c
)

target_link_libraries(json
    pico_stdlib
    FatFs_SPI
)