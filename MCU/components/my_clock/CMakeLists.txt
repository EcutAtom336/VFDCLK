idf_component_register(
SRCS
"my_clock.c"
""
INCLUDE_DIRS
"."
PRIV_REQUIRES
VFD_8_MD_06INKM
my_event_group
my_buzzer
my_setting
my_sntp
common

esp_event
nvs_flash
json


REQUIRES
)
