
#
# Copyright (C) The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Generate one static lib and one shared lib, copy them into
# ${project_dir}/distribution for other indepdendent applications
# to use.
cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_VERBOSE_MAKEFILE on)

set(lib_src_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(lib_build_DIR $ENV{HOME}/tmp/${ANDROID_ABI})
file(MAKE_DIRECTORY ${lib_build_DIR})

add_subdirectory(${lib_src_DIR}/gmath ${lib_build_DIR}/gmath)
add_subdirectory(${lib_src_DIR}/gperf ${lib_build_DIR}/gperf)

set(BUILD_SHARED_LIBS ON)
add_subdirectory(${lib_src_DIR}/socket.io-client-cpp ${lib_build_DIR}/socket.io-client-cpp)

# copy out the lib binary... need to leave the static lib around to pass gradle check
set(distribution_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../distribution)
set_target_properties(sioclient PROPERTIES ARCHIVE_OUTPUT_DIRECTORY  "${distribution_DIR}/lib/${ANDROID_ABI}")
set_target_properties(sioclient PROPERTIES LIBRARY_OUTPUT_DIRECTORY  "${distribution_DIR}/lib/${ANDROID_ABI}")
set_target_properties(sioclient PROPERTIES RUNTIME_OUTPUT_DIRECTORY  "${distribution_DIR}/bin/${ANDROID_ABI}")

#
# FIXME: copy headers to the distribution directory
#
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/socket.io-client-cpp/src/sio_client.h" DESTINATION "${distribution_DIR}/include")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/socket.io-client-cpp/src/sio_socket.h" DESTINATION "${distribution_DIR}/include")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/socket.io-client-cpp/src/sio_message.h" DESTINATION "${distribution_DIR}/include")

# copy out lib header file...
#add_custom_command(TARGET sioclient POST_BUILD
#        COMMAND "${CMAKE_COMMAND}" -E
#        copy "${CMAKE_CURRENT_SOURCE_DIR}/socket.io-client-cpp/src/sio_client.h"
#        "${distribution_DIR}/include/sio_client.h"
#        #                   **** the following 2 lines are for potential future debug purpose ****
#        #                   COMMAND "${CMAKE_COMMAND}" -E
#        #                   remove_directory "${CMAKE_CURRENT_BINARY_DIR}"
#        COMMENT "Copying socket.io-client-cpp to output directory")
#
#add_custom_command(OUTPUT "${distribution_DIR}/include/sio_socket.h"
#        COMMAND "${CMAKE_COMMAND}" -E
#        copy "${CMAKE_CURRENT_SOURCE_DIR}/socket.io-client-cpp/src/sio_socket.h"
#        "${distribution_DIR}/include/sio_socket.h"
#        #                   **** the following 2 lines are for potential future debug purpose ****
#        #                   COMMAND "${CMAKE_COMMAND}" -E
#        #                   remove_directory "${CMAKE_CURRENT_BINARY_DIR}"
#        COMMENT "Copying socket.io-client-cpp to output directory")
#
#add_custom_command(OUTPUT "${distribution_DIR}/include/sio_message.h"
#        COMMAND "${CMAKE_COMMAND}" -E
#        copy "${CMAKE_CURRENT_SOURCE_DIR}/socket.io-client-cpp/src/sio_message.h"
#        "${distribution_DIR}/include/sio_message.h"
#        #                   **** the following 2 lines are for potential future debug purpose ****
#        #                   COMMAND "${CMAKE_COMMAND}" -E
#        #                   remove_directory "${CMAKE_CURRENT_BINARY_DIR}"
#        COMMENT "Copying socket.io-client-cpp to output directory")

#add_custom_target(copy_sio_client_h ALL
#        COMMAND "${CMAKE_COMMAND}" -E
#        copy "${CMAKE_CURRENT_SOURCE_DIR}/socket.io-client-cpp/src/sio_client.h"
#        "${distribution_DIR}/include"
#        #                   **** the following 2 lines are for potential future debug purpose ****
#        #                   COMMAND "${CMAKE_COMMAND}" -E
#        #                   remove_directory "${CMAKE_CURRENT_BINARY_DIR}"
#        COMMENT "Copying socket.io-client-cpp to output directory")
#
#add_custom_target(copy_sio_socket_h ALL
#        COMMAND "${CMAKE_COMMAND}" -E
#        copy "${CMAKE_CURRENT_SOURCE_DIR}/socket.io-client-cpp/src/sio_socket.h"
#        "${distribution_DIR}/include"
#        #                   **** the following 2 lines are for potential future debug purpose ****
#        #                   COMMAND "${CMAKE_COMMAND}" -E
#        #                   remove_directory "${CMAKE_CURRENT_BINARY_DIR}"
#        COMMENT "Copying socket.io-client-cpp to output directory")
#
#add_custom_target(copy_sio_message_h ALL
#        COMMAND "${CMAKE_COMMAND}" -E
#        copy "${CMAKE_CURRENT_SOURCE_DIR}/socket.io-client-cpp/src/sio_message.h"
#        "${distribution_DIR}/include"
#        #                   **** the following 2 lines are for potential future debug purpose ****
#        #                   COMMAND "${CMAKE_COMMAND}" -E
#        #                   remove_directory "${CMAKE_CURRENT_BINARY_DIR}"
#        COMMENT "Copying socket.io-client-cpp to output directory")
