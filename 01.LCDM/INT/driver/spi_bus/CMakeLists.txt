set(app_src spi_bus_manager.c)
set(pri_req driver board)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES ${pri_req})
