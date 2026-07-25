# Указываем минимальную версию CMake (стандартная для современных Android-проектов)
cmake_minimum_required(VERSION 3.22.1)

# Имя проекта для C++ части
project("audiomodem")

# Создаем саму библиотеку.
# ВАЖНО: Имя "Klim" должно в точности совпадать с тем, что мы указали в System.loadLibrary("Klim")
add_library(
        Klim
        SHARED
        Klim.cpp
)

# Подключаем стандартную библиотеку логирования Android (пригодится для отладки C++ кода)
find_library(
        log-lib
        log
)

# Связываем нашу библиотеку Klim с системными библиотеками (например, с логгером)
target_link_libraries(
        Klim
        ${log-lib}
)