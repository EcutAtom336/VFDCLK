idf_component_register(
SRCS
"my_clock.c"
""
INCLUDE_DIRS
"."
PRIV_REQUIRES
VFD_8_MD_06INKM
my_buzzer
my_sntp
my_common
life_cycle_manage

esp_event
nvs_flash
espressif__button


REQUIRES
json
)
