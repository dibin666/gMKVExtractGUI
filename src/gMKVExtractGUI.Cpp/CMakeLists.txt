qt_add_executable(gMKVExtractGUIQt
    include/gmkvextractgui/ExtractionController.h
    include/gmkvextractgui/JobManagerWindow.h
    include/gmkvextractgui/LogWindow.h
    include/gmkvextractgui/MainWindow.h
    include/gmkvextractgui/OptionsDialog.h
    include/gmkvextractgui/TranslationEditorDialog.h
    include/gmkvextractgui/UiLocalization.h
    src/ExtractionController.cpp
    src/JobManagerWindow.cpp
    src/LogWindow.cpp
    src/MainWindow.cpp
    src/OptionsDialog.cpp
    src/TranslationEditorDialog.cpp
    src/UiLocalization.cpp
    src/main.cpp
)

target_include_directories(gMKVExtractGUIQt
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gMKVExtractGUIQt
    PRIVATE
        Qt6::Widgets
        gMKVToolNixCpp
)

set_target_properties(gMKVExtractGUIQt PROPERTIES
    WIN32_EXECUTABLE TRUE
)

file(GLOB GMKV_LOCALE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/locales/gmkvextract-*.json"
)
set(GMKV_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/gMkvExtractGuiIcon.ico")
set(GMKV_LINUX_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/linux/${GMKV_APP_ID}.svg")
set(GMKV_LINUX_DESKTOP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/linux/${GMKV_APP_ID}.desktop")
set(GMKV_LINUX_METAINFO_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/resources/linux/${GMKV_APP_ID}.metainfo.xml.in")
set(GMKV_LINUX_METAINFO_FILE "${CMAKE_CURRENT_BINARY_DIR}/${GMKV_APP_ID}.metainfo.xml")

if(WIN32)
    file(TO_NATIVE_PATH "${GMKV_ICON_FILE}" GMKV_WINDOWS_ICON_FILE_NATIVE)
    string(REPLACE "\\" "\\\\" GMKV_WINDOWS_ICON_FILE_ESCAPED "${GMKV_WINDOWS_ICON_FILE_NATIVE}")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/gMKVExtractGUI.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/gMKVExtractGUI.rc"
        @ONLY
    )
    target_sources(gMKVExtractGUIQt PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gMKVExtractGUI.rc")
endif()

add_custom_command(TARGET gMKVExtractGUIQt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${GMKV_LOCALE_FILES}
        "$<TARGET_FILE_DIR:gMKVExtractGUIQt>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${GMKV_ICON_FILE}"
        "$<TARGET_FILE_DIR:gMKVExtractGUIQt>"
)

install(TARGETS gMKVExtractGUIQt
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)
install(FILES ${GMKV_LOCALE_FILES}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES "${GMKV_ICON_FILE}"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(UNIX AND NOT APPLE)
    string(TIMESTAMP GMKV_RELEASE_DATE "%Y-%m-%d" UTC)
    configure_file(
        "${GMKV_LINUX_METAINFO_TEMPLATE}"
        "${GMKV_LINUX_METAINFO_FILE}"
        @ONLY
    )
    install(FILES "${GMKV_LINUX_DESKTOP_FILE}"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(FILES "${GMKV_LINUX_ICON_FILE}"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    )
    install(FILES "${GMKV_LINUX_METAINFO_FILE}"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
    )
endif()

if(GMKV_BUILD_CPP_TESTS)
    add_test(NAME gMKVExtractGUIQtSmoke COMMAND gMKVExtractGUIQt --smoke-test)
    set_tests_properties(gMKVExtractGUIQtSmoke PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
    add_test(NAME gMKVExtractGUIQtSelectionSmoke COMMAND gMKVExtractGUIQt --selection-smoke-test)
    set_tests_properties(gMKVExtractGUIQtSelectionSmoke PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()
