/**
 * @file RealTimeTranscriber.cpp
 * @author zah
 * @brief Implementation of RealTimeTranscriber class from AssemblyAI in C++
 * @version 0.1
 * @date 2023-11-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "RealTimeTranscriber.h"


using namespace ChatBot;


RealTimeTranscriber::RealTimeTranscriber(int sample_rate)
    : m_sampleRate(sample_rate)
    , m_framesPerBuffer(static_cast<int>(sample_rate * 0.1))
{
    // Set up WebSocket++ loggers
    m_wsClient.clear_access_channels(websocketpp::log::alevel::all);
    m_wsClient.set_access_channels(websocketpp::log::alevel::fail);

    // Initialize the Asio transport policy
    m_wsClient.init_asio();

    // Register our message handler
    m_wsClient.set_message_handler(bind(&RealTimeTranscriber::on_message, this, ::_1, ::_2));
    m_wsClient.set_open_handler(bind(&RealTimeTranscriber::on_open, this, ::_1));
    m_wsClient.set_close_handler(bind(&RealTimeTranscriber::on_close, this, ::_1));
    m_wsClient.set_tls_init_handler(bind(&RealTimeTranscriber::on_tls_init, this, ::_1));

    // Initialize PortAudio
    m_audioErr = Pa_Initialize();
    if (m_audioErr != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(m_audioErr) << std::endl;
        return;
    }

    // Preallocate 
    m_audioDataBuffer.reserve(m_framesPerBuffer * m_channels * sizeof(int16_t));
    m_audioJSONBuffer["audio_data"] = "";
}

RealTimeTranscriber::~RealTimeTranscriber() {
    if (m_isConnected.load()) {
        stop_transcription(); // Stop the transcription if it's running
    }

    // Terminate PortAudio
    if (m_audioStream) { // Check if the stream was created
        Pa_CloseStream(m_audioStream); // Close the stream if it was left open
        m_audioStream = nullptr; // Reset the pointer to indicate it's closed
    }
    Pa_Terminate(); // Terminate PortAudio
}

void RealTimeTranscriber::start_transcription() {
    // Guard against starting transcription if one is already in progress
    std::lock_guard<std::mutex> lock(m_startStopMutex);
    if (m_isConnected.load()) {
        std::cerr << "Transcription is already in progress." << std::endl;
        return;
    }

    // Establish a new WebSocket connection
    std::string uri = "wss://api.assemblyai.com/v2/realtime/ws?sample_rate=" + std::to_string(m_sampleRate);
    m_con = m_wsClient.get_connection(uri, m_wsError);
    if (m_wsError) {
        std::cerr << "Could not create connection because: " << m_wsError.message() << std::endl;
        return;
    }

    m_con->append_header("Authorization", m_aaiAPItoken);
    m_wsHandle = m_con->get_handle();
    m_wsClient.connect(m_con);

    m_isConnected.store(true); // This allows the callback loop to start

    // Open an audio I/O stream.
    m_audioErr = Pa_OpenDefaultStream(
        &m_audioStream,							    // Stream pointer
        m_channels,                                 // Input m_channels
        0,                                          // Output channels
        m_format,                                   // Sample format
        m_sampleRate,		                        // Sample rate
        m_framesPerBuffer,                          // Frames per buffer
        &RealTimeTranscriber::pa_callback,          // Callback function
        this										// Callback data (this class)
    );

    if (m_audioErr != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(m_audioErr) << std::endl;
        m_wsClient.stop();
        return;
    }

    // Start the audio stream
    m_audioErr = Pa_StartStream(m_audioStream);
    if (m_audioErr != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(m_audioErr) << std::endl;
        m_wsClient.stop();
        return;
    }

    // Start the ASIO io_service run loop in a new thread if not already running
    if (m_wsThread.joinable()) {
        m_wsThread.join(); // Make sure the previous thread has finished
    }
    m_wsThread = std::thread([this] { m_wsClient.run(); });

    // Start the thread for sending audio data
    if (m_sendThread.joinable()) {
        m_sendThread.join(); // Ensure the previous sending thread has finished
    }
    m_sendThread = std::thread(&RealTimeTranscriber::send_audio_data_thread, this);

}

void RealTimeTranscriber::stop_transcription() {
    {
        std::lock_guard<std::mutex> lock(m_startStopMutex);
        // Check if the transcription is already stopped to avoid redundant operations.
        if (!m_isConnected.load()) {
            std::cerr << "No transcription is in progress to stop." << std::endl;
            return;
        }
        m_isConnected.store(false); // This stops the callback loop
        m_stopFlag.store(true); // This stops the sending thread
    }

    

    // Lock the mutex to protect against concurrent access.
    std::lock_guard<std::mutex> lock(m_wsMutex);

    // Stop and close the PortAudio stream if it's running.
    if (m_audioStream) {
        m_audioErr = Pa_StopStream(m_audioStream);
        if (m_audioErr != paNoError) {
            std::cerr << "PortAudio error when stopping the stream: " << Pa_GetErrorText(m_audioErr) << std::endl;
        }
    }

    // Set the stop flag and notify all to unblock any waiting threads
    m_stopFlag.store(true);
    m_queueCond.notify_all();
    if (m_sendThread.joinable()) {
        m_sendThread.join(); // Ensure the sending thread is finished before destruction
    }

    // Close the WebSocket connection if it's open.
    if (m_wsClient.get_con_from_hdl(m_wsHandle)->get_state() == websocketpp::session::state::open) {
        m_wsClient.close(m_wsHandle, websocketpp::close::status::going_away, "Closing connection");
    }

    // Stop the WebSocket client's ASIO io_service to allow the thread to finish
    m_wsClient.stop();
    if (m_wsThread.joinable()) {
        m_wsThread.join();
    }

    // Reset the connection handle and state
    m_con.reset();
    m_wsHandle.reset();
}

int RealTimeTranscriber::pa_callback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData) {
    auto* client = static_cast<RealTimeTranscriber*>(userData);
    return client->on_audio_data(inputBuffer, framesPerBuffer);
}

int RealTimeTranscriber::on_audio_data(const void* inputBuffer, unsigned long framesPerBuffer) {
    // Check if the transcription has been stopped and if so, return immediately.
    if (!m_isConnected.load()) {
        return paComplete; // Use paComplete to indicate the stream can be stopped.
    }

    // Before sending, check if the WebSocket connection is open and valid.
    if (m_wsClient.get_con_from_hdl(m_wsHandle)->get_state() != websocketpp::session::state::open) {
        std::cerr << "WebSocket connection is not open." << std::endl;
        return paComplete; // Use paComplete if the connection isn't open to indicate the stream should be stopped.
    }

    // Cast data passed through stream to our structure.
    const auto* in = static_cast<const int16_t*>(inputBuffer);
    m_audioDataBuffer.assign(reinterpret_cast<const char*>(in), framesPerBuffer * m_channels * sizeof(int16_t)); // Copy the audio data into the buffer
    m_audioJSONBuffer["audio_data"] = websocketpp::base64_encode(reinterpret_cast<const unsigned char*>(m_audioDataBuffer.data()), m_audioDataBuffer.size());
    enqueue_audio_data(m_audioJSONBuffer.dump());

    return paContinue;
}

// New methods for queue handling
void RealTimeTranscriber::enqueue_audio_data(const std::string& audio_data) {
    {
        std::lock_guard<std::mutex> lock(m_audioQueueMutex);
        m_audioQueue.push(audio_data);
    }
    m_queueCond.notify_all();
}

// New thread function for sending data
void RealTimeTranscriber::send_audio_data_thread() {
    websocketpp::lib::error_code ec;

    while (!m_stopFlag.load()) {
        std::string audio_data;
        {
            std::unique_lock<std::mutex> lock(m_audioQueueMutex);
            m_queueCond.wait(lock, [this] { return !m_audioQueue.empty() || m_stopFlag.load(); });
            if (m_stopFlag.load()) {
                break;
            }
            audio_data = m_audioQueue.front();
            m_audioQueue.pop();
        }
        // Send the audio data using WebSocket
        m_wsClient.send(m_wsHandle, audio_data, websocketpp::frame::opcode::text, ec);
        if (ec) {
            std::cout << "Audio Data Send failed: " << ec.message() << std::endl;
        }
    }
    // Once stop_flag is set, send the terminate message
    m_wsClient.send(m_wsHandle, m_terminateMsg, websocketpp::frame::opcode::text, ec);
    if (ec) {
        std::cerr << "Terminate Session Send failed: " << ec.message() << std::endl;
    }

}

// Define a callback to handle incoming messages
void RealTimeTranscriber::on_message(connection_hdl hdl, message_ptr msg) {
    // Parse the JSON message
    nlohmann::json json_msg = nlohmann::json::parse(msg->get_payload());

    // Extract the message type
    std::string message_type = json_msg["message_type"];

    // Handle different message types
    if (message_type == "PartialTranscript") {
        // Handle partial transcript
        std::string text = json_msg["text"];
        float confidence = json_msg["confidence"];
        std::cout << "Partial transcript: " << text << " (Confidence: " << confidence << ")" << std::endl;
    }
    else if (message_type == "FinalTranscript") {
        // Handle final transcript
        std::string text = json_msg["text"];
        float confidence = json_msg["confidence"];
        bool punctuated = json_msg["punctuated"];
        std::cout << "Final transcript: " << text << " (Confidence: " << confidence << ")" << std::endl;
    }
    else if (message_type == "SessionBegins") {
        // Handle session start
        std::string session_id = json_msg["session_id"];
        std::string expires_at = json_msg["expires_at"];
        std::cout << "Session started with ID: " << session_id << " and expires at: " << expires_at << std::endl;
    }
    else if (message_type == "SessionTerminated") {
        // Handle session termination
        std::cout << "Session terminated." << std::endl;
    }
    else {
        // Handle unknown message type
        std::cout << "Received unknown message type: " << message_type << std::endl;
    }
}

void RealTimeTranscriber::on_open(connection_hdl hdl) {
    std::cout << "Connection opened" << std::endl;
    m_isConnected.store(true);
}

void RealTimeTranscriber::on_close(connection_hdl hdl) {
    std::cout << "Connection closed" << std::endl;
    m_isConnected.store(false);
}

context_ptr RealTimeTranscriber::on_tls_init(connection_hdl hdl) {
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
    try {
        ctx->set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::single_dh_use
        );
    }
    catch (std::exception& e) {
        std::cout << "Error in context pointer: " << e.what() << std::endl;
    }
    return ctx;
}