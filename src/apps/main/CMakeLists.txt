add_executable(main main.cpp layout_preview.cpp)
target_link_libraries(main PRIVATE imgui_impl edgar)

# Bundle Edgar.GUI-style assets (mirrors Edgar-DotNet `src/Resources`: MapDescriptions, Maps, etc.)
set(LEVELSYNTH_EDGAR_GUI_RESOURCES "${CMAKE_SOURCE_DIR}/resources/edgar_gui")
if(EXISTS "${LEVELSYNTH_EDGAR_GUI_RESOURCES}")
    add_custom_command(
        TARGET main
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${LEVELSYNTH_EDGAR_GUI_RESOURCES}"
                "$<TARGET_FILE_DIR:main>/resources/edgar_gui"
        COMMENT "Copy resources/edgar_gui next to main (MapDescriptions, Images, etc.)")
endif()
