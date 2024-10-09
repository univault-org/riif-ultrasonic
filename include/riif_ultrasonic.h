#pragma once

#include <string>
#include <vector>
#include <cstdint>

class RiifUltrasonic {
public:
    RiifUltrasonic();  // Add constructor declaration
    std::vector<int16_t> encode(const std::string& message);
    std::string decode(const std::vector<int16_t>& signal);
    std::string generateEncryptionKey();
    std::string encrypt(const std::string& data, const std::string& key);
    std::string decrypt(const std::string& encrypted_data, const std::string& key);
    bool isUltrasonicFrequency(const std::vector<int16_t>& signal);

private:
    std::vector<float> fft(const std::vector<float>& input);  // Change to float
    void ensureWorkAreaSize(int n);  // Add this method
    std::vector<int> ip;  // Add these member variables
    std::vector<float> w;
};