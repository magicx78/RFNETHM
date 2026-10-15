set(RFNETHM_SOURCES
    "main.c"
    "hmu_frame.c"
    "bridge.c"
    "source_uart.c"
    "sink_debug.c"
    "sink_tcp.c"
    "sink_hbrfeth.c"
    "sink_hmuartlgw_legacy.c"
    "log_buffer.c"
    "webui.c"
    "mdns_glue.c"
    "ota_check.c")

if(IDF_TARGET STREQUAL "esp32p4")
    list(APPEND RFNETHM_SOURCES "net_ethernet.c" "source_usb_stub.c")
    set(RFNETHM_REQUIRES driver nvs_flash esp_event esp_netif lwip esp_eth
                         esp_http_server esp_http_client esp-tls app_update esp_app_format espcoredump
                         esp_timer json mdns spi_flash)
    if(DEFINED IDF_VERSION_MAJOR AND IDF_VERSION_MAJOR GREATER_EQUAL 6)
        list(APPEND RFNETHM_REQUIRES ip101)
    endif()
else()
    list(APPEND RFNETHM_SOURCES "source_usb.c" "net.c" "improv_glue.cpp")
    set(RFNETHM_REQUIRES usb driver usb_host_cdc_acm nvs_flash esp_wifi esp_event esp_netif lwip
                         esp_http_server esp_http_client esp-tls app_update esp_app_format espcoredump
                         esp_timer json improv_wifi_busware mdns spi_flash)
endif()

idf_component_register(
    SRCS        ${RFNETHM_SOURCES}
    INCLUDE_DIRS "."
    REQUIRES     ${RFNETHM_REQUIRES}
    EMBED_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/../web/index.html.gz"
        "${CMAKE_CURRENT_SOURCE_DIR}/../web/app.css.gz"
        "${CMAKE_CURRENT_SOURCE_DIR}/../web/app.js.gz"
        "${CMAKE_CURRENT_SOURCE_DIR}/../web/assets/busmatic.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/../web/assets/busmatic_white.png")

if(IDF_TARGET STREQUAL "esp32p4")
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        RFNETHM_UART_TX_GPIO=37
        RFNETHM_UART_RX_GPIO=6
        RFNETHM_RPI_RESET_GPIO=53)
endif()
