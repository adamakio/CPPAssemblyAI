#ifndef MICROPHONE_STREAM_H
#define MICROPHONE_STREAM_H

#include <iostream>
#include <vector>
#include <portaudio.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;



class MicrophoneStream {
public:
    MicrophoneStream(int sampleRate);
    ~MicrophoneStream();

    std::vector<int16_t> getNextChunk();

    bool isOpen() const;

    void close();

    class Iterator {
    public:
        Iterator(MicrophoneStream& mic);

        std::vector<int16_t> operator*();

        void operator++();

        bool operator!=(const Iterator& other) const;


    private:
        MicrophoneStream& m_mic;
        std::vector<int16_t> m_currentChunk;
    };

    Iterator begin();
    Iterator end();

private:
    PaStream* m_stream;
    PaStreamParameters m_streamParameters;
    int m_sampleRate;
    int m_chunkSize;
    bool m_isRunning;
};


#endif // MICROPHONE_STREAM_H
