cmake_minimum_required(VERSION 3.19)
project(finviz-desktop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Charts)

qt_standard_project_setup()

qt_add_executable(finviz-desktop
    WIN32
    MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    StockModel.cpp
    StockModel.h
    StockDelegate.cpp
    StockDelegate.h
    StockFilterProxy.cpp
    StockFilterProxy.h
)

target_link_libraries(finviz-desktop
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Charts
)

add_custom_command(TARGET finviz-desktop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/stocks.csv"
        "$<TARGET_FILE_DIR:finviz-desktop>/stocks.csv"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/style.qss"
        "$<TARGET_FILE_DIR:finviz-desktop>/style.qss"
)

install(TARGETS finviz-desktop
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET finviz-desktop
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})