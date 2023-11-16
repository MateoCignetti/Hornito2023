idf_component_register(SRCS "peltier.c" "power.c" "webserver.c" "date_time.c" "control_system.c" "ntc.c" "tasks.c" "dimmer.c" "adc.c" "main.c" "wifi.c"
                    INCLUDE_DIRS "../include"
                    EMBED_TXTFILES "view.html")