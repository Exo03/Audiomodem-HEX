#include <jni.h>
#include <string>
#include <fstream>
#include <oboe/Oboe.h>
#include <android/log.h>

#define LOG_TAG "NativeModemCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

bool modulate(const std::string& inputPath, const std::string& outputPath);
bool demodulate(const std::string& inputPath, const std::string& outputPath);

#pragma pack(push, 1)
struct AppWavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 1;
    uint32_t sampleRate = 44100;
    uint32_t byteRate = 44100 * 1 * 2;
    uint16_t blockAlign = 2;
    uint16_t bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize;
};
#pragma pack(pop)

// Глобальные переменные для записи в файл
std::ofstream wavStream;
uint32_t recordedDataSize = 0;
oboe::AudioStream *stream = nullptr;

class ModemAudioCallback : public oboe::AudioStreamCallback {
public:
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames) override {
        // Если файл открыт, пишем в него сырые байты (PCM)
        if (wavStream.is_open()) {
            size_t bytesToWrite = numFrames * sizeof(int16_t);
            wavStream.write(static_cast<const char*>(audioData), bytesToWrite);
            recordedDataSize += bytesToWrite;
        }
        return oboe::DataCallbackResult::Continue;
    }
};

ModemAudioCallback callback;

bool startAudioRecord(const std::string& path) {
    // 1. Открываем файл и пишем пустой заголовок (заполним реальными размерами при остановке)
    wavStream.open(path, std::ios::binary);
    if (!wavStream.is_open()) {
        LOGE("Не удалось открыть файл для записи: %s", path.c_str());
        return false;
    }

    recordedDataSize = 0;
    AppWavHeader dummyHeader;
    wavStream.write(reinterpret_cast<const char*>(&dummyHeader), sizeof(AppWavHeader));

    // 2. Запускаем Oboe
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
        LOGI("Запись эфира начата. Файл: %s", path.c_str());
        return true;
    } else {
        LOGE("Ошибка запуска Oboe: %s", oboe::convertToText(result));
        wavStream.close();
        return false;
    }
}

// Обновленная функция остановки: финализирует WAV-файл
bool stopAudioRecord() {
    bool success = true;
    if (stream != nullptr) {
        stream->requestStop();
        stream->close();
        stream = nullptr;
        LOGI("Микрофон остановлен.");
    }

    // Перезаписываем WAV-заголовок правильными размерами
    if (wavStream.is_open()) {
        wavStream.seekp(0, std::ios::beg); // Возвращаемся в начало файла

        AppWavHeader header;
        header.dataSize = recordedDataSize;
        header.fileSize = 36 + recordedDataSize;

        wavStream.write(reinterpret_cast<const char*>(&header), sizeof(AppWavHeader));
        wavStream.close();
        LOGI("WAV файл успешно сохранен. Размер данных: %u байт", recordedDataSize);
    }

    return success;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_myapplication_NativeModemCore_modulateFile(JNIEnv *env, jobject /* this */,
                                                            jstring input_path,
                                                            jstring output_wav_path) {
    // 1. Конвертируем пути из JNI (Java) в стандартные C++ строки
    const char *input_c_str = env->GetStringUTFChars(input_path, nullptr);
    const char *output_c_str = env->GetStringUTFChars(output_wav_path, nullptr);
    std::string input(input_c_str);
    std::string output(output_c_str);

    bool success = false;
    try {
        LOGI("Запуск модуляции: %s -> %s", input_c_str, output_c_str);

        // ВЫЗЫВАЕМ ФУНКЦИЮ НАПАРНИКА
        success = modulate(input, output);

    } catch (const std::exception& e) { // <-- Добавили открывающую скобку
        // Логируем ошибку
        LOGE("Ошибка модуляции: %s", e.what());
    } // <-- Добавили закрывающую скобку

    // 3. Освобождаем память
    env->ReleaseStringUTFChars(input_path, input_c_str);
    env->ReleaseStringUTFChars(output_wav_path, output_c_str);

    return success ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_myapplication_NativeModemCore_demodulateFile(JNIEnv *env, jobject /* this */,
                                                              jstring input_wav_path,
                                                              jstring output_file_path) {
    const char *input_c_str = env->GetStringUTFChars(input_wav_path, nullptr);
    const char *output_c_str = env->GetStringUTFChars(output_file_path, nullptr);
    std::string input(input_c_str);
    std::string output(output_c_str);

    bool success = false;
    try {
        LOGI("Запуск демодуляции: %s -> %s", input_c_str, output_c_str);

        // ВЫЗЫВАЕМ ФУНКЦИЮ НАПАРНИКА
        success = demodulate(input, output);

    } catch (const std::exception& e) {
        // Логируем ошибку
        LOGE("Ошибка демодуляции: %s", e.what());
    } // <-- Добавили закрывающую скобку, которой не хватало

    env->ReleaseStringUTFChars(input_wav_path, input_c_str);
    env->ReleaseStringUTFChars(output_file_path, output_c_str);

    return success ? JNI_TRUE : JNI_FALSE;
}



extern "C" JNIEXPORT jstring
Java_com_example_myapplication_NativeModemCore_getEngineStatus(JNIEnv* env, jobject /* this */) {
std::string status = "JNI Bridge Active! Ядро аудиомодема готово.";
return env->NewStringUTF(status.c_str());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_myapplication_NativeModemCore_startListening(JNIEnv *env, jobject /* this */, jstring path) {
    const char *path_c_str = env->GetStringUTFChars(path, nullptr);
    std::string path_str(path_c_str);

    bool isStarted = startAudioRecord(path_str);

    env->ReleaseStringUTFChars(path, path_c_str);
    return isStarted ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_myapplication_NativeModemCore_stopListening(JNIEnv *env, jobject /* this */) {
    return stopAudioRecord() ? JNI_TRUE : JNI_FALSE;
}