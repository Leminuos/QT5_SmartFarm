cmake_minimum_required(VERSION 3.5)

project(QT5_SmartFarm LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

file(GLOB_RECURSE SRC_FILES
  "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE HDR_FILES
  "${CMAKE_SOURCE_DIR}/include/*.h"
)

set(RESOURCES
  resources/resources.qrc
)

set(UI_FILES
  src/mainwindow.ui
)

if(ANDROID)
  add_library(${PROJECT_NAME} SHARED
    ${SRC_FILES}
    ${HDR_FILES}
    ${RESOURCES}
    ${UI_FILES}
  )
else()
  add_executable(${PROJECT_NAME}
    ${SRC_FILES}
    ${HDR_FILES}
    ${RESOURCES}
    ${UI_FILES}
  )
endif()

find_library(MOSQUITTO_LIB mosquitto PATHS ${CMAKE_SYSROOT}/usr/lib)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets ${MOSQUITTO_LIB} gpiod)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

set(INSTALL_DESTDIR "/opt")

install(TARGETS QT5_SmartFarm
    RUNTIME DESTINATION "${INSTALL_DESTDIR}"
    BUNDLE DESTINATION "${INSTALL_DESTDIR}"
    LIBRARY DESTINATION "${INSTALL_DESTDIR}"
)
