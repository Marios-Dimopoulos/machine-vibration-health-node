# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
# Load Zephyr's build system: board, device tree, Kconfig, toolchain.
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(vibration-m33)

target_sources(app PRIVATE src/main.c)
