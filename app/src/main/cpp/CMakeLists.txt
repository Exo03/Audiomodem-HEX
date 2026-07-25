cmake_minimum_required(VERSION 3.22.1)

project("modemcore")

# Ищем библиотеку Oboe через prefab
find_package(oboe REQUIRED CONFIG)

# Добавляем наш исходный файл (замени на имя своего файла, если оно другое)
add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp
        modulator.cpp
        demodulator.cpp)

# Линкуем библиотеки: наш проект, oboe и системный логгер
target_link_libraries(${CMAKE_PROJECT_NAME}
        oboe::oboe
        log)