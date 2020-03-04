#include <Windows.h>
#include "shared.h"

const int DESIRED_WIDTH = 2560;

static DWORD gInjectorPid = 0;
static HMODULE gModule = 0;
static HANDLE gThreadHandle = 0;
static HWND gTaskbarHWnd = 0;

static WNDPROC gOriginalWndProc = 0;

void UpdateTaskbar()
{
	HWND desktopHWnd = GetDesktopWindow();
	RECT desktopRect;
	GetWindowRect(desktopHWnd, &desktopRect);

	RECT taskbarRect;
	GetWindowRect(gTaskbarHWnd, &taskbarRect);
	
	int desiredLeft = (desktopRect.right / 2) - (DESIRED_WIDTH / 2);
	int desiredRight = (desktopRect.right / 2) + (DESIRED_WIDTH / 2);

	if (taskbarRect.left != desiredLeft || taskbarRect.right != desiredRight) {
		// Resize taskbar and make sure everything is updated
		SetWindowPos(gTaskbarHWnd, NULL, desiredLeft, taskbarRect.top, DESIRED_WIDTH, taskbarRect.bottom - taskbarRect.top, SWP_NOSENDCHANGING);
		ShowWindow(gTaskbarHWnd, SW_SHOW);
		UpdateWindow(gTaskbarHWnd);
		RedrawWindow(desktopHWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		SystemParametersInfo(SPI_SETWORKAREA, 0, NULL, SPIF_SENDCHANGE);
	}
}

void SqueezeExit()
{
	if (gOriginalWndProc != NULL)
		SetWindowLongPtr(gTaskbarHWnd, GWLP_WNDPROC, (LONG_PTR)gOriginalWndProc);

	CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)FreeLibraryAndExitThread, gModule, 0, nullptr));
}

bool InjectorStillRunning()
{
	if (gInjectorPid == 0)
		return true; // Injector has not notified us so we assume we should keep running

	HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, gInjectorPid);
	DWORD lpExitCode;
	GetExitCodeProcess(handle, &lpExitCode);
	if (lpExitCode != STILL_ACTIVE)
		return false;
	
	return true;
}

LRESULT CALLBACK WndProcTaskBar(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!InjectorStillRunning()) {
		SqueezeExit();
		return CallWindowProcA(gOriginalWndProc, hwnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
		case WM_WINDOWPOSCHANGING:
			UpdateTaskbar();
			return 0;
		case WM_SQUEEZE:
			if (wParam == SQUEEZE_INJECTOR_PID) {
				gInjectorPid = (DWORD)lParam;
			}
		default:
			break;
	}
	return CallWindowProcA(gOriginalWndProc, hwnd, uMsg, wParam, lParam);
}

void SqueezeInit()
{
	gTaskbarHWnd = FindWindow(TEXT("Shell_TrayWnd"), nullptr);
	if (IsWindow(gTaskbarHWnd)) {
		gOriginalWndProc = (WNDPROC)SetWindowLongPtr(gTaskbarHWnd, GWLP_WNDPROC, (LONG_PTR)&WndProcTaskBar);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		gModule = hModule;
		SqueezeInit(); // CreateThread?
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}
