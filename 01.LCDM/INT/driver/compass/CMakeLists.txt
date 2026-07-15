set(app_src compass_driver.c calibration/calibration.c)
set(pri_req driver)
idf_component_register(SRCS ${app_src}
                       INCLUDE_DIRS "." "calibration" "storage"
                       REQUIRES ${pri_req})
