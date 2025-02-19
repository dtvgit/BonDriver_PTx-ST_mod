// PT1Ctrl.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <memory>
#include "PT1Ctrl.h"
#include "../Common/PTManager.h"
#include "../Common/PTCtrlMain.h"
#include "../Common/ServiceUtil.h"

CPTCtrlMain g_cMain(PT1_GLOBAL_LOCK_MUTEX, CMD_PT1_CTRL_EVENT_WAIT_CONNECT, CMD_PT1_CTRL_PIPE);

HANDLE g_hMutex;
SERVICE_STATUS_HANDLE g_hStatusHandle;

#define PT_CTRL_MUTEX L"PT1_CTRL_EXE_MUTEX"
#define SERVICE_NAME L"PT1Ctrl Service"

extern "C" IPTManager* CreatePT1Manager(void);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	if (_tcslen(lpCmdLine) > 0) {
		if (lpCmdLine[0] == '-' || lpCmdLine[0] == '/') {
			if (_tcsicmp(_T("install"), lpCmdLine + 1) == 0) {
				WCHAR strExePath[512] = L"";
				GetModuleFileName(NULL, strExePath, 512);
				InstallService(strExePath, SERVICE_NAME, SERVICE_NAME);
				return 0;
			}
			else if (_tcsicmp(_T("remove"), lpCmdLine + 1) == 0) {
				RemoveService(SERVICE_NAME);
				return 0;
			}
		}
	}

	if (IsInstallService(SERVICE_NAME) == FALSE) {
		//普通にexeとして起動を行う
		#if 0 // ??
		HANDLE h = ::OpenMutexW(SYNCHRONIZE, FALSE, PT1_GLOBAL_LOCK_MUTEX);
		if (h != NULL) {
			BOOL bErr = FALSE;
			if (::HRWaitForSingleObject(h, 100) == WAIT_TIMEOUT) {
				bErr = TRUE;
			}
			::ReleaseMutex(h);
			::CloseHandle(h);
			if (bErr) {
				return -1;
			}
		}
		#endif

		g_hStartEnableEvent = _CreateEvent(TRUE, FALSE, PT1_STARTENABLE_EVENT);
		if (g_hStartEnableEvent == NULL) {
			return -2;
		}
		#if 0
		// 別プロセスが終了処理中の場合は終了を待つ(最大1秒)
		if (::HRWaitForSingleObject(g_hStartEnableEvent, 1000) == WAIT_TIMEOUT) {
			::CloseHandle(g_hStartEnableEvent);
			return -3;
		}
		#endif

		g_hMutex = _CreateMutex(TRUE, PT_CTRL_MUTEX);
		if (g_hMutex == NULL) {
			::CloseHandle(g_hStartEnableEvent);
			return -4;
		}
		if (::HRWaitForSingleObject(g_hMutex, 0) == WAIT_TIMEOUT) {
			// 別プロセスが実行中だった
			::CloseHandle(g_hMutex);
			::SetEvent(g_hStartEnableEvent); // 再始動要請
			::CloseHandle(g_hStartEnableEvent);
			return -5;
		}

		//起動
		StartMain(FALSE);

		::ReleaseMutex(g_hMutex);
		::CloseHandle(g_hMutex);

		::SetEvent(g_hStartEnableEvent);
		::CloseHandle(g_hStartEnableEvent);
	}
	else {
		//サービスとしてインストール済み
		if (IsStopService(SERVICE_NAME) == FALSE) {
			g_hMutex = _CreateMutex(TRUE, PT_CTRL_MUTEX);
			int err = GetLastError();
			if (g_hMutex != NULL && err != ERROR_ALREADY_EXISTS) {
				//起動
				SERVICE_TABLE_ENTRY dispatchTable[] = {
					{ SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)service_main },
					{ NULL, NULL }
				};
				if (StartServiceCtrlDispatcher(dispatchTable) == FALSE) {
					OutputDebugString(_T("StartServiceCtrlDispatcher failed"));
				}
			}
		}
		else {
			//Stop状態なので起動する
			StartServiceCtrl(SERVICE_NAME);
		}
	}
	return 0;
}

void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{
	g_hStatusHandle = RegisterServiceCtrlHandlerEx( SERVICE_NAME, (LPHANDLER_FUNCTION_EX)service_ctrl, NULL);

    do {

		if (g_hStatusHandle == NULL){
			break;
		}

		SendStatusScm(SERVICE_START_PENDING, 0, 1);

		SendStatusScm(SERVICE_RUNNING, 0, 0);
		StartMain(TRUE);

	} while(0);

	SendStatusScm(SERVICE_STOPPED, 0, 0);
}

DWORD WINAPI service_ctrl(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	switch (dwControl){
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			SendStatusScm(SERVICE_STOP_PENDING, 0, 1);
			StopMain();
			return NO_ERROR;
			break;
		case SERVICE_CONTROL_POWEREVENT:
			OutputDebugString(_T("SERVICE_CONTROL_POWEREVENT"));
			if ( dwEventType == PBT_APMQUERYSUSPEND ){
				OutputDebugString(_T("PBT_APMQUERYSUSPEND"));
				if( g_cMain.IsFindOpen() ){
					OutputDebugString(_T("BROADCAST_QUERY_DENY"));
					return BROADCAST_QUERY_DENY;
					}
			}else if( dwEventType == PBT_APMRESUMESUSPEND ){
				OutputDebugString(_T("PBT_APMRESUMESUSPEND"));
			}
			break;
		default:
			break;
	}
	SendStatusScm(NO_ERROR, 0, 0);
	return NO_ERROR;
}

BOOL SendStatusScm(int iState, int iExitcode, int iProgress)
{
	SERVICE_STATUS ss;

	ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ss.dwCurrentState = iState;
	ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_POWEREVENT;
	ss.dwWin32ExitCode = iExitcode;
	ss.dwServiceSpecificExitCode = 0;
	ss.dwCheckPoint = iProgress;
	ss.dwWaitHint = 10000;

	return SetServiceStatus(g_hStatusHandle, &ss);
}

void StartMain(BOOL bService)
{
	std::unique_ptr<IPTManager> pt1_manager(CreatePT1Manager());
	g_cMain.StartMain(bService, pt1_manager.get());
}

void StopMain()
{
	g_cMain.StopMain();
}
