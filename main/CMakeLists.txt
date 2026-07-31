idf_component_register(SRCS
                            "main.cpp"
                            "camera.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES
                              esp32-camera
                              esp_driver_gpio
                              esp_driver_uart
                       PRIV_REQUIRES spi_flash)