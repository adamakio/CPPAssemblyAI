#include "StreamPy.h"




PYBIND11_EMBEDDED_MODULE(pybind11_embedded_module, m)
{
    m.doc() = "Module for any interfaces with Python";

    py::class_<CallbackHandler>(m, "CallbackHandler")
        .def(py::init<>())
        .def("on_data", &CallbackHandler::on_data)
        .def("on_error", &CallbackHandler::on_error)
        .def("on_open", &CallbackHandler::on_open)
        .def("on_close", &CallbackHandler::on_close);
}


StreamPy::StreamPy(CallbackHandler* callbackHandler)
    : m_sitePackagesPath("C:\\X-Plane 12\\Resources\\plugins\\XProtection\\site-packages")
    , m_sampleRate(16'000)
    , m_isTranscribing(false)
    , m_callbackHandler(callbackHandler) 
    , m_stopThread(false)
{
    py::initialize_interpreter();
    try {
        // Add the site - packages folder to the Python path
        auto sys = py::module_::import("sys"); // import sys
        auto path_attr = sys.attr("path"); // sys.path
        path_attr.attr("append")(m_sitePackagesPath); // sys.path.append
        auto paths = py::str(path_attr).cast<std::string>(); // Get the string representation of the path attribute
        std::cout <<"Successfully appended site-packages path to sys.path : " << paths << std::endl;

        m_aai = py::module_::import("assemblyai"); // import assemblyai as aai
        m_aai.attr("settings").attr("api_key") = "fb401df1f67247c9a8aaf02d4dd785ee"; // aai.settings.api_key = ""

        // Set up the interfaces with Python
        m_pem = py::module_::import("pybind11_embedded_module");
        // m_pyCallbackHandler = py::cast(m_callbackHandler);

        std::cout << "Successfully initialized Python env" << std::endl;
    }
    catch (const py::error_already_set& e) {
        std::cout << "Python error (in StreamPy constructor): " << std::string(e.what()) << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "C++ error (in StreamPy constructor): " << std::string(e.what()) << std::endl;
    }
    catch (...) {
        std::cout << "Unknown error (in StreamPy constructor)" << std::endl;
    }
}

StreamPy::~StreamPy(){
    stopTranscription();
    py::finalize_interpreter();
}

void StreamPy::startTranscription() {
    if (!m_isTranscribing) {
        m_isTranscribing = true;
        try {
            m_transcriber = m_aai.attr("RealtimeTranscriber")(
                "sample_rate"_a = m_sampleRate,
                "on_data"_a = m_pyCallbackHandler.attr("on_data"),
                "on_error"_a = m_pyCallbackHandler.attr("on_error"),
                "on_open"_a = m_pyCallbackHandler.attr("on_open"),
                "on_close"_a = m_pyCallbackHandler.attr("on_close")
                );
            m_transcriber.attr("connect")();
            m_micStream = new MicrophoneStream(m_sampleRate);
            m_streamThread = std::thread(&StreamPy::audioProcessingThread, this);

            std::cout << "Successfully started transcribing" << std::endl;
        }
        catch (const py::error_already_set& e) {
            std::cout << "Python error (in StreamPy start): " << std::string(e.what()) << std::endl;
            m_isTranscribing = false;
        }
        catch (const std::exception& e) {
            std::cout << "C++ error (in StreamPy start): " << std::string(e.what()) << std::endl;
            m_isTranscribing = false;
        }
        catch (...) {
            std::cout << "Unknown error (in StreamPy start)" << std::endl;
            m_isTranscribing= false;
        }
    }
}

void StreamPy::audioProcessingThread() {
    m_startCondition.notify_all(); // Notify that the thread has started

    for (auto it = m_micStream->begin(); !m_stopThread.load(); ++it) {
        // Process the audio data
        std::vector<int16_t> audioChunk = *it;

        // Convert the audio data to bytes
        std::vector<uint8_t> bytesData(audioChunk.size() * sizeof(int16_t));
        std::memcpy(bytesData.data(), audioChunk.data(), bytesData.size());

        // Protect micStream with a mutex
        std::lock_guard<std::mutex> lock(m_micMutex);

        // Pass the data as a Python bytes object using Pybind11
        m_transcriber.attr("stream")(py::bytes(reinterpret_cast<char*>(bytesData.data()), bytesData.size()));

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void StreamPy::stopTranscription() {
    if (m_isTranscribing) { 
        try {
            m_stopThread.store(true);
            m_startCondition.notify_all(); // Notify the thread to check for stop condition
            m_streamThread.join();

            std::lock_guard<std::mutex> lock(m_micMutex);
            if (m_micStream) {
                m_micStream->close();
                delete m_micStream;
                m_micStream = nullptr;
            }

            m_transcriber.attr("close")();
            m_stopThread.store(false);
            m_isTranscribing = false;

            std::cout << "Successfully stopped transcribing" << std::endl;
        }
        catch (const py::error_already_set& e) {
            std::cout << "Python error (in StreamPy stop): " << std::string(e.what()) << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "C++ error (in StreamPy stop): " << std::string(e.what()) << std::endl;
        }
        catch (...) {
            std::cout << "Unknown error (in StreamPy stop)" << std::endl;
        }
    }
}




bool StreamPy::isTranscribing() const {
    return m_isTranscribing;
}