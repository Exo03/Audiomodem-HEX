package com.example.myapplication

object NativeModemCore {
    init {
        System.loadLibrary("modemcore")
    }

    external fun getEngineStatus(): String

    external fun startListening(): Boolean

    external fun stopListening(): Boolean
}