TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_MAC_SDK = macosx10.14

SOURCES += \
        main.cpp

HEADERS += \
    task.hpp \
    synchronised.hpp \
    integer_sequence.hpp \
    timed_queue.hpp
