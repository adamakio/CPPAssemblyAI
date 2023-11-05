#include "CallbackHandler.h"


CallbackHandler::CallbackHandler()
    : m_partialTranscript("")
    , m_finalTranscript("")
    , m_fullTranscript("")
    , m_error("")
    , m_activity(Activity::PAUSE) {}

CallbackHandler::~CallbackHandler() {}

void CallbackHandler::on_open(py::object session_opened) {
    std::string session_id = py::str(session_opened.attr("session_id")).cast<std::string>();;
    std::cout << "Session opened" << std::endl;
}
void CallbackHandler::on_data(py::object transcript) {
    // Get text 
    std::string text = py::str(transcript.attr("text")).cast<std::string>();

    // Call the appropriate function to handle the text 
    if (text.empty()) {
        m_activity = Activity::PAUSE;
        return;
    }

    // If there is data, differentiate between partial and final transcripts
    std::string message_type = transcript.attr("message_type").cast<std::string>();
    if (message_type == "FinalTranscript") {
        m_activity = Activity::FINAL;
        m_finalTranscript = text;
        m_fullTranscript += text;
        std::cout << "Updated full transcript with : " << m_finalTranscript << std::endl;
    }
    else {
        m_activity = Activity::PARTIAL;
        m_partialTranscript = text;
        std::cout << "Updated partial transcript : " << m_partialTranscript << std::endl;
    }
}

void CallbackHandler::on_error(py::object error) {
    m_error = py::str(error).cast<std::string>();
    std::cout << "AssemblyAI Error: " << m_error << std::endl;
}

void CallbackHandler::on_close() {
    std::cout << "Session closed" << std::endl;
}

const std::string& CallbackHandler::getPartialTranscript() const {
    return m_partialTranscript;
}

const std::string& CallbackHandler::getFinalTranscript() const {
    return m_finalTranscript;
}

const std::string& CallbackHandler::getFullTranscript() const {
    return m_fullTranscript;
}

const std::string& CallbackHandler::getError() const {
	return m_error;
}

Activity CallbackHandler::getActivity() const {
    return m_activity;
}
