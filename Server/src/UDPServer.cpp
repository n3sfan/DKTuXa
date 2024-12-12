#include "UDPServer.h"

#include <iostream>

#include "common/PacketBuffer.h"
#include "common/Request.h"
#include "Server.h"

std::atomic_bool serverFinished;

HWND hwnd;

int setupUDPServer() {
    const char* className = "UDPServerWindowClass";
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = className;

    if (!RegisterClassA(&wc)) {
        std::cerr << "RegisterClass failed. Error: " << GetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    hwnd = CreateWindowA(
        className,
        "Broadcast Server",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (!hwnd) {
        std::cerr << "CreateWindow failed. Error: " << GetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_WSAASYNC:
            switch (WSAGETSELECTEVENT(lParam)) {
                case FD_READ:
                    if (!WSAGETSELECTERROR(lParam)) {
                        std::cerr << "FD_READ failed. Error: " << WSAGetLastError() << std::endl;
                        return 0;
                    }

                    SOCKET ListenSocket = (SOCKET) wParam;
                    sockaddr_in clientAddr;
                    // Xử lý request
                    PacketBuffer buffer(ListenSocket, true, &clientAddr);
                    // buffer.getBuffer() = std::string(recvbuf.get(), iResult); // Chuyển đổi buffer thành chuỗi
                    Request request;
                    Response response;
                    request.deserialize(buffer);

                    Server server;
                    server.processRequest(request, response);

                    // std::cout << "DEBUG: Received data from " << inet_ntoa(clientAddr.sin_addr) << "\n";

                    // Gửi phản hồi lại client qua UDP
                    SOCKET ResponseSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

                    buffer = PacketBuffer(ResponseSocket, false, &clientAddr);
                    response.serialize(buffer);
                    std::cout << "DEBUG: Sent response to " << inet_ntoa(clientAddr.sin_addr) << "\n";

                    closesocket(ResponseSocket);
                    break;
            }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_STOP_BROADCAST:
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}