#Description: IUART CMSIS Driver; user_visible: True
include_guard(GLOBAL)
message("driver_cmsis_iuart component is included.")

target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/fsl_uart_cmsis.c
)

target_include_directories(${MCUX_SDK_PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/.
)

#OR Logic component
if(${MCUX_DEVICE} STREQUAL "MIMX8MN5")
    include(driver_iuart_sdma)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MN4")
    include(driver_iuart_sdma)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MQ6")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MM6")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MM1")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MM5")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MM2")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8ML3")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MM3")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MQ7")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MN6")
    include(driver_iuart_sdma)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8ML6")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MN2")
    include(driver_iuart_sdma)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8ML4")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MD6")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MM4")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MN1")
    include(driver_iuart_sdma)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8ML8")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MN3")
    include(driver_iuart_sdma)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MD7")
    include(driver_iuart)
endif()
if(${MCUX_DEVICE} STREQUAL "MIMX8MQ5")
    include(driver_iuart)
endif()

include(CMSIS_Driver_Include_USART)