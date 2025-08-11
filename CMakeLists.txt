cmake_minimum_required(VERSION 3.16)

project(qml-remoteserver VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Quick SerialPort Network)

# Enable automatic MOC, UIC and RCC processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

qt_add_executable(appqml-remoteserver
    main.cpp
    genericqmlbridge.h
    genericqmlbridge.cpp
    slipprocessor.h
    slipprocessor.cpp
    datadecoder.hpp
    QmlPropertyObserver.hpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appqml-remoteserver PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appqml-remoteserver
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appqml-remoteserver
    PRIVATE Qt6::Core Qt6::Quick Qt6::SerialPort Qt6::Network
)

include(GNUInstallDirs)
install(TARGETS appqml-remoteserver
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
