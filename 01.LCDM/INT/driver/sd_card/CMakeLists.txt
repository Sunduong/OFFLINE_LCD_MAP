set(app_src sd_card_driver.c)
set(pri_req driver)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES ${pri_req})
