set(srcs "main.c"
    "wifi.c"
    "ntp.c"
    "led_matrix.c"
)

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "./include")
