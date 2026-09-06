add_executable(test_lower_bound unit/test_lower_bound.cpp)
target_link_libraries(test_lower_bound PRIVATE ggacgt_core)
add_test(NAME lower_bound COMMAND test_lower_bound)

add_executable(test_operators unit/test_operators.cpp)
target_link_libraries(test_operators PRIVATE ggacgt_core)
add_test(NAME operators COMMAND test_operators)

add_executable(test_schedules unit/test_schedules.cpp)
target_link_libraries(test_schedules PRIVATE ggacgt_core)
add_test(NAME schedules COMMAND test_schedules)

if(TARGET ggacgt_adaptive)
  add_executable(test_adaptive_parameter_controller unit/test_adaptive_parameter_controller.cpp)
  target_link_libraries(test_adaptive_parameter_controller PRIVATE ggacgt_adaptive)
  add_test(NAME adaptive_parameter_controller COMMAND test_adaptive_parameter_controller)
endif()
