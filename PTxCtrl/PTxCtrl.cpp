// PTxCtrl.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <memory>
#include "../Common/PTManager.h"
#include "../Common/PTCtrlMain.h"
#include "../Common/ServiceUtil.h"
#include "PTxCtrl.h"

CPTCtrlMain g_cMain3(PT0_GLOBAL_LOCK_MUTEX, CMD_PT3_CTRL_EVENT_WAIT_CONNECT, CMD_PT3_CTRL_PIPE, FALSE);
CPTCtrlMain g_cMain1(PT0_GLOBAL_LOCK_MUTEX, CMD_PT1_CTRL_EVENT_WAIT_CONNECT, CMD_PT1_CTRL_PIPE, FALSE);

HANDLE g_hMutex;
SERVICE_STATUS_HANDLE g_hStatusHandle;

#define PT_CTRL_MUTEX L"PT0_CTRL_EXE_MUTEX"
#define SERVICE_NAME L"PT0Ctrl Service"

extern "C" IPTManager* CreatePT1Manager(void);
extern "C" IPTManager* CreatePT3Manager(void);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	if( _tcslen(lpCmdLine) > 0 ){
		if( lpCmdLine[0] == '-' || lpCmdLine[0] == '/' ){
			if( _tcsicmp( _T("install"), lpCmdLine+1 ) == 0 ){
				WCHAR strExePath[512] = L"";
				GetModuleFileName(NULL, strExePath, 512);
				InstallService(strExePath, SERVICE_NAME,SERVICE_NAME);
				return 0;
			}else if( _tcsicmp( _T("remove"), lpCmdLine+1 ) == 0 ){
				RemoveService(SERVICE_NAME);
				return 0;
			}
		}
	}

	if( IsInstallService(SERVICE_NAME) == FALSE ){
		//普通にexeとして起動を行う
		HANDLE h = ::OpenMutexW(SYNCHRONIZE, FALSE, PT0_GLOBAL_LOCK_MUTEX);
		if (h != NULL) {
			BOOL bErr = FALSE;
			if (::WaitForSingleObject(h, 100) == WAIT_TIMEOUT) {
				bErr = TRUE;
			}
			::ReleaseMutex(h);
			::CloseHandle(h);
			if (bErr) {
				return -1;
			}
		}

		g_hStartEnableEvent = _CreateEvent(TRUE, TRUE, PT0_STARTENABLE_EVENT);
		if (g_hStartEnableEvent == NULL) {
			return -2;
		}
		// 別プロセスが終了処理中の場合は終了を待つ(最大1秒)
		if (::WaitForSingleObject(g_hStartEnableEvent, 1000) == WAIT_TIMEOUT) {
			::CloseHandle(g_hStartEnableEvent);
			return -3;
		}

		g_hMutex = _CreateMutex(TRUE, PT_CTRL_MUTEX);
		if (g_hMutex == NULL) {
			::CloseHandle(g_hStartEnableEvent);
			return -4;
		}
		if (::WaitForSingleObject(g_hMutex, 100) == WAIT_TIMEOUT) {
			// 別プロセスが実行中だった
			::CloseHandle(g_hMutex);
			::CloseHandle(g_hStartEnableEvent);
			return -5;
		}

		//起動
		StartMain(FALSE);

		::ReleaseMutex(g_hMutex);
		::CloseHandle(g_hMutex);

		::SetEvent(g_hStartEnableEvent);
		::CloseHandle(g_hStartEnableEvent);
	}else{
		//サービスとしてインストール済み
		if( IsStopService(SERVICE_NAME) == FALSE ){
			g_hMutex = _CreateMutex(TRUE, PT_CTRL_MUTEX);
			int err = GetLastError();
			if( g_hMutex != NULL && err != ERROR_ALREADY_EXISTS ) {
				//起動
				SERVICE_TABLE_ENTRY dispatchTable[] = {
					{ SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)service_main},
					{ NULL, NULL}
				};
				if( StartServiceCtrlDispatcher(dispatchTable) == FALSE ){
					OutputDebugString(_T("StartServiceCtrlDispatcher failed"));
				}
			}
		}else{
			//Stop状態なので起動する
			StartServiceCtrl(SERVICE_NAME);
		}
	}
	return 0;
}

void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{
	g_hStatusHandle = RegisterServiceCtrlHandlerEx( SERVICE_NAME, (LPHANDLER_FUNCTION_EX)service_ctrl, NULL);

	if (g_hStatusHandle == NULL){
		goto cleanup;
	}

	SendStatusScm(SERVICE_START_PENDING, 0, 1);

	SendStatusScm(SERVICE_RUNNING, 0, 0);
	StartMain(TRUE);

cleanup:
	SendStatusScm(SERVICE_STOPPED, 0, 0);

   return;
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
				if( g_cMain1.IsFindOpen() || g_cMain3.IsFindOpen()){
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

CPTxCtrlCmdServiceOperator *g_cCmdServer = NULL ;

	template<class T>
	struct safe_pobj {
		T** ppobj ;
		safe_pobj(T **ppobj_) : ppobj(ppobj_) {}
		~safe_pobj() { SAFE_DELETE((*ppobj)); }
		T* operator ->() { return *ppobj; }
	};

void StartMain(BOOL bService)
{
	g_cCmdServer = new CPTxCtrlCmdServiceOperator(CMD_PTX_CTRL_OP,bService);
	safe_pobj<CPTxCtrlCmdServiceOperator>(&g_cCmdServer)->Main();
}

void StopMain()
{
	if(g_cCmdServer!=NULL)
		g_cCmdServer->Stop();
}


  // CPTxCtrlCmdServiceOperator

CPTxCtrlCmdServiceOperator::CPTxCtrlCmdServiceOperator(std::wstring name, BOOL bService)
 : CPTxCtrlCmdOperator(name,true)
{
	PtService = bService ;
	PtPipeServer1 = PtPipeServer3 = NULL ;
	PtSupported = PtActivated = 0 ;

	Pt1Manager = CreatePT1Manager();
	Pt3Manager = CreatePT3Manager();

    if(g_cMain1.Init(PtService, Pt1Manager)) {
		PtPipeServer1 = g_cMain1.MakePipeServer() ;
		PtSupported |= 1 ;
		DBGOUT("PTxCtrl: PT1 Supported.\n");
	}else {
		DBGOUT("PTxCtrl: PT1 Not Supported.\n");
	}

    if(g_cMain3.Init(PtService, Pt3Manager)) {
		PtPipeServer3 = g_cMain3.MakePipeServer() ;
		PtSupported |= 1<<2 ;
		DBGOUT("PTxCtrl: PT3 Supported.\n");
	}else {
		DBGOUT("PTxCtrl: PT3 Not Supported.\n");
	}

	PtActivated = PtSupported ;

	PtTerminated = FALSE ;
}

CPTxCtrlCmdServiceOperator::~CPTxCtrlCmdServiceOperator()
{
	SAFE_DELETE(PtPipeServer1) ;
	SAFE_DELETE(PtPipeServer3) ;
	SAFE_DELETE(Pt1Manager) ;
	SAFE_DELETE(Pt3Manager) ;
}

BOOL CPTxCtrlCmdServiceOperator::ResSupported(DWORD &PtBits)
{
	PtBits = PtSupported ;
	return TRUE;
}

BOOL CPTxCtrlCmdServiceOperator::ResActivatePt(DWORD PtVer)
{
	if( ! (PtSupported&(1<<(PtVer-1))) ) {
		DBGOUT("PTxCtrl: PT%d Not Supported.\n",PtVer);
		return FALSE;
	}

	if( PtActivated &(1<<(PtVer-1)) ) {
		DBGOUT("PTxCtrl: PT%d Already Activated.\n",PtVer);
		return TRUE;
	}

	if(PtVer==1) {
	    if(g_cMain3.Init(PtService, Pt3Manager)) {
			PtPipeServer3 = g_cMain3.MakePipeServer() ;
			PtActivated |= 1<<2 ;
			DBGOUT("PTxCtrl: PT1 Re-Activated.\n");
			return TRUE ;
		}
	}else if(PtVer==3) {
	    if(g_cMain1.Init(PtService, Pt1Manager)) {
			PtPipeServer1 = g_cMain1.MakePipeServer() ;
			PtActivated |= 1 ;
			DBGOUT("PTxCtrl: PT3 Re-Activated.\n");
			return TRUE ;
		}
	}

	return FALSE;
}

void CPTxCtrlCmdServiceOperator::Main()
{
	PtTerminated = FALSE ;

	//------ BEGIN OF LOOP ------

	DBGOUT("PTxCtrl: service started.\n");

	while(!PtTerminated) {

		BOOL bRstStEnable=FALSE ;

		if(PtActivated&(1<<2)) { // PT3
        	if(WaitForSingleObject(g_cMain3.GetStopEvent(),0)==WAIT_OBJECT_0) {
				ResetEvent(g_hStartEnableEvent); bRstStEnable=TRUE ;
				SAFE_DELETE(PtPipeServer3);
				g_cMain3.UnInit();
				PtActivated &= ~(1<<2) ;
				ResetEvent(g_cMain3.GetStopEvent());
				DBGOUT("PTxCtrl: PT3 Stopped.\n");
			}
		}

		if(PtActivated&1) { // PT1/PT2
        	if(WaitForSingleObject(g_cMain1.GetStopEvent(),0)==WAIT_OBJECT_0) {
				if(!bRstStEnable) {
					ResetEvent(g_hStartEnableEvent);
					bRstStEnable=TRUE ;
				}
				SAFE_DELETE(PtPipeServer1);
				g_cMain1.UnInit();
				PtActivated &= ~1 ;
				ResetEvent(g_cMain1.GetStopEvent());
				DBGOUT("PTxCtrl: PT1 Stopped.\n");
			}
		}

		if(!PtActivated && !PtService) { PtTerminated=TRUE; continue; }
		else if(bRstStEnable) SetEvent(g_hStartEnableEvent);

		if(WaitForCmd(15*1000)==WAIT_OBJECT_0) {
			if(!ServiceReaction()) {
				DBGOUT("PTxCtrl: service reaction failed.\n");
			}
		}else{
			//アプリ層死んだ時用のチェック
			if(PtActivated&(1<<2)) { // PT3
				if( Pt3Manager->CloseChk() == FALSE){
					if(!PtService) SetEvent(g_cMain3.GetStopEvent()) ;
				}
			}
			if(PtActivated&1) { // PT1/PT2
				if( Pt1Manager->CloseChk() == FALSE){
					if(!PtService) SetEvent(g_cMain1.GetStopEvent()) ;
				}
			}
		}

	}

	DBGOUT("PTxCtrl: service finished.\n");

	//------ END OF LOOP ------

	SAFE_DELETE(PtPipeServer3);
	SAFE_DELETE(PtPipeServer1);
	if(PtActivated&1)
		g_cMain1.UnInit();
	if(PtActivated&(1<<2))
		g_cMain3.UnInit();

	PtActivated = 0 ;
}

void CPTxCtrlCmdServiceOperator::Stop()
{
	g_cMain1.StopMain();
	g_cMain3.StopMain();
	PtTerminated = TRUE ;
}

