/**
* @file CPPAssemblyAI.cpp
* @author zah
* @brief Implementation of Assembly AI Realtime API in pure C++
* @version 0.1
* @date 2023-10-20
*
* @copyright Copyright (c) 2023
*
*/

#include "RealTimeTranscriber.h"

using namespace ChatBot;

int main() {
    const int SAMPLE_RATE = 16000;
    std::unique_ptr<RealTimeTranscriber> transcriber; // Use smart pointer to manage resource
    char control = ' ';

    while (true) {
        std::cout << "Press 's' to start transcription or 'q' to quit: ";
        std::cin >> control;

        if (control == 's') {
            auto start_time = std::chrono::high_resolution_clock::now(); // Start timing
            transcriber = std::make_unique<RealTimeTranscriber>(SAMPLE_RATE);
            transcriber->start_transcription();
            auto end_time = std::chrono::high_resolution_clock::now(); // End timing
            std::cout << "Transcription started in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << "ms" << std::endl;

            std::cout << "Transcription started. Press 'e' to end transcription: ";
            std::cin >> control;
            if (control == 'e') {
                auto start_time = std::chrono::high_resolution_clock::now(); // Start timing
                transcriber->stop_transcription();
                auto end_time = std::chrono::high_resolution_clock::now(); // End timing
                std::cout << "Transcription stopped in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << "ms" << std::endl;
                std::cout << "Transcription stopped." << std::endl;
            }
        }
        else if (control == 'q') {
            std::cout << "Exiting..." << std::endl;
            break;
        }
    }
    return 0;
}
