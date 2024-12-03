idf_component_register(SRCS "spi_display.c" "lvgl_interface.c"
                    INCLUDE_DIRS "." "../include"
                    REQUIRES esp_timer)