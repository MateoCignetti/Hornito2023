idf_component_register(SRCS "webserver.c" "wifi.c" "date_time.c" "main.c"
                    INCLUDE_DIRS "../include/"
                    EMBED_TXTFILES "view.html")