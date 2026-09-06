cmake_minimum_required(VERSION 3.16)
project(ggacgt_hh CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "-O2")
endif()

# REFACTORING_STRATEGY.md §8: core/operators/control/ga/io build into one static
# library shared by both binaries; app/main_*.cpp link against it plus their own
# observer/controller translation units.
add_library(ggacgt_core STATIC
  src/core/bpp_instance.cpp
  src/core/lower_bound.cpp
  src/core/population.cpp
  src/core/rng.cpp
  src/operators/adaptive_mutation.cpp
  src/operators/controlled_replacement.cpp
  src/operators/controlled_selection.cpp
  src/operators/ff_common.cpp
  src/operators/ff_initializer.cpp
  src/operators/gene_level_crossover.cpp
  src/operators/rearrangement_by_pairs.cpp
  src/ga/gga_cgt_engine.cpp
  src/io/config_loader.cpp
  src/io/generation_trace_logger.cpp
  src/io/instance_list_loader.cpp
  src/io/instance_loader.cpp
  src/io/paths.cpp
  src/io/solution_writer.cpp
  src/io/summary_writer.cpp
)
target_include_directories(ggacgt_core PUBLIC src)

add_executable(gga-cgt-hh src/app/main_base.cpp)
target_link_libraries(gga-cgt-hh PRIVATE ggacgt_core)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/app/main_informacion.cpp")
  # informacion-only pieces: the adaptive parameter controller, its two extra loggers,
  # and the two operators the informacion variant's crossover genuinely needs (RP_GLX
  # and the GLX crossover) that the base variant never uses -- kept out of
  # ggacgt_core so the base binary's link graph stays exactly what §2 describes.
  add_library(ggacgt_adaptive STATIC
    src/control/adaptive_parameter_controller.cpp
    src/io/adaptive_trace_logger.cpp
    src/io/adaptive_generation_trace_logger.cpp
    src/io/stagnation_dump_observer.cpp
    src/operators/gene_level_crossover_glx.cpp
    src/operators/rearrangement_by_pairs_glx.cpp
  )
  target_include_directories(ggacgt_adaptive PUBLIC src)
  target_link_libraries(ggacgt_adaptive PUBLIC ggacgt_core)

  add_executable(gga-cgt-info-hh src/app/main_informacion.cpp)
  target_link_libraries(gga-cgt-info-hh PRIVATE ggacgt_adaptive)
endif()

enable_testing()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()
