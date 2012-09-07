project(MLP)

cmake_minimum_required( VERSION 2.6 )
set ( CMAKE_BUILD_TYPE Release )
add_definitions ( -Wall ${QT_DEFINITIONS})

find_package ( Qt4 REQUIRED )

include ( ${QT_USE_FILE} )
include_directories (${CMAKE_SOURCE_DIR}
                     ${CMAKE_BINARY_DIR}
                     ${QT_QTCORE_INCLUDE_DIR}
                     ${QT_QTGUI_INCLUDE_DIR}
                     ${QT_QTNETWORK_INCLUDE_DIR}
)

set (MLP_SRCS race-client.cpp
     neuron.cpp
     network.cpp
     main.cpp
     mainwindow.cpp
)

set (MLP_UIS
     mainwindow.ui
)

QT4_WRAP_UI(UIS ${MLP_UIS})

set (MLP_MOCS
     mainwindow.h
     network.h
     race-client.h
)

QT4_WRAP_CPP(MOCS ${MLP_MOCS})

add_executable (MLP
                ${MLP_SRCS}
                ${UIS}
                ${RSCS}
                ${TRS}
                ${MOCS}
)

target_link_libraries (MLP
                       ${QT_QTCORE_LIBRARY}
                       ${QT_QTGUI_LIBRARY}
                       ${QT_QTNETWORK_LIBRARY}
)