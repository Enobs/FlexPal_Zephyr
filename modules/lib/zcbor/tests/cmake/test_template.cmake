FILE(GLOB app_sources src/*.c)
FILE(GLOB app_sources_cpp src/*.cpp)
target_sources(app PRIVATE
  ${app_sources} ${app_sources_cpp}
  )

if (VERBOSE)
  zephyr_compile_definitions(ZCBOR_VERBOSE)
endif()

if (ASSERTS)
  zephyr_compile_definitions(ZCBOR_ASSERTS)
endif()

if (CANONICAL)
  zephyr_compile_definitions(ZCBOR_CANONICAL)
endif()

if (CONFIG_BIG_ENDIAN OR BIG_ENDIAN)
  zephyr_compile_definitions(ZCBOR_BIG_ENDIAN)
endif()

zephyr_compile_options(-Werror)

if (CONFIG_64BIT)
  set(bit_arg -b 64)
endif()
