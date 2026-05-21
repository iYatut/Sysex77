/*
  ==============================================================================

    Sy99ParamApiServer.h
    Minimal localhost REST API for React SPA (params meta + LiveSynthState dump).

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class Sy99ParamApiServer
{
public:
    static constexpr int kDefaultPort = 8765;

    Sy99ParamApiServer() = default;
    ~Sy99ParamApiServer();

    void start (int port = kDefaultPort);
    void stop();

    bool isRunning() const noexcept { return running.load(); }
    int getPort() const noexcept { return listenPort; }

private:
    class ServerThread;

    std::unique_ptr<ServerThread> serverThread;
    std::atomic<bool> running { false };
    int listenPort = kDefaultPort;
};

Sy99ParamApiServer& getSy99ParamApiServer() noexcept;
