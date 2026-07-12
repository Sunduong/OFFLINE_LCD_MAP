set(app_src gps_driver.cpp)
set(pri_req driver)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "."
                       REQUIRES ${pri_req} TinyGPSPlus-ESPIDF)
