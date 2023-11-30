set(autotype_win_SOURCES AutoTypeWindows.cpp)

add_library(keepassxc-autotype-windows MODULE ${autotype_win_SOURCES})
target_link_libraries(keepassxc-autotype-windows keepassx_core ${autotype_LIB} Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)
install(TARGETS keepassxc-autotype-windows
        BUNDLE DESTINATION . COMPONENT Runtime
        LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR} COMPONENT Runtime)
