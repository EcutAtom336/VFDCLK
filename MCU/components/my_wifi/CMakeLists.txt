idf_component_register(
SRCS
"my_wifi.c"
""
INCLUDE_DIRS
"."
PRIV_REQUIRES
my_event_group
my_setting
common

esp_wifi
nvs_flash


REQUIRES
json
)
