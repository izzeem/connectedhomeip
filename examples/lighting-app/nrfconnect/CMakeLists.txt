#
#    Copyright (c) 2020 Project CHIP Authors
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#
cmake_minimum_required(VERSION 3.13.1)

get_filename_component(CHIP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/connectedhomeip REALPATH)
get_filename_component(NRFCONNECT_COMMON ${CHIP_ROOT}/examples/platform/nrfconnect REALPATH)
get_filename_component(LIGHTING_COMMON ${CHIP_ROOT}/examples/lighting-app/lighting-common REALPATH)
get_filename_component(GEN_DIR ${CHIP_ROOT}/zzz_generated/ REALPATH)

include(${CHIP_ROOT}/config/nrfconnect/app/check-nrfconnect-version.cmake)

# Set Kconfig root files that will be processed as a first Kconfig for used child images.
set(mcuboot_KCONFIG_ROOT ${CHIP_ROOT}/config/nrfconnect/chip-module/Kconfig.mcuboot.root)
set(multiprotocol_rpmsg_KCONFIG_ROOT ${CHIP_ROOT}/config/nrfconnect/chip-module/Kconfig.multiprotocol_rpmsg.root)

if(NOT CONF_FILE STREQUAL "prj_no_dfu.conf" AND NOT BOARD STREQUAL "nrf52840dongle_nrf52840")
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static_dfu.yml)
endif()

list(APPEND ZEPHYR_EXTRA_MODULES ${CHIP_ROOT}/config/nrfconnect/chip-module)
find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(chip-nrfconnect-lighting-example)

include(${CHIP_ROOT}/config/nrfconnect/app/enable-gnu-std.cmake)
include(${CHIP_ROOT}/config/nrfconnect/app/flashing.cmake)
include(${CHIP_ROOT}/src/app/chip_data_model.cmake)

# -Wmaybe-uninitialized has too many false positives, including on std::optional
# and chip::Optional.  Make it nonfatal.
#
# See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80635
target_compile_options(app PRIVATE -Werror -Wno-error=maybe-uninitialized)

target_include_directories(app PRIVATE
                           main/include
                           ${LIGHTING_COMMON}
                           ${GEN_DIR}
                           ${GEN_DIR}/app-common
                           ${GEN_DIR}/lighting-app
                           ${NRFCONNECT_COMMON}/util/include
                           ${NRFCONNECT_COMMON}/app/include)

target_sources(app PRIVATE
               main/AppTask.cpp
               main/main.cpp
               main/ZclCallbacks.cpp
               ${GEN_DIR}/lighting-app/zap-generated/callback-stub.cpp
               ${GEN_DIR}/lighting-app/zap-generated/IMClusterCommandHandler.cpp
               ${NRFCONNECT_COMMON}/util/LEDWidget.cpp
               ${NRFCONNECT_COMMON}/util/PWMDevice.cpp
               ${NRFCONNECT_COMMON}/util/ThreadUtil.cpp)

chip_configure_data_model(app
    INCLUDE_SERVER
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../lighting-common/lighting-app.zap
    GEN_DIR ${GEN_DIR}/lighting-app/zap-generated
)

if(CONFIG_CHIP_OTA_REQUESTOR)
    target_sources(app PRIVATE ${NRFCONNECT_COMMON}/util/OTAUtil.cpp)
endif()

if(CONFIG_MCUMGR_SMP_BT)
    target_sources(app PRIVATE ${NRFCONNECT_COMMON}/util/DFUOverSMP.cpp)
endif()

if (CONFIG_SHELL AND BOARD STREQUAL "nrf52840dongle_nrf52840")
  target_sources(app PRIVATE ${NRFCONNECT_COMMON}/util/DFUTrigger.cpp)
endif()

if (CONFIG_CHIP_PW_RPC)

# Make all targets created below depend on zephyr_interface to inherit MCU-related compilation flags
link_libraries($<BUILD_INTERFACE:zephyr_interface>)

set(PIGWEED_ROOT "${CHIP_ROOT}/third_party/pigweed/repo")
include(${PIGWEED_ROOT}/pw_build/pigweed.cmake)
include(${PIGWEED_ROOT}/pw_protobuf_compiler/proto.cmake)

pw_set_backend(pw_log pw_log_basic)
pw_set_backend(pw_assert pw_assert_log)
pw_set_backend(pw_sys_io pw_sys_io.nrfconnect)
pw_set_backend(pw_trace pw_trace_tokenized)
set(dir_pw_third_party_nanopb "${CHIP_ROOT}/third_party/nanopb/repo" CACHE STRING "" FORCE)

add_subdirectory(third_party/connectedhomeip/examples/platform/nrfconnect/pw_sys_io)
add_subdirectory(third_party/connectedhomeip/third_party/nanopb/repo)
add_subdirectory(third_party/connectedhomeip/third_party/pigweed/repo)

pw_proto_library(attributes_service
  SOURCES
    ${CHIP_ROOT}/examples/common/pigweed/protos/attributes_service.proto
  INPUTS
    ${CHIP_ROOT}/examples/common/pigweed/protos/attributes_service.options
  PREFIX
    attributes_service
  STRIP_PREFIX
    ${CHIP_ROOT}/examples/common/pigweed/protos
  DEPS
    pw_protobuf.common_protos
)

pw_proto_library(button_service
  SOURCES
    ${CHIP_ROOT}/examples/common/pigweed/protos/button_service.proto
  PREFIX
    button_service
  STRIP_PREFIX
    ${CHIP_ROOT}/examples/common/pigweed/protos
  DEPS
    pw_protobuf.common_protos
)

pw_proto_library(descriptor_service
  SOURCES
    ${CHIP_ROOT}/examples/common/pigweed/protos/descriptor_service.proto
  PREFIX
    descriptor_service
  STRIP_PREFIX
    ${CHIP_ROOT}/examples/common/pigweed/protos
  DEPS
    pw_protobuf.common_protos
)

pw_proto_library(device_service
  SOURCES
    ${CHIP_ROOT}/examples/common/pigweed/protos/device_service.proto
  INPUTS
    ${CHIP_ROOT}/examples/common/pigweed/protos/device_service.options
  PREFIX
    device_service
  STRIP_PREFIX
    ${CHIP_ROOT}/examples/common/pigweed/protos
  DEPS
    pw_protobuf.common_protos
)

pw_proto_library(lighting_service
  SOURCES
    ${CHIP_ROOT}/examples/common/pigweed/protos/lighting_service.proto
  STRIP_PREFIX
    ${CHIP_ROOT}/examples/common/pigweed/protos
  PREFIX
    lighting_service
  DEPS
    pw_protobuf.common_protos
)

pw_proto_library(ot_cli_service
  SOURCES
    ${CHIP_ROOT}/examples/common/pigweed/protos/ot_cli_service.proto
  INPUTS
    ${CHIP_ROOT}/examples/common/pigweed/protos/ot_cli_service.options
  STRIP_PREFIX
    ${CHIP_ROOT}/examples/common/pigweed/protos
  PREFIX
    ot_cli_service
  DEPS
    pw_protobuf.common_protos
)

pw_proto_library(thread_service
  SOURCES
    ${CHIP_ROOT}/examples/common/pigweed/protos/thread_service.proto
  INPUTS
    ${CHIP_ROOT}/examples/common/pigweed/protos/thread_service.options
  STRIP_PREFIX
    ${CHIP_ROOT}/examples/common/pigweed/protos
  PREFIX
    thread_service
  DEPS
    pw_protobuf.common_protos
)

target_sources(app PRIVATE
  ../../common/pigweed/RpcService.cpp
  ../../common/pigweed/nrfconnect/PigweedLoggerMutex.cpp
  ${NRFCONNECT_COMMON}/Rpc.cpp
  ${NRFCONNECT_COMMON}/util/PigweedLogger.cpp
)

target_include_directories(app PRIVATE
  ${PIGWEED_ROOT}/pw_sys_io/public
  ${CHIP_ROOT}/src/lib/support
  ${CHIP_ROOT}/src/system
  ${NRFCONNECT_COMMON}
  ../../common
  ../../common/pigweed
  ../../common/pigweed/nrfconnect)

target_compile_options(app PRIVATE
                       "-DPW_RPC_ATTRIBUTE_SERVICE=1"
                       "-DPW_RPC_BUTTON_SERVICE=1"
                       "-DPW_RPC_DESCRIPTOR_SERVICE=1"
                       "-DPW_RPC_DEVICE_SERVICE=1"
                       "-DPW_RPC_LIGHTING_SERVICE=1"
                       "-DPW_RPC_THREAD_SERVICE=1"
                       "-DPW_RPC_TRACING_SERVICE=1"
                       "-DPW_TRACE_BACKEND_SET=1")

target_link_libraries(app PRIVATE
  attributes_service.nanopb_rpc
  button_service.nanopb_rpc
  descriptor_service.nanopb_rpc
  device_service.nanopb_rpc
  lighting_service.nanopb_rpc
  thread_service.nanopb_rpc
  pw_checksum
  pw_hdlc
  pw_log
  pw_rpc.server
  pw_trace_tokenized
  pw_trace_tokenized.trace_buffer
  pw_trace_tokenized.rpc_service
  pw_trace_tokenized.protos.nanopb_rpc
)

target_link_options(app
  PUBLIC
    "-T${PIGWEED_ROOT}/pw_tokenizer/pw_tokenizer_linker_sections.ld"
)

endif(CONFIG_CHIP_PW_RPC)
