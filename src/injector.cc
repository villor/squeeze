#include <Windows.h>
#include <Shlwapi.h>
#include "shared.h"

#pragma comment(lib, "Shlwapi.lib")

static bool gShouldExit = 0;

// TODO: Add explorer.exe restart detection

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    // Find shell window
	HWND hWnd = FindWindow(TEXT("Shell_TrayWnd"), nullptr);
    int t = 0;
    while (hWnd == nullptr) {
        Sleep(500);
        hWnd = FindWindow(TEXT("Shell_TrayWnd"), nullptr);
        t++;
        if (t > 30 && hWnd == nullptr) {
            MessageBox(0, TEXT("Could not find Shell_TrayWnd (is eplorer.exe running?)"), TEXT("Squeeze: Injection error"), MB_ICONERROR);
            return -1;
        }
    }

    // Get thread and process id for shell window
    DWORD processId;
    DWORD threadId = GetWindowThreadProcessId(hWnd, &processId);

    HANDLE hProcess = OpenProcess(0xFFF, FALSE, processId);
    if (hProcess == NULL) {
        MessageBox(0, TEXT("Unable to obtain the explorer handle"), TEXT("Squeeze: Injection error"), MB_ICONERROR);
        return -1;
    }

    // Use current exe directory to make DLL path
    const LPCSTR DLL_NAME = "squeeze.dll";
    CHAR dllPath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, dllPath, MAX_PATH);
    StrCpyA(PathFindFileNameA(dllPath), DLL_NAME);

    // Allocate memory in explorer.exe and write squeeze DLL path to it
    SIZE_T dllPathSize = strlen(dllPath) + 1;
    SIZE_T dllPathNumBytesWritten;
    LPVOID lpRemoteBuf = VirtualAllocEx(hProcess, NULL, dllPathSize, MEM_COMMIT, PAGE_READWRITE);
    if (WriteProcessMemory(hProcess, lpRemoteBuf, dllPath, dllPathSize, &dllPathNumBytesWritten)) {
        if (dllPathNumBytesWritten != dllPathSize) {
            VirtualFreeEx(hProcess, lpRemoteBuf, dllPathSize, MEM_COMMIT);
            MessageBox(0, TEXT("Written memory length does not match DLL path length."), TEXT("Squeeze: Injection error"), MB_ICONERROR);
            CloseHandle(hProcess);
            return -1;
        }
    } else {
        MessageBox(0, TEXT("Failed to write DLL path to explorer memory"), TEXT("Squeeze: Injection error"), MB_ICONERROR);
        CloseHandle(hProcess);
        return -1;
    }
    VirtualFreeEx(hProcess, lpRemoteBuf, dllPathSize, MEM_COMMIT);

    // Inject squeeze DLL
    DWORD dwNewThreadId;
    LPVOID lpLoadDll = LoadLibraryA;
    HANDLE hNewRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)lpLoadDll, lpRemoteBuf, 0, &dwNewThreadId);
    if (hNewRemoteThread == NULL) {
        MessageBox(0, TEXT("Failed to start remote thread (LoadLibraryA)."), TEXT("Squeeze: Injection error"), MB_ICONERROR);
        CloseHandle(hProcess);
        return -1;
    }
    WaitForSingleObject(hNewRemoteThread, INFINITE);
    CloseHandle(hNewRemoteThread);
    CloseHandle(hProcess);

    // Send injector PID to the DLL so it knows when to exit
    SendMessage(hWnd, WM_SQUEEZE, SQUEEZE_INJECTOR_PID, GetCurrentProcessId());

    Sleep(INFINITE);
}