#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdint>

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

int main(int argc, char* argv[]) {
    char* inputPath = argv[1];
    char* outputPath = argv[2];

    std::ifstream inFile(inputPath, std::ios::binary);
    WavHeader header;
    inFile.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    uint32_t totalSamples = header.dataSize / 2;
    std::vector<int16_t> audioData(totalSamples);
    inFile.read(reinterpret_cast<char*>(audioData.data()), header.dataSize);
    inFile.close();

    const uint32_t SAMPLE_RATE = 44100;
    const double CARRIER_FREQ = 8000.0;
    const int BIT_DURATION_MS = 1;
    const int SAMPLES_PER_BIT = (SAMPLE_RATE * BIT_DURATION_MS) / 1000;

    uint64_t totalBits = totalSamples / SAMPLES_PER_BIT;
    uint64_t totalBytes = totalBits / 8;

    std::vector<uint8_t> recoveredData(totalBytes);
    uint64_t sampleIndex = 0;

    double phaseIncrement = 2.0 * M_PI * CARRIER_FREQ / SAMPLE_RATE;
    double currentPhase = 0.0;

    for(size_t i = 0; i < totalBytes; i++) {
        uint8_t byte = 0;
        
        for(int bit = 7; bit >= 0; bit--) {
            double correlationSum = 0.0;

            for(int s = 0; s < SAMPLES_PER_BIT; s++) {
                double receivedSample = audioData[sampleIndex++];
                
                correlationSum += receivedSample * std::sin(currentPhase);
                
                currentPhase += phaseIncrement;
                if (currentPhase > 2.0 * M_PI) {
                    currentPhase -= 2.0 * M_PI;
                }
            }
            
            if(correlationSum < 0) byte |= (1 << bit);
        }

        recoveredData[i] = byte;
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    outFile.write(reinterpret_cast<const char*>(recoveredData.data()), totalBytes);
    outFile.close();

    return 0;
}