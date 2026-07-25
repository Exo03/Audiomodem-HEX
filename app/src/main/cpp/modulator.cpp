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

int main(int argc, char* argv[]) {
    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

    std::ifstream inFile(inputPath, std::ios::binary | std::ios::ate);

    std::streamsize fileSize = inFile.tellg();
    inFile.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(fileSize);
    inFile.read(reinterpret_cast<char*>(buffer.data()), fileSize);
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

    for(size_t i = 0; i < fileSize; i++) {
        uint8_t byte = buffer[i];

        for(int bit = 7; bit >= 0; bit--) {
            bool isOne = (byte >> bit) & 1;
            
            double phaseOffset = isOne ? M_PI : 0.0;

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

    return 0;
}