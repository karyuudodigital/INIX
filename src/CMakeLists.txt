set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent Test)

set(APP_SOURCES
    main.cpp
    assets/resources.qrc
    app/MainWindow.cpp
    domain/IniDocument.cpp
    services/IniParser.cpp
    services/IniSerializer.cpp
    services/IniDiffService.cpp
    services/IniMergeService.cpp
    services/IniSearchService.cpp
    ui/models/IniSettingsTableModel.cpp
    ui/models/IniFilterProxyModel.cpp
    ui/models/DiffTableModel.cpp
    ui/models/MergePreviewTableModel.cpp
)

if(WIN32)
    list(APPEND APP_SOURCES assets/app_icon.rc)
endif()

set(APP_HEADERS
    app/MainWindow.h
    domain/IniTypes.h
    domain/IniDocument.h
    services/IniParser.h
    services/IniSerializer.h
    services/IniDiffService.h
    services/IniMergeService.h
    services/IniSearchService.h
    ui/models/IniSettingsTableModel.h
    ui/models/IniFilterProxyModel.h
    ui/models/DiffTableModel.h
    ui/models/MergePreviewTableModel.h
)

add_executable(INIX
    ${APP_SOURCES}
    ${APP_HEADERS}
)

target_include_directories(INIX PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(INIX PRIVATE Qt6::Core Qt6::Widgets Qt6::Concurrent)
