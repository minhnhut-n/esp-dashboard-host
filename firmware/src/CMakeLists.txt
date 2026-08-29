SET (SOURCE ${CMAKE_SOURCE_DIR}/src/main.c
            ${CMAKE_SOURCE_DIR}/src/app_controller.c
            ${CMAKE_SOURCE_DIR}/lib/wifi/wifi_manager.c
            ${CMAKE_SOURCE_DIR}/lib/wifi/wifi_service.c
            ${CMAKE_SOURCE_DIR}/lib/wifi/wifi_handler.c
            ${CMAKE_SOURCE_DIR}/lib/http/http_handler.c
            ${CMAKE_SOURCE_DIR}/lib/http/http_service.c
            ${CMAKE_SOURCE_DIR}/lib/http/http_manager.c
            ${CMAKE_SOURCE_DIR}/lib/event_bus/event_bus.c
            ${CMAKE_SOURCE_DIR}/lib/storage_manager/storage_manager.c)

SET (INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src
                 ${CMAKE_SOURCE_DIR}/lib/http/
                 ${CMAKE_SOURCE_DIR}/lib/event_bus/
                 ${CMAKE_SOURCE_DIR}/lib/wifi/
                 ${CMAKE_SOURCE_DIR}/lib/storage_manager/
                 ${CMAKE_SOURCE_DIR}/lib/helper/)

idf_component_register(
    SRCS ${SOURCE}
    INCLUDE_DIRS ${INCLUDE_DIR}
    PRIV_REQUIRES nvs_flash esp_netif esp_wifi esp_event esp_http_server esp_timer log
)
