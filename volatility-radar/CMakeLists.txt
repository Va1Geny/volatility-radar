cmake_minimum_required(VERSION 3.22)
project(volatility-radar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Charts WebSockets Network Test)

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
# The analyzer ships with its trained model; users create analyzer/.venv to enable predictions.
install(DIRECTORY analyzer DESTINATION ${CMAKE_INSTALL_BINDIR}
    PATTERN ".venv" EXCLUDE
    PATTERN "cache" EXCLUDE
    PATTERN "__pycache__" EXCLUDE
    PATTERN "*.log" EXCLUDE
    PATTERN "*.log.err" EXCLUDE
    PATTERN "direction_lstm.keras" EXCLUDE)

# Unit tests: `ctest --test-dir <build dir>` (no window needed).
enable_testing()
qt_add_executable(test_stockmodel tests/test_stockmodel.cpp StockModel.cpp StockFilterProxy.cpp)
target_include_directories(test_stockmodel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_stockmodel PRIVATE Qt6::Core Qt6::Test)
add_test(NAME test_stockmodel COMMAND test_stockmodel)
if(WIN32)  # let ctest find the Qt DLLs
    set_tests_properties(test_stockmodel PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
endif()

qt_generate_deploy_app_script(
    TARGET volatility-radar
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})