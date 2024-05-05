idf_component_register(
SRCS
"my_setting.c"
""
INCLUDE_DIRS
"."
PRIV_REQUIRES
my_bt
my_clock
my_wifi
VFD_8_MD_06INKM
my_common
life_cycle_manage

esp_hw_support
json
esp_wifi
nvs_flash
driver
espressif__button

REQUIRES
)
