# qt5
find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(QRC_FILE res.qrc)

qt5_add_resources(QRC ${QRC_FILE})
add_executable(app main.cc mainwindow.cc itemdelegate.cc listview.cc password.cc ${QRC})

target_link_libraries(app PUBLIC
                      Qt5::Widgets 
                      bolo
                      ${CMAKE_DL_LIBS}
                      ${GNU_FS_LIB})
