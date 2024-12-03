idf_component_register(SRCS "udp_client.c" "../http_client/http_client.c"
                    INCLUDE_DIRS "." "../include" "../http_client"
                    REQUIRES esp_wifi json)