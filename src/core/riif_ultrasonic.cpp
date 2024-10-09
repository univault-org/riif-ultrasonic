#include "riif_ultrasonic.h"
#include "fft_impl.hpp"
#include <algorithm>
#include <random>
#include <cmath>
#include <iostream>

const double PI = 3.14159265358979323846;
const int SAMPLE_RATE = 44100;
const double FREQ_0 = 20000.0;
const double FREQ_1 = 21000.0;

RiifUltrasonic::RiifUltrasonic() : ip(), w() {}

void RiifUltrasonic::ensureWorkAreaSize(int n) {
    if (ip.empty() || ip[0] < n / 2) {
        ip.resize(2 + static_cast<int>(std::sqrt(n / 2)));
        w.resize(n / 2);
        ip[0] = 0;  // Mark for initialization
    }
}

std::vector<float> RiifUltrasonic::fft(const std::vector<float>& input) {
    // Find the next power of 2
    int n = 1;
    while (n < static_cast<int>(input.size())) {
        n *= 2;
    }

    ensureWorkAreaSize(n);
    
    std::vector<float> data(n, 0.0f);  // Initialize with zeros
    std::copy(input.begin(), input.end(), data.begin());  // Copy input data

    rdft(n, 1, data.data(), ip.data(), w.data());
    
    return data;
}

std::vector<int16_t> RiifUltrasonic::encode(const std::string& message) {
    std::vector<int16_t> encoded;
    for (char c : message) {
        for (int i = 7; i >= 0; --i) {
            bool bit = (c >> i) & 1;
            double freq = bit ? FREQ_1 : FREQ_0;
            for (int j = 0; j < SAMPLE_RATE / 1000; ++j) {
                double t = static_cast<double>(j) / SAMPLE_RATE;
                encoded.push_back(static_cast<int16_t>(32767 * std::sin(2 * PI * freq * t)));
            }
        }
    }
    std::cout << "Encoded message length: " << encoded.size() << std::endl;
    return encoded;
}

std::string RiifUltrasonic::decode(const std::vector<int16_t>& signal) {
    std::string decoded;
    int samples_per_bit = SAMPLE_RATE / 1000;
    
    for (size_t i = 0; i < signal.size(); i += samples_per_bit) {
        std::vector<float> fft_input(samples_per_bit);
        for (size_t j = 0; j < fft_input.size() && i + j < signal.size(); ++j) {
            fft_input[j] = static_cast<float>(signal[i + j]) / 32767.0f;
        }

        auto fft_result = fft(fft_input);

        size_t freq_0_index = static_cast<size_t>(FREQ_0 * fft_result.size() / SAMPLE_RATE);
        size_t freq_1_index = static_cast<size_t>(FREQ_1 * fft_result.size() / SAMPLE_RATE);

        float mag_0 = std::sqrt(fft_result[2*freq_0_index]*fft_result[2*freq_0_index] + 
                                fft_result[2*freq_0_index+1]*fft_result[2*freq_0_index+1]);
        float mag_1 = std::sqrt(fft_result[2*freq_1_index]*fft_result[2*freq_1_index] + 
                                fft_result[2*freq_1_index+1]*fft_result[2*freq_1_index+1]);

        bool bit = mag_1 > mag_0;
        decoded.push_back(bit ? '1' : '0');
    }

    // Convert binary string to ASCII
    std::string result;
    for (size_t i = 0; i + 7 < decoded.length(); i += 8) {
        result.push_back(static_cast<char>(std::stoi(decoded.substr(i, 8), nullptr, 2)));
    }

    return result;
}

bool RiifUltrasonic::isUltrasonicFrequency(const std::vector<int16_t>& signal) {
    std::vector<float> fft_input(signal.size());
    for (size_t i = 0; i < signal.size(); ++i) {
        fft_input[i] = static_cast<float>(signal[i]) / 32767.0f;
    }
    auto fft_result = fft(fft_input);
    
    double max_freq = 0;
    double max_magnitude = 0;
    for (size_t i = 0; i < fft_result.size() / 2; ++i) {
        double frequency = i * SAMPLE_RATE / fft_result.size();
        double magnitude = std::sqrt(fft_result[2*i]*fft_result[2*i] + fft_result[2*i+1]*fft_result[2*i+1]);
        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            max_freq = frequency;
        }
    }
    
    return max_freq > 20000 && max_freq < 22000;
}

std::string RiifUltrasonic::generateEncryptionKey() {
    const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, charset.length() - 1);

    std::string key;
    for (int i = 0; i < 32; ++i) {
        key += charset[dis(gen)];
    }
    return key;
}

std::string RiifUltrasonic::encrypt(const std::string& data, const std::string& key) {
    std::string encrypted;
    for (size_t i = 0; i < data.length(); ++i) {
        encrypted += data[i] ^ key[i % key.length()];
    }
    return encrypted;
}

std::string RiifUltrasonic::decrypt(const std::string& encrypted_data, const std::string& key) {
    return encrypt(encrypted_data, key); // XOR encryption is its own inverse
}