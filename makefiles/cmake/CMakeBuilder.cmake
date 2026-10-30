# The plugin binary -- CMake twin of makefiles/ambuild/AMBuilder: the SDK sources
# (matching makefiles/ambuild/shared.ambuild), the plugin's own sources in
# AMBuilder's order, its protobufs and what it links with.
# Included by CMakeLists.txt after shared.cmake and the platform file.

# The SDK's translation units -- the same list, in the same order, as
# makefiles/ambuild/shared.ambuild.
set(SOURCE_FILES
    ${SOURCESDK_DIR}/public/tier0/memoverride.cpp
    ${SOURCESDK_DIR}/tier0_perproject/keyvalues3.cpp
    ${SOURCESDK_DIR}/tier0_perproject/convar.cpp
    ${SOURCESDK_DIR}/entity2/entityidentity.cpp
    ${SOURCESDK_DIR}/entity2/entitysystem.cpp
    ${SOURCESDK_DIR}/entity2/entitykeyvalues.cpp
)

# The plugin's own sources -- the same list, in the same order, as
# makefiles/ambuild/AMBuilder. Listed, not globbed: a new .cpp goes into both.
list(APPEND SOURCE_FILES
    src/plugin.cpp
    src/scheduler.cpp
    src/fixes/fix.cpp
    src/fixes/abstract_entity.cpp
    src/fixes/beam_crash.cpp
    src/fixes/demo_record.cpp
    src/fixes/hammer_id.cpp
    src/fixes/input_activator_crash.cpp
    src/fixes/rampbug.cpp
    src/fixes/water_jump.cpp
    src/fixes/server_list_players.cpp
    src/fixes/slow_animation.cpp
    src/fixes/steam_ban.cpp
    src/fixes/sv_cheats.cpp
    src/fixes/team_limit.cpp
    src/fixes/voice_flood.cpp
    src/sdk/schemasystem_helper.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
s2sdk_protobufs(${PROJECT_NAME}
    common/network_connection.proto
    common/networkbasetypes.proto
    networksystem/networksystem_protomessages.proto
    game/shared/cs/cs_gameevents.proto
    game/shared/cs/cs_usercmd.proto
    game/shared/base_gcmessages.proto
    game/shared/econ/econ_gcmessages.proto
    common/engine_gcmessages.proto
    gcsdk/gcsdk_gcmessages.proto
    gcsdk/gcsystemmsgs.proto
    game/shared/cstrike15/cstrike15_gcmessages.proto
    game/shared/cstrike15/cstrike15_usermessages.proto
    common/source2_steam_stats.proto
    common/netmessages.proto
    gcsdk/steammessages.proto
    game/shared/usercmd.proto
    game/shared/usermessages.proto
    game/shared/gameevents.proto
    game/shared/clientmessages.proto
    game/shared/te.proto
    common/valveextensions.proto
)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    find_program(STRIP_EXECUTABLE strip)
    if (STRIP_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${STRIP_EXECUTABLE} --strip-unneeded $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "Stripping symbols from $<TARGET_FILE:${PROJECT_NAME}>"
        )
    endif()
endif()

target_link_libraries(${PROJECT_NAME} ${LINK_LIBRARIES})

# What the platform file asked for on the link line (the export map on Linux),
# like linux.ambuild / windows.ambuild's ConfigureBinary() on the AMBuild side.
if (PLUGIN_LINK_OPTIONS)
    target_link_options(${PROJECT_NAME} PRIVATE ${PLUGIN_LINK_OPTIONS})
endif ()
if (PLUGIN_LINK_DEPENDS)
    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS ${PLUGIN_LINK_DEPENDS})
endif ()
