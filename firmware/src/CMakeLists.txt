SET (SOURCE ${CMAKE_SOURCE_DIR}/src/main.c
                ${CMAKE_SOURCE_DIR}/lib/wifi_api.c
                ${CMAKE_SOURCE_DIR}/lib/http_json_helper.c)

SET (INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src
                 ${CMAKE_SOURCE_DIR}/lib/)

idf_component_register(
    SRCS ${SOURCE}
    INCLUDE_DIRS ${INCLUDE_DIR}
    PRIV_REQUIRES nvs_flash esp_netif esp_wifi esp_event
)
