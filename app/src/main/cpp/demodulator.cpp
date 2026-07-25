#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#pragma pack(push, 1)
struct WavHeader {
    char riff[4];
    uint32_t fileSize;
    char wave[4];
    char fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];
    uint32_t dataSize;
};
#pragma pack(pop)

bool demodulate(const std::string& inputPath, const std::string& outputPath) {
    std::ifstream inFile(inputPath, std::ios::binary);
    if (!inFile.is_open()) return false;

    WavHeader header;
    inFile.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    uint32_t totalSamples = header.dataSize / 2;
    std::vector<int16_t> audioData(totalSamples);
    inFile.read(reinterpret_cast<char*>(audioData.data()), header.dataSize);
    inFile.close();

    const uint32_t SAMPLE_RATE = 44100;
    const int BIT_DURATION_MS = 1;
    const int SAMPLES_PER_BIT = (SAMPLE_RATE * BIT_DURATION_MS) / 1000;

    // ШАГ 1: Временная синхронизация (поиск начала сигнала по энергии)
    double maxAmp = 0.0;
    for (int16_t s : audioData) {
        if (std::abs(s) > maxAmp) maxAmp = std::abs(s);
    }

    // Порог: 20% от максимальной громкости записи
    double threshold = maxAmp * 0.2;
    int window = SAMPLES_PER_BIT;
    long long energySum = 0;

    for (int i = 0; i < window && i < audioData.size(); i++) {
        energySum += std::abs(audioData[i]);
    }

    uint64_t startIndex = 0;
    for (size_t i = 0; i < audioData.size() - window; i++) {
        if ((double)energySum / window > threshold) {
            startIndex = i;
            // Делаем шаг назад, чтобы точно захватить начало первой синусоиды
            startIndex = (startIndex > 2) ? startIndex - 2 : 0;
            break;
        }
        energySum -= std::abs(audioData[i]);
        energySum += std::abs(audioData[i + window]);
    }

    // ШАГ 2: Дифференциальная демодуляция (DBPSK)
    uint64_t availableSamples = audioData.size() - startIndex;
    uint64_t totalBits = availableSamples / SAMPLES_PER_BIT;
    uint64_t totalBytes = totalBits / 8;

    if (totalBytes == 0) return false;

    std::vector<uint8_t> recoveredData;
    uint64_t sampleIndex = startIndex;

    for(size_t i = 0; i < totalBytes; i++) {
        uint8_t byte = 0;

        for(int bit = 7; bit >= 0; bit--) {
            double correlationSum = 0.0;

            for(int s = 0; s < SAMPLES_PER_BIT; s++) {
                double currSample = audioData[sampleIndex + s];
                double prevSample = 0.0;

                // Умножаем текущий сэмпл на сэмпл ровно один бит назад
                if (sampleIndex + s >= SAMPLES_PER_BIT) {
                    prevSample = audioData[sampleIndex + s - SAMPLES_PER_BIT];
                }

                correlationSum += currSample * prevSample;
            }

            // Отрицательная корреляция означает инверсию фазы (бит 1)
            if (correlationSum < 0) {
                byte |= (1 << bit);
            }
            sampleIndex += SAMPLES_PER_BIT;
        }

        // Первый принятый байт был техническим (пустым), пропускаем его
        if (i > 0) {
            recoveredData.push_back(byte);
        }
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    outFile.write(reinterpret_cast<const char*>(recoveredData.data()), recoveredData.size());
    outFile.close();

    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: demodulator <input_wav> <output_file>\n";
        return 1;
    }

    if (demodulate(argv[1], argv[2])) {
        return 0;
    } else {
        std::cerr << "Demodulation failed.\n";
        return 1;
    }
}