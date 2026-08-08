SET (SOURCE ${CMAKE_SOURCE_DIR}/src/main.c
    )  

SET (INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src
                 ${CMAKE_SOURCE_DIR}/lib/)

idf_component_register(
    SRCS ${SOURCE}
    INCLUDE_DIRS ${INCLUDE_DIR}
    PRIV_REQUIRES nvs_flash esp_netif esp_wifi esp_event
)
