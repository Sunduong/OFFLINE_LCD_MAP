idf_component_register(SRCS "app_main.c"
                       INCLUDE_DIRS "."
                       REQUIRES spi_bus lcd sd_card)
