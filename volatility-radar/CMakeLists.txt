cmake_minimum_required(VERSION 3.19)
project(volatility-radar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Charts WebSockets Network)

qt_standard_project_setup()

qt_add_executable(volatility-radar
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
    FinnhubClient.cpp
    FinnhubClient.h
    FinnhubRest.cpp
    FinnhubRest.h
    PredictionClient.cpp
    PredictionClient.h
)

# .exe icon (Explorer, taskbar) on Windows; window icon everywhere via Qt resources.
if(WIN32)
    target_sources(volatility-radar PRIVATE app.rc)
endif()
qt_add_resources(volatility-radar "icon" PREFIX "/" FILES resources/app.png)

target_link_libraries(volatility-radar
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Charts
        Qt6::WebSockets
        Qt6::Network
)

# Data files (.env, watchlist.txt, stocks.json, style.qss) are read from the source tree
# if it exists, else from next to the exe (installed copies below).
target_compile_definitions(volatility-radar PRIVATE
    APP_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

install(TARGETS volatility-radar
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES stocks.json style.qss watchlist.txt .env.example
    DESTINATION ${CMAKE_INSTALL_BINDIR})

qt_generate_deploy_app_script(
    TARGET volatility-radar
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})