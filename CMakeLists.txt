cmake_minimum_required(VERSION 3.21)
project(evernight VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(QT_QML_GENERATE_QMLLS_INI OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Multimedia Svg Network)
qt_standard_project_setup()
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

set(KIT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kit)

function(collect out)
    set(patterns "")
    foreach(p IN LISTS ARGN)
        list(APPEND patterns "${CMAKE_CURRENT_SOURCE_DIR}/${p}")
    endforeach()
    file(GLOB found RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS ${patterns})
    list(SORT found)
    set(${out} ${found} PARENT_SCOPE)
endfunction()

qt_add_executable(appEvernight main.cpp)

collect(KIT_CORE_SOURCES kit/Core/*.cpp kit/Core/*.hpp)

qt_add_qml_module(EvCore
    STATIC
    URI Evernight.Core
    VERSION 1.0
    NO_PLUGIN
    SOURCES ${KIT_CORE_SOURCES}
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Evernight/Core"
)
set_target_properties(EvCore PROPERTIES POSITION_INDEPENDENT_CODE ON AUTOMOC ON)
target_include_directories(EvCore PUBLIC ${KIT_DIR} ${KIT_DIR}/Core)
target_link_libraries(EvCore PUBLIC Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)

function(kit_module name)
    cmake_parse_arguments(ARG "" "" "LIBS;DEPS" ${ARGN})
    collect(module_sources kit/${name}/*.cpp kit/${name}/*.hpp)

    qt_add_qml_module(Ev${name}
        STATIC
        URI Evernight.${name}
        VERSION 1.0
        SOURCES ${module_sources}
        DEPENDENCIES ${ARG_DEPS}
        NO_PLUGIN_OPTIONAL
        OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Evernight/${name}"
    )

    set_target_properties(Ev${name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(Ev${name} PUBLIC ${KIT_DIR}/${name})
    target_include_directories(Ev${name} PRIVATE ${KIT_DIR})
    target_link_libraries(Ev${name} PUBLIC Qt6::Core Qt6::Qml Qt6::Gui ${ARG_LIBS})
endfunction()

kit_module(Shapes
    LIBS Qt6::Quick EvCore
    DEPS QtQuick Evernight.Core
)
target_sources(EvShapes PRIVATE kit/Shapes/shapedata.inc)

kit_module(Extraa
    LIBS Qt6::Quick EvCore
    DEPS QtQuick Evernight.Core
)

set_source_files_properties(theme/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(theme/Snack.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

collect(APP_SOURCES core/*.cpp core/*.h)
collect(APP_QML components/*.qml pages/*.qml)
collect(APP_ICONS icons/*.svg)
collect(APP_THEMES theme/variation/*.ini)

qt_add_qml_module(appEvernight
    URI Evernight
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Evernight"
    SOURCES ${APP_SOURCES}
    QML_FILES
        Main.qml
        theme/Theme.qml
        theme/Snack.qml
        ${APP_QML}
    RESOURCES ${APP_ICONS} ${APP_THEMES}
)

target_include_directories(appEvernight PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)

set_target_properties(appEvernight PROPERTIES
    QT_ANDROID_APP_NAME "Evernight"
    QT_ANDROID_PACKAGE_NAME "com.safalski.evernight"
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
)

set(DISCORD_SDK ${CMAKE_CURRENT_SOURCE_DIR}/third_party/discord_social_sdk)
if(ANDROID)
    add_library(discord_sdk SHARED IMPORTED)
    set_target_properties(discord_sdk PROPERTIES
        IMPORTED_LOCATION ${DISCORD_SDK}/android/jni/arm64-v8a/libdiscord_partner_sdk.so
        INTERFACE_INCLUDE_DIRECTORIES ${DISCORD_SDK}/include)
    set_property(TARGET appEvernight APPEND PROPERTY QT_ANDROID_EXTRA_LIBS
        ${DISCORD_SDK}/android/jni/arm64-v8a/libdiscord_partner_sdk.so)
endif()

target_link_libraries(appEvernight PRIVATE
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Svg
    Qt6::Network
    EvShapes
    EvShapesplugin
    EvExtraa
    EvExtraaplugin
    $<$<BOOL:${ANDROID}>:discord_sdk>
)

qt_import_qml_plugins(appEvernight)
