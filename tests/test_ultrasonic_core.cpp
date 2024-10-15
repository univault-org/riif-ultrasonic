#include <gtest/gtest.h>
#include "../include/riif_ultrasonic.h"
#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <iomanip>
#include <random>

class RiifUltrasonicCoreTest : public ::testing::Test {
protected:
    RiifUltrasonic ultrasonic;
};

TEST(RiifUltrasonicCoreTest, SimpleBitPatternTest) {
    RiifUltrasonic riif;
    
    // Set parameters
    RiifUltrasonic::Parameters params;
    params.sampleRate = 48000;
    params.f0 = 15000.0f;
    params.df = 1000.0f;
    params.samplesPerFrame = 512;  // ~10.7 ms per bit
    riif.setParameters(params);

    // Generate a bit pattern from a short text message
    std::string message = "Hello RIIF!";
    std::vector<bool> bits;
    for (char c : message) {
        for (int i = 0; i < 8; ++i) {
            bits.push_back((c & (1 << (7-i))) != 0);  // MSB first
        }
    }

    std::cout << "Original message: " << message << std::endl;
    std::cout << "Bit pattern length: " << bits.size() << " bits" << std::endl;
    std::cout << "Original bit pattern: ";
    for (bool bit : bits) {
        std::cout << (bit ? '1' : '0');
    }
    std::cout << std::endl;

    // Generate audio samples
    std::vector<int16_t> audio_samples;
    for (bool bit : bits) {
        float frequency = bit ? params.f0 + params.df : params.f0;
        for (int i = 0; i < params.samplesPerFrame; ++i) {
            float t = static_cast<float>(i) / params.sampleRate;
            float sample = std::sin(2 * M_PI * frequency * t);
            audio_samples.push_back(static_cast<int16_t>(sample * 32767));
        }
    }

    std::cout << "Generated audio signal with " << audio_samples.size() << " samples" << std::endl;

    // Try to decode the audio signal
    std::vector<bool> decoded_bits = riif.decode(audio_samples);

    std::cout << "Decoded bit pattern: ";
    for (bool bit : decoded_bits) {
        std::cout << (bit ? '1' : '0');
    }
    std::cout << std::endl;

    // Convert decoded bits back to text
    std::string decoded_message;
    for (size_t i = 0; i < decoded_bits.size(); i += 8) {
        if (i + 8 <= decoded_bits.size()) {
            char c = 0;
            for (int j = 0; j < 8; ++j) {
                c = (c << 1) | decoded_bits[i + j];
            }
            decoded_message += c;
        }
    }
    std::cout << "Decoded message: " << decoded_message << std::endl;

    // Compare the original and decoded bit patterns
    EXPECT_EQ(bits.size(), decoded_bits.size()) << "Mismatch in number of decoded bits";
    int mismatches = 0;
    for (size_t i = 0; i < bits.size() && i < decoded_bits.size(); ++i) {
        if (bits[i] != decoded_bits[i]) {
            mismatches++;
        }
    }
    
    // Calculate bit error rate
    double bit_error_rate = static_cast<double>(mismatches) / bits.size();
    std::cout << "Bit Error Rate: " << (bit_error_rate * 100) << "%" << std::endl;

    // We'll allow a small error rate (e.g., 5%) for now
    EXPECT_LT(bit_error_rate, 0.05) << "Bit error rate too high";

    // Check if the decoded message matches the original
    EXPECT_EQ(message, decoded_message) << "Decoded message does not match original";
}

TEST(RiifUltrasonicCoreTest, LongerBitPatternTest) {
    RiifUltrasonic riif;
    
    // Set parameters
    RiifUltrasonic::Parameters params;
    params.sampleRate = 48000;
    params.f0 = 15000.0f;
    params.df = 1000.0f;
    params.samplesPerFrame = 480;  // 10 ms per bit for faster transmission
    riif.setParameters(params);

    // Generate a 128-bit random pattern
    std::vector<bool> bits(128);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);
    for (int i = 0; i < 128; ++i) {
        bits[i] = dis(gen) == 1;
    }

    // Generate audio samples
    std::vector<int16_t> audio_samples;
    for (bool bit : bits) {
        float frequency = bit ? params.f0 + params.df : params.f0;
        for (int i = 0; i < params.samplesPerFrame; ++i) {
            float t = static_cast<float>(i) / params.sampleRate;
            float sample = std::sin(2 * M_PI * frequency * t);
            audio_samples.push_back(static_cast<int16_t>(sample * 32767));
        }
    }

    std::cout << "Generated audio signal with " << audio_samples.size() << " samples" << std::endl;

    // Print the first few bits of the original pattern
    std::cout << "First 32 bits of original pattern: ";
    for (int i = 0; i < 32 && i < bits.size(); ++i) {
        std::cout << (bits[i] ? '1' : '0');
    }
    std::cout << "..." << std::endl;

    // Try to decode the audio signal
    std::vector<bool> decoded_bits = riif.decode(audio_samples);

    // Print the first few bits of the decoded pattern
    std::cout << "First 32 bits of decoded pattern: ";
    for (int i = 0; i < 32 && i < decoded_bits.size(); ++i) {
        std::cout << (decoded_bits[i] ? '1' : '0');
    }
    std::cout << "..." << std::endl;

    // Compare the original and decoded bit patterns
    EXPECT_EQ(bits.size(), decoded_bits.size()) << "Mismatch in number of decoded bits";
    int mismatches = 0;
    for (size_t i = 0; i < bits.size() && i < decoded_bits.size(); ++i) {
        if (bits[i] != decoded_bits[i]) {
            mismatches++;
        }
    }
    
    // Calculate bit error rate
    double bit_error_rate = static_cast<double>(mismatches) / bits.size();
    std::cout << "Bit Error Rate: " << (bit_error_rate * 100) << "%" << std::endl;

    // We'll allow a small error rate (e.g., 5%) for now
    EXPECT_LT(bit_error_rate, 0.05) << "Bit error rate too high";
}
