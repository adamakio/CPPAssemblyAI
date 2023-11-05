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

    while (true) {
        std::cout << "Press enter to start transcription\n";
        std::cin.get();
        
        {
            auto start_time = std::chrono::high_resolution_clock::now(); // Start timing
            transcriber = std::make_unique<RealTimeTranscriber>(SAMPLE_RATE);
            transcriber->start_transcription();
            auto end_time = std::chrono::high_resolution_clock::now(); // End timing
            std::cout << "Transcription started in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << "ms" << std::endl;
        }

        std::cout << "Press enter to stop transcription\n";
        std::cin.get();
        
        {
            auto start_time = std::chrono::high_resolution_clock::now(); // Start timing
            transcriber->stop_transcription();
            auto end_time = std::chrono::high_resolution_clock::now(); // End timing
            std::cout << "Transcription stopped in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << "ms" << std::endl;
        }
        
        std::string input;
        std::cout << "Enter q to exit and c to continue: ";
        std::getline(std::cin, input);
        if (input == "q")
            break;
    }
    return 0;
}
