/**
* @file StreamPy.h
* @author zah
* @brief Header for StreamPy class that handles transcription using Python and AssemblyAI
* @version 0.1
* @date 2023-10-20
* 
* @copyright Copyright (c) 2023
* 
*/
#ifndef STREAMPY_H
#define STREAMPY_H

#include "CallbackHandler.h"
#include "MicStream.h"

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <chrono>


#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

namespace py = pybind11;
using namespace pybind11::literals;

/// @brief Class that handles transcription using Python and AssemblyAI
class StreamPy
{
public:
	StreamPy(CallbackHandler* callbackHandler);	///< Constructor for StreamPy class: initializes member variables
	~StreamPy(); ///< Destructor for StreamPy class: stops transcription and frees resources

	void startTranscription(); ///< Starts transcription
	void stopTranscription(); ///< Stops transcription

	bool isTranscribing() const; ///< Returns whether or not transcription is currently running

private:
	// Paths required in sys.path for python interpreter
	std::string m_sitePackagesPath; ///< Path to site-packages folder (with AssemblyAI)
	// Path to python embeddable zip folder (with python3.dll) also need to add to sys

	int m_sampleRate; ///< Sample rate of audio
	bool m_isTranscribing; ///< Whether or not transcription is currently running
			 
	// Python objects that are used for transcription (start/stop)
	py::module_ m_pem; ///< Python mocule for interfaces with Python: CallbackHandler and MicrophoneStream
	py::module_ m_aai; ///< Python module for AssemblyAI
	py::object m_transcriber; ///< Python object for transcriber
			

	// Callback handler objects
	CallbackHandler* m_callbackHandler; ///< C++ callback handler object
	py::object m_pyCallbackHandler; ///< Python callback handler object 

	// Microphone stream objects
	MicrophoneStream* m_micStream; ///< C++ microphone stream object

	// C++ threading objects
	void audioProcessingThread();

	std::thread m_streamThread; ///< The thread that will run audioProcessingThread();
	std::atomic<bool> m_stopThread; ///< Atomic bool object to break the for loop in m_streamThread
	std::mutex m_micMutex; ///< C++ mutex object that can be locked to protect access to m_micStream
	std::condition_variable m_startCondition; ///< C++ condition variable to ensure that the audio processing thread starts before joining it in the stopTranscription method.

};

#endif // !STREAMPY_H
