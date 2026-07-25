package com.example.myapplication

object NativeModemCore {
    // Загружаем библиотеку. Имя должно совпадать с тем, что указано в CMakeLists.txt
    init {
        System.loadLibrary("Klim")
    }

    // Объявляем внешнюю функцию, которая будет реализована в C++
    external fun getEngineStatus(): String
}