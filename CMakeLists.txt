project(QNeuralNet)

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

set (QNN_SRCS race-client.cpp
     neuron.cpp
     network.cpp
     main.cpp
     mainwindow.cpp
)

set (QNN_UIS
     mainwindow.ui
)

QT4_WRAP_UI(UIS ${QNN_UIS})

set (QNN_MOCS
     mainwindow.h
     network.h
     race-client.h
)

QT4_WRAP_CPP(MOCS ${QNN_MOCS})

add_executable (qneuralnet
                ${QNN_SRCS}
                ${UIS}
                ${RSCS}
                ${TRS}
                ${MOCS}
)

target_link_libraries (qneuralnet
                       ${QT_QTCORE_LIBRARY}
                       ${QT_QTGUI_LIBRARY}
                       ${QT_QTNETWORK_LIBRARY}
)