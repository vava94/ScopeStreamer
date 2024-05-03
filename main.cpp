//
// Created by vok on 19.10.23.
//
#include <iostream>
#include <memory>
#include <cstring>
#include <signal.h>
#include <csignal>
#include "ColorConverter/ColorConverter.h"
#include "CameraHandler/V4L2Handler.hpp"
#include "TCPDude/tcpdude.h"

TCPDude *tcpDude;

uint16_t port = 10001;
std::string cam = "/dev/video0";

SOCKET clientSocket = -1;

char tcpData[4096] = {0};
uint16_t tcpDataCount = 0;
bool newTCPData = false;
std::mutex tcpDataMutex;

bool *isRunning = new bool;

void clientConnected(SOCKET socket) {
    if (clientSocket == -1) {
        clientSocket = socket;
        std::cout << "Client connected: " << tcpDude->getAddress(clientSocket) << std::endl;
    } else {
        std::cout << "Connect attempt: " << tcpDude->getAddress(socket) << std::endl;
        std::cout << "Client already connected: " << tcpDude->getAddress(clientSocket) << std::endl;
        tcpDude->disconnect(socket);
    }
}

void clientDisconnected(SOCKET socket) {
    if (socket == clientSocket) {
        clientSocket = -1;
    }
    std::cout << "Client disconnected" << std:: endl;
}

void tcpDataReady(SOCKET socket, char *data, size_t size) {
    if (socket != clientSocket) return;
    tcpDataMutex.lock();
    memcpy(&tcpData[tcpDataCount], data, size);
    tcpDataCount += size;
    tcpDataMutex.unlock();
}

void tcpDudeError(int code) {
    std::cerr << "TCPDude error: " << code << std::endl;
}

void interruptHandler(int) {
    std::cout << "Keyboard interrupt. Wait for tasks finishing." << std::endl;
    *isRunning = false;
}

void newCameraFrame(CameraDevice *cameraDevice) {
    std::cout << "frame " << cam << std::endl;
}

int main(int argc, char** argv) {

    int endCode = 0;

    struct sigaction sigIntHandler{};
    CameraDevice *cameraDevice = nullptr;
    V4L2Handler v4L2Handler = V4L2Handler();

    sigIntHandler.sa_handler = interruptHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    if (argc > 1 && (argc % 2 == 0)) {
        for (int i = 1; i < argc; i+=2) {
            if (std::string(argv[i]) == "--port") {
                port = std::stoi(argv[i + 1]) & 0xFFFF;
            }
            else if (std::string(argv[i]) == "--cam") {
                cam = argv[i + 1];
            }
        }
    }
    else {
        std::cout << "Running with default params" << std::endl;
    }
    std::cout << "Server port: " << port << std::endl;
    std::cout << "Camera: " << cam << std::endl;



    tcpDude = new TCPDude(TCPDude::SERVER_MODE);
    tcpDude->setConnectedCallback(clientConnected);
    tcpDude->setConnectedCallback(clientDisconnected);
    tcpDude->setDataReadyCallback(tcpDataReady);
    tcpDude->setErrorHandlerCallback(tcpDudeError);
    tcpDude->startServer(port);


    size_t camsCount = v4L2Handler.listCameras();
    if(camsCount == 0) {
        std::cerr << "No connected cameras." << std::endl;
        endCode = ENODEV;
        goto stop_server;
    }

    for (size_t i = 0; i < camsCount; i++) {
        if (v4L2Handler.getCameraAt(i)->name() == cam) {
            cameraDevice = v4L2Handler.getCameraAt(i);
            break;
        }
    }

    cameraDevice = v4L2Handler.getCameraAt(0);

    if (cameraDevice == nullptr) {
        std::cerr << "No such camera: " << cam << std::endl;
        endCode = ENODEV;
        goto stop_server;
    }

    if(!v4L2Handler.open((int)0)){
        std::cerr << "Can't open camera: " << cam << std::endl;
        endCode = EACCES;
        goto stop_server;
    }
    v4L2Handler.setCallbackNewFrame(newCameraFrame);

    if (!v4L2Handler.startCapture(cameraDevice, cameraDevice->formats()[0], false)) {
        std::cerr << "Can't start capture from camera: " << cam << std::endl;
        endCode = EACCES;
        goto stop_server;
    }

    *isRunning = true;

    while (*isRunning) {

        if (newTCPData && tcpDataMutex.try_lock()) {

        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));

    }

 stop_server:
    tcpDude->stopServer();

    return endCode;
} 