#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#pragma pack(push, 1)
struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;      
    uint16_t numChannels = 1;     
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize;
};
#pragma pack(pop)

bool modulate(const std::string& inputPath, const std::string& outputPath) {
    std::ifstream inFile(inputPath, std::ios::binary | std::ios::ate);
    if (!inFile.is_open()) return false;

    std::streamsize originalSize = inFile.tellg();
    inFile.seekg(0, std::ios::beg);

    // Добавляем 1 пустой референсный байт (0x00) в начало.
    // Он нужен демодулятору для сравнения фазы самого первого бита данных.
    size_t fileSize = originalSize + 1;
    std::vector<uint8_t> buffer(fileSize);
    buffer[0] = 0x00;
    inFile.read(reinterpret_cast<char*>(buffer.data() + 1), originalSize);
    inFile.close();

    const uint32_t SAMPLE_RATE = 44100;
    const double CARRIER_FREQ = 8000.0;
    const int BIT_DURATION_MS = 1;
    const int SAMPLES_PER_BIT = (SAMPLE_RATE * BIT_DURATION_MS) / 1000;
    const double AMPLITUDE = 30000.0;

    uint64_t totalBits = fileSize * 8;
    uint64_t totalSamples = totalBits * SAMPLES_PER_BIT;
    uint32_t dataSize = totalSamples * 2;

    std::vector<int16_t> audioData(totalSamples);
    uint64_t sampleIndex = 0;

    const double phaseIncrement = 2.0 * M_PI * CARRIER_FREQ / SAMPLE_RATE;
    double currentPhase = 0.0;

    // Переменная для накопления сдвига фазы (DBPSK)
    double phaseOffset = 0.0;

    for(size_t i = 0; i < fileSize; i++) {
        uint8_t byte = buffer[i];

        for(int bit = 7; bit >= 0; bit--) {
            bool isOne = (byte >> bit) & 1;

            // Если бит равен 1, инвертируем фазу на 180 градусов.
            // Если бит 0, фаза остается такой же, как у предыдущего бита.
            if (isOne) {
                phaseOffset += M_PI;
            }

            for(int s = 0; s < SAMPLES_PER_BIT; s++) {
                double sample = AMPLITUDE * std::sin(currentPhase + phaseOffset);
                audioData[sampleIndex++] = static_cast<int16_t>(sample);

                currentPhase += phaseIncrement;
                if (currentPhase > 2.0 * M_PI) {
                    currentPhase -= 2.0 * M_PI;
                }
            }
        }
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    WavHeader header;
    header.sampleRate = SAMPLE_RATE;
    header.byteRate = SAMPLE_RATE * 1 * 16 / 8;
    header.blockAlign = 1 * 16 / 8;
    header.dataSize = dataSize;
    header.fileSize = 36 + dataSize;

    outFile.write(reinterpret_cast<const char*>(&header), sizeof(WavHeader));
    outFile.write(reinterpret_cast<const char*>(audioData.data()), dataSize);
    outFile.close();

    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: modulator <input_file> <output_wav>\n";
        return 1;
    }

    if (modulate(argv[1], argv[2])) {
        return 0;
    } else {
        std::cerr << "Modulation failed.\n";
        return 1;
    }
}