#include "StdAfx.h"
#include <memory>
#include "PTCtrlMain.h"

CPTCtrlMain::CPTCtrlMain(
	wstring strGlobalLockMutex,
	wstring strPipeEvent,
	wstring strPipeName )
{
	m_pManager = NULL;
	m_strGlobalLockMutex = strGlobalLockMutex;
	m_strPipeEvent = strPipeEvent;
	m_strPipeName = strPipeName;
	m_hStopEvent = _CreateEvent(TRUE, FALSE,NULL);
	m_bService = FALSE;
}

CPTCtrlMain::~CPTCtrlMain(void)
{
	StopMain();
	if( m_hStopEvent != NULL ){
		CloseHandle(m_hStopEvent);
	}
//	m_cPipeserver.StopServer();
}

BOOL CPTCtrlMain::Init(BOOL bService, IPTManager *pManager)
{
	if(!pManager) return FALSE;

	UnInit();

	m_pManager = pManager ;

	BOOL bInit = TRUE;
	if( m_pManager->LoadSDK() == FALSE ){
		OutputDebugString(L"PTx SDK�̃��[�h�Ɏ��s");
		bInit = FALSE;
	}

	DBGOUT("PTx SDK�̃��[�h%s\n",bInit?"����":"���s") ;

	if( bInit ){
		m_pManager->Init();
	}
	m_bService = bService;

	DoKeepAlive();

	return bInit ;
}

void CPTCtrlMain::UnInit()
{
	if(m_pManager) {
		m_pManager->UnInit();
		m_pManager=NULL;
	}
}

CPipeServer *CPTCtrlMain::MakePipeServer()
{
	CPipeServer *pcPipeServer = new CPipeServer(m_strGlobalLockMutex.c_str());
	pcPipeServer->StartServer(
		m_strPipeEvent.c_str()/*CMD_PT_CTRL_EVENT_WAIT_CONNECT*/,
		m_strPipeName.c_str()/*CMD_PT_CTRL_PIPE*/, OutsideCmdCallback, this);
	return pcPipeServer ;
}

void CPTCtrlMain::DoKeepAlive()
{
	m_dwLastDeactivated=dur();
}

DWORD CPTCtrlMain::LastDeactivated()
{
	return m_dwLastDeactivated ;
}

void CPTCtrlMain::StartMain(BOOL bService, IPTManager *pManager)
{
	if(!Init(bService, pManager)) {
		m_pManager=NULL;
		return ;
	}

	//Pipe�T�[�o�[�X�^�[�g
	std::unique_ptr<CPipeServer> pipeServer(MakePipeServer());

	bool terminated = false, deactivated=false ;
	DoKeepAlive();

	const DWORD WAIT = 15000, DEACT_WAIT = 5000 ;
	while(!terminated) {

		if(!deactivated) {

			if( HRWaitForSingleObject(m_hStopEvent, WAIT) != WAIT_TIMEOUT ){
				deactivated=true;
				DoKeepAlive();
				ResetEvent(g_hStartEnableEvent);
			}else {
				mutex_locker_t locker(m_strGlobalLockMutex,false) ;
				if(locker.lock(10)) {
					//�A�v���w���񂾎��p�̃`�F�b�N
					if( m_pManager->CloseChk() == FALSE ) {
						if( m_bService == FALSE){
							locker.unlock();
							DoKeepAlive();
							deactivated=true;
						}else {
							m_pManager->FreeDevice();
						}
					}
				}
			}

		}else {

			auto launchMutexCheck = []() ->bool {
				HANDLE h = ::OpenMutex(SYNCHRONIZE, FALSE, LAUNCH_PTX_CTRL_MUTEX);
				if (h != NULL) {
					::ReleaseMutex(h);
					::CloseHandle(h);
					return true ;
				}
				return false ;
			};

			while(dur(LastDeactivated())<DEACT_WAIT||launchMutexCheck()) {
				if(HRWaitForSingleObject(m_hStopEvent, 0) != WAIT_OBJECT_0) {
					deactivated=false; break;
				}
				if(HRWaitForSingleObject(g_hStartEnableEvent, 100) == WAIT_OBJECT_0) {
					DoKeepAlive();
					ResetEvent(g_hStartEnableEvent);
				}
			}
			if(deactivated) {
				if(HRWaitForSingleObject(g_hStartEnableEvent,0) == WAIT_OBJECT_0) {
					DoKeepAlive();
					ResetEvent(g_hStartEnableEvent);
				}else {
					terminated=true ;
				}
			}
		}

	}

	pipeServer->StopServer();
	UnInit();
}

void CPTCtrlMain::StopMain()
{
	if( m_hStopEvent != NULL ){
		SetEvent(m_hStopEvent);
	}
}

int CALLBACK CPTCtrlMain::OutsideCmdCallback(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam, BOOL* /*pbResDataAbandon*/)
{
	CPTCtrlMain* pSys = (CPTCtrlMain*)pParam;

	switch( pCmdParam->dwParam ){
		case CMD_KEEP_ALIVE:
			pSys->CmdKeepAlive(pCmdParam, pResParam);
			break;
		case CMD_CLOSE_EXE:
			pSys->CmdCloseExe(pCmdParam, pResParam);
			break;
		case CMD_GET_TOTAL_TUNER_COUNT:
			pSys->CmdGetTotalTunerCount(pCmdParam, pResParam);
			break;
		case CMD_GET_ACTIVE_TUNER_COUNT:
			pSys->CmdGetActiveTunerCount(pCmdParam, pResParam);
			break;
		case CMD_SET_LNB_POWER:
			pSys->CmdSetLnbPower(pCmdParam, pResParam);
			break;
		case CMD_OPEN_TUNER:
			pSys->CmdOpenTuner(pCmdParam, pResParam);
			break;
		case CMD_CLOSE_TUNER:
			pSys->CmdCloseTuner(pCmdParam, pResParam);
			break;
		case CMD_SET_CH:
			pSys->CmdSetCh(pCmdParam, pResParam);
			break;
		case CMD_GET_SIGNAL:
			pSys->CmdGetSignal(pCmdParam, pResParam);
			break;
		case CMD_OPEN_TUNER2:
			pSys->CmdOpenTuner2(pCmdParam, pResParam);
			break;
		case CMD_GET_STREAMING_METHOD:
			pSys->CmdGetStreamingMethod(pCmdParam, pResParam);
			break;
		case CMD_SET_FREQ:
			pSys->CmdSetFreq(pCmdParam, pResParam);
			break;
		case CMD_GET_IDLIST_S:
			pSys->CmdGetIdListS(pCmdParam, pResParam);
			break;
		case CMD_SET_ID_S:
			pSys->CmdSetIdS(pCmdParam, pResParam);
			break;
		case CMD_GET_ID_S:
			pSys->CmdGetIdS(pCmdParam, pResParam);
			break;
		default:
			pResParam->dwParam = CMD_NON_SUPPORT;
			break;
	}
	return 0;
}

//CMD_KEEP_ALIVE PTxCtrl.exe�̊����ێ��R�}���h
void CPTCtrlMain::CmdKeepAlive(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	pResParam->dwParam = CMD_SUCCESS;
	DoKeepAlive();
	SetEvent(g_hStartEnableEvent);
}

//CMD_CLOSE_EXE PTxCtrl.exe�̏I��
void CPTCtrlMain::CmdCloseExe(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	pResParam->dwParam = CMD_SUCCESS;
	StopMain();
}

//CMD_GET_TOTAL_TUNER_COUNT GetTotalTunerCount
void CPTCtrlMain::CmdGetTotalTunerCount(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	DWORD dwNumTuner;
	dwNumTuner = m_pManager->GetTotalTunerCount();

	pResParam->dwParam = CMD_SUCCESS;
	CreateDefStream(dwNumTuner, pResParam);
}

//CMD_GET_ACTIVE_TUNER_COUNT GetActiveTunerCount
void CPTCtrlMain::CmdGetActiveTunerCount(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	BOOL bSate;
	CopyDefData((DWORD*)&bSate, pCmdParam->bData);

	DWORD dwNumTuner;
	dwNumTuner = m_pManager->GetActiveTunerCount(bSate);

	pResParam->dwParam = CMD_SUCCESS;
	CreateDefStream(dwNumTuner, pResParam);
}

//CMD_SET_LNB_POWER SetLnbPower
void CPTCtrlMain::CmdSetLnbPower(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	int iID;
	BOOL bEnabled;
	CopyDefData2((DWORD*)&iID, (DWORD*)&bEnabled, pCmdParam->bData);

	BOOL r = m_pManager->SetLnbPower(iID, bEnabled);

	if( r ){
		pResParam->dwParam = CMD_SUCCESS;
	}else{
		pResParam->dwParam = CMD_ERR;
	}
}

//CMD_OPEN_TUNER OpenTuner
void CPTCtrlMain::CmdOpenTuner(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	BOOL bSate;
	CopyDefData((DWORD*)&bSate, pCmdParam->bData);

	int iID = m_pManager->OpenTuner(bSate);
	if( iID != -1 ){
		ResetEvent(m_hStopEvent); // �I�������̎��
		pResParam->dwParam = CMD_SUCCESS;
	}else{
		pResParam->dwParam = CMD_ERR;
	}

	CreateDefStream(iID, pResParam);
}

//CMD_CLOSE_TUNER CloseTuner
void CPTCtrlMain::CmdCloseTuner(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	int iID;
	CopyDefData((DWORD*)&iID, pCmdParam->bData);

	if(iID!=0xFFFF'FFFF) m_pManager->CloseTuner(iID);
	pResParam->dwParam = CMD_SUCCESS;
	if (m_bService == FALSE) {
		if (m_pManager->IsFindOpen() == FALSE) {
			StopMain();
		}
	}
}

//CMD_SET_CH SetChannel
void CPTCtrlMain::CmdSetCh(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	int iID;
	DWORD dwCh;
	DWORD dwTSID;
	BOOL hasStream=FALSE;
	CopyDefData3((DWORD*)&iID, &dwCh, &dwTSID, pCmdParam->bData);
	if( m_pManager->SetCh(iID,dwCh,dwTSID,hasStream) ){
		pResParam->dwParam = CMD_SUCCESS;
	}else{
		pResParam->dwParam = CMD_ERR;
	}
	if(!hasStream) {
		pResParam->dwParam |= CMD_BIT_NON_STREAM;
	}
}

//CMD_GET_SIGNAL GetSignalLevel
void CPTCtrlMain::CmdGetSignal(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	int iID;
	DWORD dwCn100;
	CopyDefData((DWORD*)&iID, pCmdParam->bData);
	dwCn100 = m_pManager->GetSignal(iID);

	pResParam->dwParam = CMD_SUCCESS;
	CreateDefStream(dwCn100, pResParam);
}

//CMD_OPEN_TUNER2 OpenTuner2
void CPTCtrlMain::CmdOpenTuner2(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	BOOL bSate;
	int iTunerID;
	CopyDefData2((DWORD*)&bSate, (DWORD*)&iTunerID, pCmdParam->bData);
	int iID = m_pManager->OpenTuner2(bSate, iTunerID);
	if( iID != -1 ){
		ResetEvent(m_hStopEvent); // �I�������̎��
		pResParam->dwParam = CMD_SUCCESS;
	}else{
		pResParam->dwParam = CMD_ERR;
	}
	CreateDefStream(iID, pResParam);
}

//CMD_GET_STREAMING_METHOD GetStreamingMethod
void CPTCtrlMain::CmdGetStreamingMethod(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	DWORD method = m_pManager->GetStreamingMethod();

	pResParam->dwParam = CMD_SUCCESS;
	CreateDefStream(method, pResParam);
}

//CMD_SET_FREQ SetFreq
void CPTCtrlMain::CmdSetFreq(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	int iID;
	DWORD dwCh;
	CopyDefData2((DWORD*)&iID, (DWORD*)&dwCh, pCmdParam->bData);
	BOOL bRes = m_pManager->SetFreq(iID, dwCh);
	if( bRes ){
		pResParam->dwParam = CMD_SUCCESS;
	}else{
		pResParam->dwParam = CMD_ERR;
	}
}

//CMD_GET_IDLIST_S GetIdListS
void CPTCtrlMain::CmdGetIdListS(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	int iID;
	CopyDefData((DWORD*)&iID, pCmdParam->bData);
	PTTSIDLIST list = {0} ;
	BOOL bRes = m_pManager->GetIdListS(iID, &list);
	if( bRes ){
		pResParam->dwParam = CMD_SUCCESS;
	}else{
		pResParam->dwParam = CMD_ERR;
	}
	CreateDefStreamN(&list.dwId[0], 8, pResParam);
}

//CMD_GET_ID_S GetIdS
void CPTCtrlMain::CmdGetIdS(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	int iID;
	CopyDefData((DWORD*)&iID, pCmdParam->bData);
	DWORD dwId=0;
	BOOL bRes = m_pManager->GetIdS(iID, &dwId);
	if( bRes ){
		pResParam->dwParam = CMD_SUCCESS;
	}else{
		pResParam->dwParam = CMD_ERR;
	}
	CreateDefStream(dwId, pResParam);
}

//CMD_SET_ID_S SetIdS
void CPTCtrlMain::CmdSetIdS(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	int iID;
	DWORD dwId;
	CopyDefData2((DWORD*)&iID, (DWORD*)&dwId, pCmdParam->bData);
	BOOL bRes = m_pManager->SetIdS(iID, dwId);
	if( bRes ){
		pResParam->dwParam = CMD_SUCCESS;
	}else{
		pResParam->dwParam = CMD_ERR;
	}
}

BOOL CPTCtrlMain::IsFindOpen()
{
	if(m_pManager==NULL) return FALSE ;
	return m_pManager->IsFindOpen();
}
