/**
* @file CallbackHandler.h
* @author zah
* @brief Header file for CallbackHandler class that handles callbacks from Python
* @version 0.1
* @date 2023-10-20
* 
* @copyright Copyright (c) 2023
* 
*/
#ifndef CALLBACKHANDLER_H
#define CALLBACKHANDLER_H

#include <string>
#include <iostream>
#include "pybind11/embed.h"
#include "pybind11/pybind11.h"

namespace py = pybind11;
using namespace py::literals;


/// @brief Enumeration for activty levels of streaming
enum Activity {
    PAUSE = 0,   ///< No transcript obtained last
    PARTIAL = 1, ///< Partial transcript obtained last
    FINAL = 2,   ///< Final transcript obtained last
};


/// @brief Class that handles callbacks from Python
class CallbackHandler {
public:
    CallbackHandler(); ///< Constructor for CallbackHandler class: initializes member variables
    ~CallbackHandler(); ///< Destructor for CallbackHandler class: frees resources

    void on_open(py::object session_id); ///< Callback for when connection is opened
    void on_data(py::object transcript); ///< Callback for when data is received
    void on_error(py::object error); ///< Callback for when an error occurs
    void on_close(); ///< Callback for when connection is closed

    const std::string& getPartialTranscript() const; ///< Returns partial transcript
    const std::string& getFinalTranscript() const; ///< Returns final transcript
    const std::string& getFullTranscript() const; ///< Returns full transcript
    const std::string& getError() const; ///< Returns error message from AssemblyAI
    Activity getActivity() const; ///< True if user isn't speaking

private:
    Activity m_activity; ///< Activity of user (partial, final, pause)
    mutable std::string m_partialTranscript; ///< Partial transcript
    mutable std::string m_finalTranscript; ///< Final transcript
    mutable std::string m_fullTranscript; ///< Full transcript
    mutable std::string m_error; ///< Error message from AssemblyAI
};
#endif // CALLBACKHANDLER_H