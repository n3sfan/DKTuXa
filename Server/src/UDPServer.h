#include <atomic>

#include <winsock2.h>
#include <windows.h>

#define WM_WSAASYNC (WM_USER + 1)
#define WM_STOP_BROADCAST (WM_USER + 2)
#define DEFAULT_PORT2 5556
#define BUFFER_SIZE 8192

extern std::atomic_bool serverFinished;

extern HWND hwnd;

int initUDPServer();

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);