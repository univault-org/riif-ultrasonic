#pragma once

#include <vector>
#include <string>
#include <complex>
#include <deque>  // Add this include for std::deque
#include "../src/reed-solomon/rs.hpp"

class RiifUltrasonic {
public:
    RiifUltrasonic();
    ~RiifUltrasonic();
    
    std::vector<int16_t> encode(const std::string& message);
    std::vector<bool> decode(const std::vector<int16_t>& signal);

    struct Parameters {
        int sampleRate;
        int samplesPerFrame;
        int nBitsInMarker;
        int nMarkerFrames;
        double f0;
        double df;
        int numFreqs;
        int rsMsgLength;
        int rsEccLength;
        int preambleDuration;
    };

    void setParameters(const Parameters& params);
    const Parameters& getParameters() const;

private:
    Parameters m_params;
    static constexpr int DEFAULT_SAMPLE_RATE = 48000;
    static constexpr int DEFAULT_SAMPLES_PER_FRAME = 1024;
    static constexpr int DEFAULT_BITS_IN_MARKER = 16;
    static constexpr int DEFAULT_MARKER_FRAMES = 8;
    static constexpr double DEFAULT_F0 = 15000.0;
    static constexpr double DEFAULT_DF = 500.0; 
    static constexpr int DEFAULT_NUM_FREQS = 2;
    static constexpr int DEFAULT_RS_MSG_LENGTH = 223;
    static constexpr int DEFAULT_RS_ECC_LENGTH = 32;
    static constexpr int DEFAULT_PREAMBLE_DURATION = 256;

    std::vector<double> m_frequencies;
    RS::ReedSolomon* rs;
    uint8_t* rs_work_buffer;
    std::vector<float> m_tx_output;

    void initializeFrequencies();
    std::vector<uint8_t> rsEncode(const std::vector<uint8_t>& data);
    std::vector<uint8_t> rsDecode(const std::vector<uint8_t>& encoded_data);
    void generateTones(const std::vector<uint8_t>& encoded, std::vector<int>& tones);
    void generateWaveform(const std::vector<int>& tones, std::vector<int16_t>& signal);
    void addPreamble(std::vector<int16_t>& signal);

    // Decoding functions
    size_t detectPreamble(const std::vector<float>& signal);
    std::vector<std::complex<float>> performFFT(const std::vector<float>& frame);
    std::vector<uint8_t> demodulateFFT(const std::vector<std::complex<float>>& fft_result);
    int findDominantFrequency(const std::vector<std::complex<float>>& fft_result);

    void addTone(std::vector<int16_t>& signal, double freq, int duration);

    // FFT-related members
    std::vector<int> m_fftWorkArea;
    std::vector<float> m_fftSinCosTable;

    // FFT functions
    void prepareFFT(int n);

    // Add these new function declarations in the private section:
    std::vector<float> m_rxBuffer;
    size_t m_rxBufferOffset;

    void normalizeAmplitude(const std::vector<int16_t>& input, std::vector<float>& output);
    bool processFrame(const std::vector<float>& frame);
    std::vector<uint8_t> demodulateFrame(const std::vector<float>& frame);

    uint8_t m_current_byte;
    int m_bit_count;

    // Add these new members for spectrum history
    std::deque<std::vector<std::complex<float>>> m_spectrum_history;
    static constexpr int SPECTRUM_HISTORY_SIZE = 5;  // You can adjust this value

    // You might want to add or modify these function declarations
    std::vector<float> calculateAverageSpectrum();
    std::vector<float> normalizeSpectrum(const std::vector<float>& spectrum);
};
