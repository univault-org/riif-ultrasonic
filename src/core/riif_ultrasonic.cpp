#include "riif_ultrasonic.h"
#include "fft_impl.hpp"
#include <cmath>
#include <algorithm>
#include <random>
#include <complex>
#include <iostream>
#include <iomanip>
#include <bitset>
#include <deque>

const double PI = 3.14159265358979323846;

RiifUltrasonic::RiifUltrasonic() : m_current_byte(0), m_bit_count(0)
{
    m_params = {
        DEFAULT_SAMPLE_RATE,
        DEFAULT_SAMPLES_PER_FRAME,
        DEFAULT_BITS_IN_MARKER,
        DEFAULT_MARKER_FRAMES,
        DEFAULT_F0,
        DEFAULT_DF,
        DEFAULT_NUM_FREQS,
        DEFAULT_RS_MSG_LENGTH,
        DEFAULT_RS_ECC_LENGTH,
        DEFAULT_PREAMBLE_DURATION};
    initializeFrequencies();
    rs_work_buffer = new uint8_t[RS::ReedSolomon::getWorkSize_bytes(m_params.rsMsgLength, m_params.rsEccLength)];
    rs = new RS::ReedSolomon(m_params.rsMsgLength, m_params.rsEccLength, rs_work_buffer);
}

RiifUltrasonic::~RiifUltrasonic()
{
    delete rs;
    delete[] rs_work_buffer;
}

void RiifUltrasonic::setParameters(const Parameters &params)
{
    m_params = params;
    initializeFrequencies();
    delete rs;
    delete[] rs_work_buffer;
    rs_work_buffer = new uint8_t[RS::ReedSolomon::getWorkSize_bytes(m_params.rsMsgLength, m_params.rsEccLength)];
    rs = new RS::ReedSolomon(m_params.rsMsgLength, m_params.rsEccLength, rs_work_buffer);
}

void RiifUltrasonic::initializeFrequencies()
{
    m_frequencies.clear();
    for (int i = 0; i < m_params.numFreqs; ++i)
    {
        m_frequencies.push_back(m_params.f0 + i * m_params.df);
    }
}

std::vector<int16_t> RiifUltrasonic::encode(const std::string &message)
{
    std::vector<uint8_t> data(message.begin(), message.end());
    std::vector<uint8_t> encoded = rsEncode(data);

    std::vector<int> tones;
    generateTones(encoded, tones);
    std::vector<int16_t> signal;
    generateWaveform(tones, signal);

    size_t minSize = m_params.samplesPerFrame * (tones.size() + m_params.preambleDuration / m_params.samplesPerFrame);
    signal.resize(minSize, 0);

    return signal;
}

void RiifUltrasonic::generateTones(const std::vector<uint8_t> &encoded, std::vector<int> &tones)
{
    tones.clear();
    tones.reserve(encoded.size() * 8);

    for (uint8_t byte : encoded)
    {
        for (int i = 7; i >= 0; --i)
        {
            int bit = (byte >> i) & 1;
            tones.push_back(bit);
        }
    }
}

void RiifUltrasonic::generateWaveform(const std::vector<int> &tones, std::vector<int16_t> &signal)
{
    signal.clear();
    signal.reserve(tones.size() * m_params.samplesPerFrame);

    double phase = 0.0;
    double phaseIncrement0 = 2 * M_PI * m_params.f0 / m_params.sampleRate;
    double phaseIncrement1 = 2 * M_PI * (m_params.f0 + m_params.df) / m_params.sampleRate;

    for (size_t j = 0; j < tones.size(); ++j)
    {
        int tone = tones[j];
        double phaseIncrement = tone == 0 ? phaseIncrement0 : phaseIncrement1;

        for (int i = 0; i < m_params.samplesPerFrame; ++i)
        {
            double sample = std::sin(phase);

            // Apply smoothing
            double smoothFactor = 0.5 * (1 - std::cos(2 * M_PI * i / m_params.samplesPerFrame));
            sample *= smoothFactor;

            signal.push_back(static_cast<int16_t>(sample * 32767));

            phase += phaseIncrement;
            if (phase >= 2 * M_PI)
            {
                phase -= 2 * M_PI;
            }
        }
    }
}

std::vector<uint8_t> RiifUltrasonic::rsEncode(const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> encoded(m_params.rsMsgLength + m_params.rsEccLength, 0);
    rs->Encode(data.data(), encoded.data());
    return encoded;
}

std::vector<bool> RiifUltrasonic::decode(const std::vector<int16_t>& signal) {
    std::vector<float> normalizedSignal;
    normalizeAmplitude(signal, normalizedSignal);

    std::vector<bool> decoded_bits;
    size_t frame_size = m_params.samplesPerFrame;

    for (size_t i = 0; i < normalizedSignal.size(); i += frame_size) {
        size_t frame_end = std::min(i + frame_size, normalizedSignal.size());
        std::vector<float> frame(normalizedSignal.begin() + i, normalizedSignal.begin() + frame_end);
        
        auto fft_result = performFFT(frame);
        auto demodulated_data = demodulateFFT(fft_result);

        if (!demodulated_data.empty()) {
            decoded_bits.push_back(demodulated_data[0] == 1);
        } else {
            decoded_bits.push_back(false);  // Default to 0 if no bit detected
        }
    }

    return decoded_bits;
}

std::vector<uint8_t> RiifUltrasonic::rsDecode(const std::vector<uint8_t> &encoded_data)
{
    if (encoded_data.size() != m_params.rsMsgLength + m_params.rsEccLength)
    {
        return std::vector<uint8_t>();
    }

    std::vector<uint8_t> decoded(m_params.rsMsgLength, 0);
    int result = rs->Decode(encoded_data.data(), decoded.data());

    if (result != 0)
    {
        return std::vector<uint8_t>();
    }

    return decoded;
}

void RiifUltrasonic::addPreamble(std::vector<int16_t> &signal)
{
    for (int i = 0; i < m_params.preambleDuration; ++i)
    {
        double t = static_cast<double>(i) / m_params.sampleRate;
        double freq = m_frequencies[0] + (m_frequencies[m_params.numFreqs - 1] - m_frequencies[0]) * i / m_params.preambleDuration;
        signal.push_back(static_cast<int16_t>(32767 * std::sin(2 * PI * freq * t)));
    }
}

void RiifUltrasonic::addTone(std::vector<int16_t> &signal, double freq, int duration)
{
    for (int i = 0; i < duration; ++i)
    {
        double t = static_cast<double>(i) / m_params.sampleRate;
        int16_t sample = static_cast<int16_t>(32767 * std::sin(2 * PI * freq * t));
        signal.push_back(sample);
    }
}

size_t RiifUltrasonic::detectPreamble(const std::vector<float> &signal)
{
    float threshold = 0.1f; // Adjust this value based on your signal characteristics
    for (size_t i = 0; i < signal.size() - m_params.preambleDuration; ++i)
    {
        float energy = 0;
        for (size_t j = 0; j < m_params.preambleDuration; ++j)
        {
            energy += signal[i + j] * signal[i + j];
        }
        if (energy > threshold * m_params.preambleDuration)
        {
            return i;
        }
    }
    return signal.size();
}

void RiifUltrasonic::prepareFFT(int n)
{
    m_fftWorkArea.resize(2 + std::sqrt(n / 2));
    m_fftSinCosTable.resize(n / 2);
    m_fftWorkArea[0] = 0; // First time initialization
}

std::vector<std::complex<float>> RiifUltrasonic::performFFT(const std::vector<float> &frame)
{
    int n = 1024; // Fixed size to ensure consistency
    
    if (frame.size() > n) {
        throw std::runtime_error("Input frame size exceeds FFT size");
    }

    if (m_fftWorkArea.empty() || m_fftSinCosTable.empty() || m_fftWorkArea.size() != n * 5 / 4)
    {
        prepareFFT(n);
    }

    std::vector<float> fftInput(n, 0.0f);
    std::copy(frame.begin(), frame.end(), fftInput.begin());

    rdft(n, 1, fftInput.data(), m_fftWorkArea.data(), m_fftSinCosTable.data());

    std::vector<std::complex<float>> result(n / 2 + 1);
    result[0] = std::complex<float>(fftInput[0], 0);     // DC component
    result[n / 2] = std::complex<float>(fftInput[1], 0); // Nyquist frequency

    for (int i = 1; i < n / 2; ++i)
    {
        result[i] = std::complex<float>(fftInput[2 * i], fftInput[2 * i + 1]);
    }

    return result;
}

std::vector<uint8_t> RiifUltrasonic::demodulateFFT(const std::vector<std::complex<float>> &fft_result)
{
    std::vector<uint8_t> demodulated;
    const float magnitude_threshold = 0.1f;
    const float relative_threshold = 1.2f;

    size_t fft_size = (fft_result.size() - 1) * 2;
    size_t bin0_center = static_cast<size_t>(m_params.f0 * fft_size / m_params.sampleRate);
    size_t bin1_center = static_cast<size_t>((m_params.f0 + m_params.df) * fft_size / m_params.sampleRate);

    float mag0 = std::abs(fft_result[bin0_center]);
    float mag1 = std::abs(fft_result[bin1_center]);

    int detected_bit = -1;
    if (mag0 > magnitude_threshold || mag1 > magnitude_threshold)
    {
        detected_bit = (mag1 > mag0 * relative_threshold) ? 1 : 0;
    }
    else
    {
        detected_bit = 0; // Default to 0 if neither magnitude is significant
    }

    demodulated.push_back(detected_bit);
    return demodulated;
}

int RiifUltrasonic::findDominantFrequency(const std::vector<std::complex<float>> &fft_result)
{
    // Implement frequency detection logic here
    // For now, we'll just return 0
    return 0;
}

void RiifUltrasonic::normalizeAmplitude(const std::vector<int16_t> &input, std::vector<float> &output)
{
    output.resize(input.size());

    for (size_t i = 0; i < input.size(); ++i)
    {
        output[i] = input[i] / 32768.0f;
    }
}

bool RiifUltrasonic::processFrame(const std::vector<float> &frame)
{
    auto fft_result = performFFT(frame);
    auto demodulated_data = demodulateFFT(fft_result);

    m_rxBuffer.insert(m_rxBuffer.end(), demodulated_data.begin(), demodulated_data.end());

    return true;
}

std::vector<uint8_t> RiifUltrasonic::demodulateFrame(const std::vector<float> &frame)
{
    std::vector<std::complex<float>> fftResult = performFFT(frame);
    std::vector<uint8_t> demodulated = demodulateFFT(fftResult);
    return demodulated;
}

std::vector<float> RiifUltrasonic::calculateAverageSpectrum()
{
    std::vector<float> avg_spectrum(m_spectrum_history[0].size(), 0.0f);
    for (const auto &spectrum : m_spectrum_history)
    {
        for (size_t i = 0; i < spectrum.size(); ++i)
        {
            avg_spectrum[i] += std::abs(spectrum[i]);
        }
    }
    for (auto &mag : avg_spectrum)
    {
        mag /= m_spectrum_history.size();
    }
    return avg_spectrum;
}

std::vector<float> RiifUltrasonic::normalizeSpectrum(const std::vector<float> &spectrum)
{
    float max_magnitude = *std::max_element(spectrum.begin(), spectrum.end());
    std::vector<float> normalized_spectrum(spectrum.size());
    for (size_t i = 0; i < spectrum.size(); ++i)
    {
        normalized_spectrum[i] = spectrum[i] / max_magnitude;
    }
    return normalized_spectrum;
}