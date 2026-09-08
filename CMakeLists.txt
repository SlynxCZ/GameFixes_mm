cmake_minimum_required(VERSION 3.18)
project(game_fixes C CXX ASM)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

foreach(var HL2SDKCS2 MMSOURCE_DEV CSGO_PROTO)
    if(NOT DEFINED ENV{${var}})
        message(FATAL_ERROR "Environment variable ${var} is not set!")
    endif()
endforeach()

set(SOURCESDK_DIR $ENV{HL2SDKCS2})
set(METAMOD_DIR $ENV{MMSOURCE_DEV})
set(CSGO_PROTO_DIR $ENV{CSGO_PROTO})

include(makefiles/shared.cmake)
include(makefiles/protobuf.cmake)

add_subdirectory(vendor/dynlibutils)

file(GLOB_RECURSE SOURCE_FILES
    src/*.cpp
    src/*.h
    src/*.hpp
)

list(APPEND SOURCE_FILES
    ${SOURCESDK_DIR}/public/tier0/memoverride.cpp
    ${SOURCESDK_DIR}/tier1/keyvalues3.cpp
    ${SOURCESDK_DIR}/tier1/convar.cpp
    ${SOURCESDK_DIR}/entity2/entityidentity.cpp
    ${SOURCESDK_DIR}/entity2/entitysystem.cpp
    ${SOURCESDK_DIR}/entity2/entitykeyvalues.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    find_program(STRIP_EXECUTABLE strip)
    if (STRIP_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${STRIP_EXECUTABLE} --strip-unneeded $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "Stripping symbols from $<TARGET_FILE:${PROJECT_NAME}>"
        )
    endif()
endif()

if (LINUX)
    include(${CMAKE_SOURCE_DIR}/makefiles/linux.base.cmake)
    set(PLUGIN_BIN_DIR "linuxsteamrt64")
elseif (WIN32)
    include(${CMAKE_SOURCE_DIR}/makefiles/windows.base.cmake)
    set(PLUGIN_BIN_DIR "win64")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/addons/${PROJECT_NAME}/bin/${PLUGIN_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/addons/${PROJECT_NAME}/bin/${PLUGIN_BIN_DIR}"
)

target_link_libraries(${PROJECT_NAME} ${LINK_LIBRARIES})

# configs/ (the .ini and the generated metamod .vdf) lands next to the binary,
# so ${CMAKE_BINARY_DIR}/addons is a complete, drop-in install tree.
add_custom_command(
    TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/configs ${CMAKE_BINARY_DIR}
)
