if(CONFIG_VEGA_SDK_HAL)
  zephyr_library()
endif()

add_subdirectory_ifdef(
  CONFIG_VEGA_SDK_HAL
  vega_sdk_riscv
  )
