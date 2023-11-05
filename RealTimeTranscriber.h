/**
* @file RealTimeTranscriber.h
* @author zah
* @brief Header for RealTimeTranscriber class from AssemblyAI in C++
* @version 0.1
* @date 2023-10-20
*
* @copyright Copyright (c) 2023
*
*/

#ifndef REALTIMETRANSCRIBER_H
#define REALTIMETRANSCRIBER_H

#include "portaudio.h"
#include <nlohmann/json.hpp>
#include <websocketpp/base64/base64.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <thread>
#include <atomic>
#include <mutex>

namespace ChatBot {
    typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
    typedef client::message_ptr message_ptr;
    typedef websocketpp::connection_hdl connection_hdl;
    typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;

    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;

    /// @brief Class for transcribing audio in real time using AssemblyAI API
    class RealTimeTranscriber
    {
    public:
        RealTimeTranscriber(int sample_rate);
        ~RealTimeTranscriber();

        void start_transcription();
        void stop_transcription();

    private:
        static int pa_callback(const void* inputBuffer, void* outputBuffer,
            unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void* userData);

        int on_audio_data(const void* inputBuffer, unsigned long framesPerBuffer);

        void enqueue_audio_data(const std::string& audio_data);

        std::string dequeue_audio_data();

        void send_audio_data_thread();

        // Define a callback to handle incoming messages
        void on_message(connection_hdl hdl, message_ptr msg);

        void on_open(connection_hdl hdl);
        void on_close(connection_hdl hdl);

        context_ptr on_tls_init(connection_hdl hdl);

        // WebSocket client and connection handle
        client m_wsClient; ///< WebSocket client
        client::connection_ptr m_con = nullptr; ///< WebSocket connection pointer
        connection_hdl m_wsHandle; ///< WebSocket connection handle
        websocketpp::lib::error_code m_wsError; ///< WebSocket error code

        // Termination message created in constructor
        nlohmann::json m_terminateJSON{ {"terminate_session", true} }; ///< JSON payload for terminating session
        std::string m_terminateMsg{ m_terminateJSON.dump() }; ///< Terminate session message

        // Audio buffers for sending data are initialized in constructor to avoid reallocation
        std::mutex m_audioBufferMutex; ///< Mutex for protecting audio buffers
        std::string m_audioDataBuffer; ///< Buffer for audio data
        nlohmann::json m_audioJSONBuffer; ///< Buffer for audio JSON payload

        // Thread and mutex for synchronization
        std::thread m_wsThread; ///< Thread for running the WebSocket client's ASIO io_service
        std::mutex m_wsMutex; ///< Mutex to protect against concurrent access to WebSocket connection
        std::atomic<bool> m_isConnected{ false }; ///< Indicates if the WebSocket connection is open

        // Queue and condition variable for synchronization
        std::thread m_sendThread; ///< Thread for sending audio data
        std::queue<std::string> m_audioQueue; ///< Queue for audio data
        std::condition_variable m_queueCond; ///< Condition variable for queue
        std::atomic<bool> m_stopFlag{ false }; ///< Indicates if the transcription has been stopped

        // PortAudio stream
        std::mutex m_audioStreamMutex; ///< Mutex for protecting the audio stream
        PaStream* m_audioStream{ nullptr }; ///< PortAudio stream pointer
        PaError m_audioErr{ paNoError }; ///< PortAudio error code

        // Configuration parameters
        const std::string m_aaiAPItoken; ///< We'll want this to be configurable
        const int m_sampleRate; ///< 16kHz is adequate for speech recognition
        const int m_framesPerBuffer; ///< 100ms to 2000ms of audio data per message (0.1 * sampleRate -> 2.0 * sampleRate)
        const PaSampleFormat m_format{ paInt16 }; ///< WAV PCM16
        const int m_channels{ 1 }; ///< Mono (single-channel)
    };
} // namespace ChatBot
#endif // !REALTIMETRANSCRIBER_H

