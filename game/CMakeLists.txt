add_executable(NineUnder
    src/main.cpp
    src/game_app.cpp
    src/arena_greybox.cpp
    src/map_data.cpp
)

target_include_directories(NineUnder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(NineUnder PRIVATE nu::engine)

if(MSVC)
    target_compile_options(NineUnder PRIVATE /W4)
endif()

set_target_properties(NineUnder PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:NineUnder>"
    OUTPUT_NAME "NineUnder"
)

nu_stage_runtime_data(NineUnder)
