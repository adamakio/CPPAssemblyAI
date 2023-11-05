#include "MicStream.h"



MicrophoneStream::MicrophoneStream(int sampleRate)
    : m_stream(nullptr)
    , m_sampleRate(sampleRate)
    , m_chunkSize(sampleRate * 0.1)
    , m_isRunning(false)
{
    PaError err;
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return;
    }

    m_streamParameters.device = Pa_GetDefaultInputDevice(); // Use the default input audio device
    if (m_streamParameters.device == paNoDevice) {
        std::cerr << "No default input device found!" << std::endl;
        return;
    }

    m_streamParameters.channelCount = 1; // Single channel
    m_streamParameters.sampleFormat = paInt16; // 16-bit PCM
    m_streamParameters.suggestedLatency = Pa_GetDeviceInfo(m_streamParameters.device)->defaultLowInputLatency;
    m_streamParameters.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(&m_stream, &m_streamParameters, nullptr, m_sampleRate, m_chunkSize, paClipOff, nullptr, nullptr);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return;
    }

    err = Pa_StartStream(m_stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return;
    }

    m_isRunning = true;
}

MicrophoneStream::~MicrophoneStream() {
    close();
}

std::vector<int16_t> MicrophoneStream::getNextChunk() {
    std::vector<int16_t> audioData(m_chunkSize);
    if (m_isRunning) {
        Pa_ReadStream(m_stream, audioData.data(), m_chunkSize);
    }
    return audioData;
}

void MicrophoneStream::close() {
    if (m_isRunning) {
        Pa_StopStream(m_stream);
        Pa_CloseStream(m_stream);
        Pa_Terminate();
        m_isRunning = false;
    }
}

bool MicrophoneStream::isOpen() const {
    return m_isRunning;
}


MicrophoneStream::Iterator::Iterator(MicrophoneStream& mic) : m_mic(mic) {}

std::vector<int16_t> MicrophoneStream::Iterator::operator*() {
    return m_currentChunk;
}

void MicrophoneStream::Iterator::operator++() {
    m_currentChunk = m_mic.getNextChunk();
}

bool MicrophoneStream::Iterator::operator!=(const Iterator& other) const {
    return m_mic.isOpen();
}


MicrophoneStream::Iterator MicrophoneStream::begin() {
    return MicrophoneStream::Iterator(*this);
}

MicrophoneStream::Iterator MicrophoneStream::end() {
    return MicrophoneStream::Iterator(*this);
}


