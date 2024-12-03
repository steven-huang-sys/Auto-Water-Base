SET(SOURCES screens/ui_Screen1.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c)

# add_library(ui ${SOURCES})

idf_component_register(SRCS ${SOURCES}
                INCLUDE_DIRS "." "../include"
                REQUIRES esp_timer)