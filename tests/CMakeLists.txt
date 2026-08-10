set(TEST_SUPPORT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test_stubs.c)

function(add_obs_unit_test TEST_NAME)
  add_executable(${TEST_NAME} ${ARGN})
  target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_link_libraries(${TEST_NAME} PRIVATE OBS::libobs)
  if(WIN32)
    target_link_libraries(${TEST_NAME} PRIVATE user32 gdi32)
  endif()
  if(MSVC)
    target_compile_options(${TEST_NAME} PRIVATE /Gy)
    target_link_options(${TEST_NAME} PRIVATE /OPT:REF)
  else()
    target_compile_options(${TEST_NAME} PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(${TEST_NAME} PRIVATE -Wl,--gc-sections)
  endif()
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
  if(WIN32)
    set_tests_properties(${TEST_NAME} PROPERTIES ENVIRONMENT "PATH=${OBS_TEST_BIN_DIR}")
  endif()
endfunction()

add_obs_unit_test(test_keyboard_layout
  test_keyboard_layout.c
  ${CMAKE_SOURCE_DIR}/src/keyboard-overlay-layout.c
)
add_obs_unit_test(test_keyboard_keys
  test_keyboard_keys.c
  ${CMAKE_SOURCE_DIR}/src/keyboard-overlay-keys.c
  ${TEST_SUPPORT_SOURCES}
)
add_obs_unit_test(test_mouse_trail
  test_mouse_trail.c
  ${CMAKE_SOURCE_DIR}/src/mouse-overlay-trail.c
  ${CMAKE_SOURCE_DIR}/src/mouse-overlay-cursor.c
  ${TEST_SUPPORT_SOURCES}
)
add_obs_unit_test(test_mouse_click
  test_mouse_click.c
  ${CMAKE_SOURCE_DIR}/src/mouse-overlay-click.c
  ${CMAKE_SOURCE_DIR}/src/mouse-overlay-cursor.c
  ${TEST_SUPPORT_SOURCES}
)
