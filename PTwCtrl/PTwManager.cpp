#include "StdAfx.h"
#include <assert.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include "PTwManager.h"

#ifndef PTW_GALAPAGOS // NOT Galapagosization ( EARTH defacto standard )

#include "../Common/PTxManager.cxx"

#else // begin of PTW_GALAPAGOS

#include "PtDrvWrap.h"
//===========================================================================
// CPTwManager ( PTxWDM bypass manager )
//---------------------------------------------------------------------------
CPTwManager::CPTwManager(void)
{
	InitializeCriticalSection(&Critical_);
	m_cpPrimaryOperator=nullptr;
	m_hPrimaryAuxiliaryThread=INVALID_HANDLE_VALUE;

	WCHAR strExePath[512] = L"";
	GetModuleFileName(NULL, strExePath, 512);

	WCHAR szPath[_MAX_PATH];	// �p�X
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_tsplitpath_s( strExePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_tmakepath_s(  szPath, _MAX_PATH, szDrive, szDir, NULL, NULL );

	m_strExePath = strExePath;

	wstring strIni;

	strIni = szPath;
	strIni += L"\\BonDriver_PTw-ST.ini";

	if(!PathFileExists(strIni.c_str())) {
		strIni = szPath;
		strIni += L"\\BonDriver_PTx-ST.ini";
	}

	m_bUseLNB = GetPrivateProfileInt(L"SET", L"UseLNB", 0, strIni.c_str());
	m_uiVirtualCount = GetPrivateProfileInt(L"SET", L"DMABuff", 8, strIni.c_str()); // ���̒l�͎g�p����Ȃ�
	if( m_uiVirtualCount == 0 ){
		m_uiVirtualCount = 8;
	}

	m_dwMaxDurFREQ = GetPrivateProfileInt(L"SET", L"MAXDUR_FREQ", 1000, strIni.c_str() ); //���g�������ɔ�₷�ő厞��(msec)
	m_dwMaxDurTMCC = GetPrivateProfileInt(L"SET", L"MAXDUR_TMCC", 1500, strIni.c_str() ); //TMCC�擾�ɔ�₷�ő厞��(msec)
	m_dwMaxDurTSID = GetPrivateProfileInt(L"SET", L"MAXDUR_TSID", 3000, strIni.c_str() ); //TSID�ݒ�ɔ�₷�ő厞��(msec)

	m_bAuxiliaryRedirectAll = GetPrivateProfileInt(L"SET", L"AuxiliaryRedirectAll", 0, strIni.c_str());
}
//---------------------------------------------------------------------------
CPTwManager::~CPTwManager(void)
{
	DeleteCriticalSection(&Critical_);
}
//---------------------------------------------------------------------------
BOOL CPTwManager::LoadSDK()
{
	return TRUE ;
}
//---------------------------------------------------------------------------
void CPTwManager::FreeSDK()
{
	StopPrimaryAuxiliary();
	m_cpPrimaryOperator=nullptr;
	for( auto &dev: m_EnumDev ) {
		SAFE_DELETE(dev);
	}
	m_EnumDev.clear();
}
//---------------------------------------------------------------------------
BOOL CPTwManager::Init()
{
	m_EnumDev.clear();

	int deviceInfoCount = PtDrvGetDevCount();

	for (int i=0; i<deviceInfoCount; i++) {
		DEV_STATUS* pItem = new DEV_STATUS;
		m_EnumDev.push_back(pItem);
	}

	DBGOUT("CPTwManager::Init succeeded.\n");
	return TRUE;
}
//---------------------------------------------------------------------------
void CPTwManager::UnInit()
{
	FreeSDK() ;
}
//---------------------------------------------------------------------------
BOOL CPTwManager::IsFindOpen()
{
	critical_lock lock(&Critical_);

	BOOL bFind = FALSE;
	for( auto dev: m_EnumDev ) {
		if( dev->Opened() ){
			bFind = TRUE;
		}
	}
	return bFind;
}
//---------------------------------------------------------------------------
DWORD CPTwManager::GetActiveTunerCount(BOOL bSate)
{
	critical_lock lock(&Critical_);

	DWORD total_used=0;
	for( auto dev : m_EnumDev ){
		if( bSate == FALSE ){
			for(auto op: dev->cpOperatorT) if(op) total_used++;
		}else {
			for(auto op: dev->cpOperatorS) if(op) total_used++;
		}
	}
	return total_used;
}
//---------------------------------------------------------------------------
BOOL CPTwManager::SetLnbPower(int iID, BOOL bEnabled)
{
	critical_lock lock(&Critical_);

	size_t iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	int iTuner = iID&0x000000FF;

	if( iDevID>=m_EnumDev.size() )
		return FALSE;

	if( !m_EnumDev[iDevID]->Opened() )
		return FALSE;

	if(enISDB == PT::Device::ISDB_S) {
		if( iTuner == 0 )
			m_EnumDev[iDevID]->bLnbS0 = bEnabled ;
		else
			m_EnumDev[iDevID]->bLnbS1 = bEnabled ;
	}

	for(auto op: m_EnumDev[iDevID]->cpOperatorS) {
		if(op) {
			op->CmdSetLnbPower(
				m_EnumDev[iDevID]->bLnbS0 || m_EnumDev[iDevID]->bLnbS1 ?
				TRUE : FALSE
			);
		}
	}

	return TRUE;
}
//---------------------------------------------------------------------------
int CPTwManager::OpenTuner(BOOL bSate)
{
	critical_lock lock(&Critical_);

	for(int iDevID=0;iDevID<(int)m_EnumDev.size();iDevID++) {
		for(int iTuner=0;iTuner<2;iTuner++) {
			int iID = OpenTuner2(bSate,iDevID<<1|iTuner);
			if(iID>=0) return iID;
		}
	}
	return -1;
}
//---------------------------------------------------------------------------
int CPTwManager::OpenTuner2(BOOL bSate, int iTunerID)
{
	critical_lock lock(&Critical_);

	PT::Device::ISDB enISDB = bSate ? PT::Device::ISDB_S : PT::Device::ISDB_T;
	wstring bonName;

	auto launch_ctrl = [&]() -> HANDLE {
		if(HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, bonName.c_str())) {
			//���ɋN�����i��q����j
			CloseHandle(hMutex);
			return NULL;
		}
		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		ZeroMemory(&pi,sizeof(pi));
		ZeroMemory(&si,sizeof(si));
		si.cb=sizeof(si);
		wstring cmdline = L"\""+m_strExePath+L"\" "+bonName;
		BOOL bRet = CreateProcess( NULL, (LPTSTR)cmdline.c_str(), NULL
			, NULL, FALSE, GetPriorityClass(GetCurrentProcess()), NULL, NULL, &si, &pi );

		if(pi.hThread&&pi.hThread!=INVALID_HANDLE_VALUE) CloseHandle(pi.hThread);
		if(!bRet&&pi.hProcess&&pi.hProcess!=INVALID_HANDLE_VALUE)
		{ CloseHandle(pi.hProcess); pi.hProcess=NULL; }
		return pi.hProcess ;
	};

	if(iTunerID/2 >= (int)m_EnumDev.size()){
		return -1;
	}

	int iDevID = iTunerID>>1;
	int iTuner = iTunerID&1;
	int iID=-1;
	do {

		if(bSate) {
			if(m_EnumDev[iDevID]->cpOperatorS[iTuner]) break;
		}else {
			if(m_EnumDev[iDevID]->cpOperatorT[iTuner]) break;
		}
		int id = (iDevID<<16) | (enISDB<<8) | iTuner ;
		Format(bonName,SHAREDMEM_TRANSPORT_FORMAT,PT_VER,id) ;
		HANDLE hProcess=NULL;
		if(m_bAuxiliaryRedirectAll || m_cpPrimaryOperator!=nullptr) {
			// NOTE: pt2wdm �ɂ͂P�v���Z�X�ɂ��P�`���[�i�[�Ƃ����䐧��������ׁA
			//       �ʃv���Z�X���R�R�ŗ����グ�ĐV�K�`���[�i�[�ւ̃o�C�p�X���{��
			hProcess = launch_ctrl() ;
			if(!hProcess||hProcess==INVALID_HANDLE_VALUE) break ;
		}
		auto client = new CPTxWDMCmdOperator(bonName);
		auto do_abort = [&]() {
			if(m_cpPrimaryOperator==client) {
				StopPrimaryAuxiliary();
				m_cpPrimaryOperator=nullptr;
			}else
				client->CmdTerminate();
			delete client;
		};
		if(hProcess==NULL) {
			assert(m_cpPrimaryOperator==nullptr);
			m_cpPrimaryOperator=client;
			if(!StartPrimaryAuxiliary()) {
				do_abort(); break;
			}
			hProcess = m_hPrimaryAuxiliaryThread;
		}
		if(!client->CmdOpenTuner(bSate,iDevID<<1|iTuner)) {
			do_abort(); break;
		}
		SERVER_SETTINGS SrvOpt;
		SrvOpt.dwSize = sizeof(SERVER_SETTINGS);
		SrvOpt.CtrlPackets = SHAREDMEM_TRANSPORT_PACKET_NUM ;
		SrvOpt.StreamerThreadPriority = GetThreadPriority(GetCurrentThread()) ;
		SrvOpt.MAXDUR_FREQ = m_dwMaxDurFREQ ;
		SrvOpt.MAXDUR_TMCC = m_dwMaxDurTMCC ;
		SrvOpt.MAXDUR_TSID = m_dwMaxDurTSID ;
		SrvOpt.StreamerPacketSize = SHAREDMEM_TRANSPORT_PACKET_SIZE ;
		if(!client->CmdSetupServer(&SrvOpt)) {
			MessageBeep(MB_ICONEXCLAMATION);
			do_abort(); break;
		}
		if(bSate) {
			m_EnumDev[iDevID]->hProcessS[iTuner] = hProcess ;
			m_EnumDev[iDevID]->cpOperatorS[iTuner] = client ;
		}else {
			m_EnumDev[iDevID]->hProcessT[iTuner] = hProcess ;
			m_EnumDev[iDevID]->cpOperatorT[iTuner] = client ;
		}
		iID = id ;
		//�������J�n
		client->CmdSetTunerSleep(TRUE);
		client->CmdSetTunerSleep(FALSE);
		if(m_bUseLNB) SetLnbPower(iID, TRUE) ;
		client->CmdSetStreamEnable(FALSE);
		client->CmdSetStreamEnable(TRUE);

	}while(0);

	return iID ;
}
//---------------------------------------------------------------------------
BOOL CPTwManager::CloseTuner(int iID)
{
	critical_lock lock(&Critical_);

	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	int iTuner = iID&0x000000FF;

	BOOL bSate = enISDB == PT::Device::ISDB_S ? TRUE : FALSE ;

	if( (int)m_EnumDev.size() <= iDevID ){
		return FALSE;
	}

	CPTxWDMCmdOperator *client = nullptr ;
	HANDLE hProcess = NULL;

	if(bSate) {
		client = m_EnumDev[iDevID]->cpOperatorS[iTuner] ;
		hProcess = m_EnumDev[iDevID]->hProcessS[iTuner] ;
	}else {
		client = m_EnumDev[iDevID]->cpOperatorT[iTuner] ;
		hProcess = m_EnumDev[iDevID]->hProcessT[iTuner] ;
	}

	if(!client) return TRUE ; // already closed

	BOOL bAbnormal = TRUE ;
	if(WaitForSingleObject(hProcess,0)==WAIT_TIMEOUT) {
		if(m_bUseLNB) SetLnbPower(iID, FALSE);
		if(client->CmdSetStreamEnable(FALSE)) {
			if(client->CmdSetTunerSleep(TRUE)) {
				if(client->CmdTerminate())
					bAbnormal = FALSE ;
			}
		}
		if(bAbnormal) {
			if(client->CmdTerminate())
				bAbnormal = FALSE ;
		}
		if(!bAbnormal) {
			if(WaitForSingleObject(hProcess,PTXWDMCMDTIMEOUT)==WAIT_TIMEOUT)
				bAbnormal=TRUE;
		}
	}else bAbnormal=FALSE ;

	if(client==m_cpPrimaryOperator) {
		StopPrimaryAuxiliary() ;
		m_cpPrimaryOperator = nullptr;
		hProcess=NULL;
	}else if(bAbnormal) {
		TerminateProcess(hProcess,-1) ;
	}

	delete client ;
	if(hProcess) CloseHandle(hProcess);

	if(bSate) {
		m_EnumDev[iDevID]->cpOperatorS[iTuner] = nullptr;
		m_EnumDev[iDevID]->hProcessS[iTuner] = NULL;
	}else {
		m_EnumDev[iDevID]->cpOperatorT[iTuner] = nullptr ;
		m_EnumDev[iDevID]->hProcessT[iTuner] = NULL;
	}

	return TRUE ;
}
//---------------------------------------------------------------------------
BOOL CPTwManager::SetCh(int iID, unsigned long ulCh, DWORD dwTSID, BOOL &hasStream)
{
	critical_lock lock(&Critical_);

	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	int iTuner = iID&0x000000FF;

	unsigned long ch = ulCh & 0xffff;
	//�I�t�Z�b�g�͖��������(�I�t�Z�b�g�����@�\�́Apt2wdm �ɂ͑��݂��Ȃ�)

	hasStream=FALSE;

	if( (int)m_EnumDev.size() <= iDevID ){
		return FALSE;
	}

	BOOL bSate = enISDB == PT::Device::ISDB_S ? TRUE : FALSE ;

	auto PtDrvCh = [&]() -> decltype(ch) { // channel converter for pt2wdm
		if(bSate) { // BS/CS110
			return ch;
		}else {
			if(ch>=63) { // UHF (13-62)
				return ch-63 + 51 ;
			}else if(ch>=3&&ch<=12) { // CATV (C13-C22)
				return ch-3 ;
			}else if(ch>=22&&ch<=62) { // CATV (C23-C63)
				return ch-12 ;
			} // VHF �`�����l���́Apt2wdm �ɂ͑��݂��Ȃ�
		}
		return 0xFFFF;
	};

	auto freq = PtDrvCh();
	if(freq==0xFFFF)
		return FALSE ;

	CPTxWDMCmdOperator *client = nullptr ;

	if(bSate) {
		client = m_EnumDev[iDevID]->cpOperatorS[iTuner] ;
	}else {
		client = m_EnumDev[iDevID]->cpOperatorT[iTuner] ;
	}

	if(!client) return FALSE ; // already closed

	DWORD dwStream=0 ;
	if(dwTSID<=7) dwStream = dwTSID, dwTSID = 0 ;

	return client->CmdSetChannel(hasStream, (DWORD)freq, dwTSID, dwStream);
}
//---------------------------------------------------------------------------
DWORD CPTwManager::GetSignal(int iID)
{
	critical_lock lock(&Critical_);

	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	int iTuner = iID&0x000000FF;

	BOOL bSate = enISDB == PT::Device::ISDB_S ? TRUE : FALSE ;

	CPTxWDMCmdOperator *client = nullptr ;

	if(bSate) {
		client = m_EnumDev[iDevID]->cpOperatorS[iTuner] ;
	}else {
		client = m_EnumDev[iDevID]->cpOperatorT[iTuner] ;
	}

	if(!client) return FALSE ; // already closed

	DWORD Cn100=0, CurAgc=0, MaxAgc=0 ;
	client->CmdGetCnAgc(Cn100, CurAgc, MaxAgc) ;

	return Cn100;
}
//---------------------------------------------------------------------------
BOOL CPTwManager::CloseChk()
{
	critical_lock lock(&Critical_);

	// idle checker
	int cntFound = 0 ;
	for(int iDevID=0;iDevID<(int)m_EnumDev.size();iDevID++) {
		if(!m_EnumDev[iDevID]->Opened()) continue;
		for(int iSate=0;iSate<2;iSate++) {
			PT::Device::ISDB enISDB = iSate ? PT::Device::ISDB_S : PT::Device::ISDB_T;
			for(int iTuner=0;iTuner<2;iTuner++) {
			    CPTxWDMCmdOperator *client = nullptr ;
				HANDLE hProcess = NULL;
				if(iSate) {
					client=m_EnumDev[iDevID]->cpOperatorS[iTuner];
					hProcess=m_EnumDev[iDevID]->hProcessS[iTuner];
				}else {
					client=m_EnumDev[iDevID]->cpOperatorT[iTuner];
					hProcess=m_EnumDev[iDevID]->hProcessT[iTuner];
				}
				if(!client||!hProcess) continue;
				if(WaitForSingleObject(hProcess,0)==WAIT_OBJECT_0) {
					int iID = iDevID<<16 | enISDB<<8 | iTuner ;
					CloseTuner(iID);
					continue;
				}
				cntFound++;
			}
		}
	}
	return cntFound>0 ? TRUE : FALSE ;
}
//---------------------------------------------------------------------------
bool CPTwManager::StartPrimaryAuxiliary()
{
	HANDLE &Thread = m_hPrimaryAuxiliaryThread ;
	if(!m_cpPrimaryOperator) return false /*primary AUX Op is not existed*/;
	if(Thread != INVALID_HANDLE_VALUE) return true /*already activated*/;

	Thread = (HANDLE)_beginthreadex(NULL, 0, PrimaryAuxiliaryProc, this,
		CREATE_SUSPENDED, NULL) ;

	if(Thread != INVALID_HANDLE_VALUE) {
		::ResumeThread(Thread) ;
		DBGOUT("-- Start Primary AUX thread --\n");
		return true ;
	}else {
		DBGOUT("*** Primary AUX thread creation failed. ***\n");
	}

	return false;
}
//---------------------------------------------------------------------------
bool CPTwManager::StopPrimaryAuxiliary()
{
	HANDLE &Thread = m_hPrimaryAuxiliaryThread ;
	if(!m_cpPrimaryOperator) return false /*primary AUX Op is not existed*/;
	if(Thread == INVALID_HANDLE_VALUE) return true /*already inactivated*/;

	if(::WaitForSingleObject(Thread,0)==WAIT_TIMEOUT)
		m_cpPrimaryOperator->CmdTerminate();

	if(::WaitForSingleObject(Thread,AuxiliaryMaxAlive*2) != WAIT_OBJECT_0) {
		::TerminateThread(Thread, 0);
	}
	CloseHandle(Thread);
	Thread = INVALID_HANDLE_VALUE ;

	DBGOUT("-- Stop Primary AUX thread --\n");

	return true;
}
//---------------------------------------------------------------------------
unsigned int __stdcall CPTwManager::PrimaryAuxiliaryProc (PVOID pv)
{
	auto this_ = static_cast<CPTwManager*>(pv) ;
	unsigned int result = CPTxWDMCtrlAuxiliary(
		this_->m_cpPrimaryOperator->Name(),
		AuxiliaryMaxWait, AuxiliaryMaxAlive ).MainLoop() ;
	_endthreadex(result) ;
	return result;
}
//===========================================================================
#endif // end of PTW_GALAPAGOS
