idf_component_register(
SRCS
"VFD_8_MD_06INKM.c"
""
INCLUDE_DIRS
"."
PRIV_REQUIRES
common

driver
nvs_flash
esp_adc


REQUIRES
json
)
