idf_component_register(
    SRCS "app_main.c" "modbus_crc.c" "modbus_rtu.c" "register_map.c"
    INCLUDE_DIRS "."
    REQUIRES driver
)
