# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(core_tests)

set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/core)

target_sources(app PRIVATE
  src/main.c
  ${CORE_DIR}/state_machine.c
  ${CORE_DIR}/pid.c
  ${CORE_DIR}/mixer.c
  ${CORE_DIR}/input_mapper.c
  ${CORE_DIR}/battery.c
  ${CORE_DIR}/microblue_parser.c
  ${CORE_DIR}/calibration.c
)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
