#include <jni.h>
#include <string>
#include <oboe/Oboe.h>
#include <android/log.h>
#include "modulator.cpp"
#include "demodulator.cpp"

#define LOG_TAG "NativeModemCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


class ModemAudioCallback : public oboe::AudioStreamCallback {
public:
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) override {

        int16_t *inputData = static_cast<int16_t *>(audioData);

        return oboe::DataCallbackResult::Continue;
    }
};

oboe::AudioStream *stream = nullptr;
ModemAudioCallback callback;

bool startAudioRecord() {
    oboe::AudioStreamBuilder builder;

    builder.setDirection(oboe::Direction::Input)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setFormat(oboe::AudioFormat::I16)
            ->setChannelCount(oboe::ChannelCount::Mono)
            ->setSampleRate(44100)
            ->setCallback(&callback);

    oboe::Result result = builder.openStream(&stream);

    if (result == oboe::Result::OK) {
        stream->requestStart();
        LOGI("Аудиопоток успешно запущен!");
        return true;
    } else {
        LOGE("Ошибка запуска аудиопотока: %s", oboe::convertToText(result));
        return false;
    }
}

bool stopAudioRecord() {
    if (stream != nullptr) {
        oboe::Result stopResult = stream->requestStop();

        oboe::Result closeResult = stream->close();

        stream = nullptr;

        if (stopResult == oboe::Result::OK && closeResult == oboe::Result::OK) {
            LOGI("Аудиопоток успешно остановлен и закрыт.");
            return true;
        } else {
            LOGE("Ошибка при остановке/закрытии аудиопотока.");
            return false;
        }
    }

    return true;
}

extern "C" JNIEXPORT jboolean
Java_com_example_myapplication_NativeModemCore_stopListening(JNIEnv *env, jobject /* this */) {
    bool isStopped = stopAudioRecord();
    return isStopped ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jstring
Java_com_example_myapplication_NativeModemCore_getEngineStatus(JNIEnv* env, jobject /* this */) {
std::string status = "JNI Bridge Active! Ядро аудиомодема готово.";
return env->NewStringUTF(status.c_str());
}

extern "C" JNIEXPORT jboolean
Java_com_example_myapplication_NativeModemCore_startListening(JNIEnv *env, jobject /* this */) {

    bool isStarted = startAudioRecord();

    return isStarted ? JNI_TRUE : JNI_FALSE;
}