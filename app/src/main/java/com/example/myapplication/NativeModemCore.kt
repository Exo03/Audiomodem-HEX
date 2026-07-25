package com.example.myapplication

object NativeModemCore {
    init {
        System.loadLibrary("modemcore") //[cite: 5]
    }

    external fun getEngineStatus(): String //[cite: 5]

    external fun startListening(filePath: String): Boolean

    external fun stopListening(): Boolean //[cite: 5]

    // Новые функции для работы с файлами
    external fun modulateFile(inputFilePath: String, outputWavPath: String): Boolean

    external fun demodulateFile(inputWavPath: String, outputFilePath: String): Boolean
}