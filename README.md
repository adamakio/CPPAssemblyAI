# Real-Time Transcription Client

XProtection is a real-time audio transcription client using WebSocket and PortAudio libraries to provide a seamless audio-to-text conversion experience. This client is designed to connect to the AssemblyAI API for real-time transcription services.

## Features

- Real-time audio streaming to WebSocket server.
- Audio capture with PortAudio library.
- Thread-safe audio data queue management.
- Secure WebSocket connection with TLS support.
- JSON-based message handling for transcription results.

## Prerequisites

Before you begin, ensure you have met the following requirements:

- C++17 compatible compiler
- [PortAudio](http://www.portaudio.com/) installed on your system
- [nlohmann/json](https://github.com/nlohmann/json) for JSON parsing
- [websocketpp](https://github.com/zaphoyd/websocketpp) for WebSocket communication
- [Boost](https://www.boost.org/) libraries for Asio and SSL support

## Installation

To install XProtection, clone the repository and compile the source code with CMake or your preferred build system.

## Contributing

Contributions to XProtection are welcome. To contribute:

- Fork the repository.
- Create a new branch (git checkout -b feature/fooBar).
- Make changes and add them (git add .).
- Commit your changes (git commit -m 'Add some fooBar').
- Push to the branch (git push origin feature/fooBar).
- Create a new Pull Request.

## Error Handling

The client includes robust error handling for PortAudio operations, throwing exceptions upon failures and providing detailed error messages.

## License
This project is licensed under the MIT License - see the LICENSE.md file for details.

## Acknowledgments
Thanks to AssemblyAI for providing the real-time transcription API.
Shoutout to the developers of the used open-source libraries.

## Contact
For support or queries, please contact us at zouhamaimou@gmail.com