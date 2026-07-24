#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring
Java_com_example_myapplication_NativeModemCore_getEngineStatus(JNIEnv* env, jobject /* this */) {
std::string status = "JNI Bridge Active! Ядро аудиомодема готово.";
return env->NewStringUTF(status.c_str());
}