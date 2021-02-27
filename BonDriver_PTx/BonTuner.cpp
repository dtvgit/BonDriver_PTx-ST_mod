#include "stdafx.h"
#include <Windows.h>
#include <process.h>
#include <algorithm>
#include <iterator>
#include <vector>
#include <set>

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include "BonTuner.h"

using namespace std;

#define DATA_BUFF_SIZE	(188*256)
#define MAX_BUFF_COUNT	500

static CRITICAL_SECTION secBonTuners;
static set<CBonTuner*> BonTuners ;

void InitializeBonTuners(HMODULE hModule)
{
	::InitializeCriticalSection(&secBonTuners);
	CBonTuner::m_hModule = hModule;
}

void FinalizeBonTuners()
{
	::EnterCriticalSection(&secBonTuners);
	vector<CBonTuner*> clone;
	copy(BonTuners.begin(),BonTuners.end(),back_inserter(clone));
	for(auto bon: clone) if(bon!=NULL) bon->Release();
	::LeaveCriticalSection(&secBonTuners);
	::DeleteCriticalSection(&secBonTuners);
}

#pragma warning( disable : 4273 )
extern "C" __declspec(dllexport) IBonDriver * CreateBonDriver()
{
	// ����v���Z�X����̕����C���X�^���X�擾�\(IBonDriver3�Ή��ɂ��)
	::EnterCriticalSection(&secBonTuners);
	CBonTuner *p = new CBonTuner ;
	if(p!=NULL) BonTuners.insert(p);
	::LeaveCriticalSection(&secBonTuners);
	return p;
}
#pragma warning( default : 4273 )


HINSTANCE CBonTuner::m_hModule = NULL;

	//PTxCtrl���s�t�@�C���̃~���[�e�b�N�X��
	#define PT1_CTRL_MUTEX L"PT1_CTRL_EXE_MUTEX" // PTCtrl.exe
	#define PT3_CTRL_MUTEX L"PT3_CTRL_EXE_MUTEX" // PT3Ctrl.exe
	#define PT2_CTRL_MUTEX L"PT2_CTRL_EXE_MUTEX" // PTwCtrl.exe

	//PTxCtrl�ւ̃R�}���h���M�p�I�u�W�F�N�g
	CPTSendCtrlCmd
        PT1CmdSender(1), PT3CmdSender(3), // PT1/2/3
        PTwCmdSender(2); // pt2wdm


CBonTuner::CBonTuner()
{
	m_hOnStreamEvent = NULL;

	m_LastBuff = NULL;

	m_dwCurSpace = 0xFF;
	m_dwCurChannel = 0xFF;
	m_hasStream = TRUE ;

	m_iID = -1;
	m_hStopEvent = _CreateEvent(FALSE, FALSE, NULL);
	m_hThread = NULL;

	::InitializeCriticalSection(&m_CriticalSection);

	WCHAR strExePath[512] = L"";
	GetModuleFileName(m_hModule, strExePath, 512);

	WCHAR szPath[_MAX_PATH];	// �p�X
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_tsplitpath_s( strExePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_tmakepath_s(  szPath, _MAX_PATH, szDrive, szDir, NULL, NULL );
	m_strDirPath = szPath;

	wstring strIni;
	strIni = szPath;
	strIni += L"BonDriver_PTx-ST.ini";

	auto has_prefix = [](wstring target, wstring prefix) -> bool {
		return !CompareNoCase(prefix,target.substr(0,prefix.length())) ;
	};

	if(has_prefix(szFname,L"BonDriver_PTx"))
		m_iPT=0;
	else if(has_prefix(szFname,L"BonDriver_PTw"))
		m_iPT=2;
	else if(has_prefix(szFname,L"BonDriver_PT3"))
		m_iPT=3;
	else
		m_iPT=1;

	m_isISDB_S = TRUE;
	WCHAR szName[256];
	m_iTunerID = -1;

	_wcsupr_s( szFname, sizeof(szFname) ) ;

	auto parse_fname = [&](wstring ptx, wstring prefix=L"") -> void {
	    WCHAR cTS=L'S'; int id=-1;
		wstring ident = L"BONDRIVER_" + ptx ;
		for(auto &v: ident) v=towupper(v);
		wchar_t wcsID[11]; wcsID[0]=wcsID[10]=0;
		if(swscanf_s(szFname,(ident+L"-%1c%[0-9]%*s").c_str(),&cTS,1,wcsID,10)!=2) {
			id = -1 ;
			if(swscanf_s(szFname,(ident+L"-%1c%*s").c_str(),&cTS,1)!=1)
				cTS = L'S' ;
		}else if(wcsID[0]>=L'0'&&wcsID[0]<=L'9')
			id = _wtoi(wcsID);
	    if(prefix==L"") prefix=ptx ;
		if(cTS==L'T')	m_strTunerName = prefix + L" ISDB-T" , m_isISDB_S = FALSE ;
		else 			m_strTunerName = prefix + L" ISDB-S" ;
		if(id>=0) {
			wsprintfW(szName, L" (%d)", id);
			m_strTunerName += szName ;
			m_iTunerID = id ;
		}
	};

	if(m_iPT==0) { // PTx Tuner ( auto detect )

		int detection = GetPrivateProfileIntW(L"SET", L"xFirstPT3", -1, strIni.c_str());
		m_bXFirstPT3 = detection>=0 ? BOOL(detection) :
			PathFileExists((m_strDirPath+L"PT3Ctrl.exe").c_str()) ;

		parse_fname(L"PTx");

	}else if(m_iPT==2) { // pt2wdm Tuner

		wstring strPTwini = m_strDirPath + L"BonDriver_PTw-ST.ini";
		if(PathFileExists(strPTwini.c_str())) strIni = strPTwini;

		parse_fname(L"PTw");

    }else if(m_iPT==3) { // PT3 Tuner

		wstring strPT3ini = m_strDirPath + L"BonDriver_PT3-ST.ini";
		if(PathFileExists(strPT3ini.c_str())) strIni = strPT3ini;

		parse_fname(L"PT3");

	}else {  // PT Tuner (PT1/2)

		wstring strPTini = wstring(szPath) + L"BonDriver_PT-ST.ini";
		if(PathFileExists(strPTini.c_str())) strIni = strPTini;

		int iPTn = GetPrivateProfileIntW(L"SET", L"PT1Ver", 2, strIni.c_str());
		if(iPTn<1||iPTn>2) iPTn=2;

		parse_fname(L"PT", iPTn==1 ? L"PT1" : L"PT2");
	}

	m_bTrySpares = GetPrivateProfileIntW(L"SET", L"TrySpares", 0, strIni.c_str());
	m_bBon3Lnb = GetPrivateProfileIntW(L"SET", L"Bon3Lnb", 0, strIni.c_str());
	m_bSpeedyScan = GetPrivateProfileIntW(L"SET", L"SpeedyScan", 0, strIni.c_str());
	m_dwSetChDelay = GetPrivateProfileIntW(L"SET", L"SetChDelay", 0, strIni.c_str());

	wstring strChSet;

	//dll���Ɠ������O��.ChSet.txt���ɗD�悵�ēǂݍ��݂����s����
	//(fixed by 2020 LVhJPic0JSk5LiQ1ITskKVk9UGBg)
	strChSet = szPath;	strChSet += szFname;	strChSet += L".ChSet.txt";
	if(!m_chSet.ParseText(strChSet.c_str())) {
		strChSet = szPath;
		if(m_iPT==3) {
			if (m_isISDB_S)
				strChSet += L"BonDriver_PT3-S.ChSet.txt";
			else
				strChSet += L"BonDriver_PT3-T.ChSet.txt";
		}else if(m_iPT==1) {
			if (m_isISDB_S)
				strChSet += L"BonDriver_PT-S.ChSet.txt";
			else
				strChSet += L"BonDriver_PT-T.ChSet.txt";
		}else if(m_iPT==2) {
			if (m_isISDB_S)
				strChSet += L"BonDriver_PTw-S.ChSet.txt";
			else
				strChSet += L"BonDriver_PTw-T.ChSet.txt";
        }
		if(!m_iPT||!m_chSet.ParseText(strChSet.c_str())) {
			strChSet = szPath;
			if (m_isISDB_S)
				strChSet += L"BonDriver_PTx-S.ChSet.txt";
			else
				strChSet += L"BonDriver_PTx-T.ChSet.txt";
			if(!m_chSet.ParseText(strChSet.c_str()))
				BuildDefSpace(strIni);
		}
	}

	switch(m_iPT ? m_iPT : m_bXFirstPT3 ? 3 : 1) {
	case 1:	m_pCmdSender = &PT1CmdSender; break;
	case 3:	m_pCmdSender = &PT3CmdSender; break;
	case 2:	m_pCmdSender = &PTwCmdSender; break; // pt2wdm
	}
}

CBonTuner::~CBonTuner()
{
	CloseTuner();

	::EnterCriticalSection(&m_CriticalSection);
	SAFE_DELETE(m_LastBuff);
	::LeaveCriticalSection(&m_CriticalSection);

	::CloseHandle(m_hStopEvent);
	m_hStopEvent = NULL;

	::DeleteCriticalSection(&m_CriticalSection);

	::EnterCriticalSection(&secBonTuners);
	BonTuners.erase(this);
	::LeaveCriticalSection(&secBonTuners);
}

void CBonTuner::BuildDefSpace(wstring strIni)
{
	//.ChSet.txt�����݂��Ȃ��ꍇ�́A����̃`�����l�������\�z����
	//(added by 2021 LVhJPic0JSk5LiQ1ITskKVk9UGBg)

	BOOL UHF=TRUE, CATV=FALSE, VHF=FALSE, BS=TRUE, CS110=TRUE;
	DWORD BSStreams=8, CS110Streams=8;
	BOOL BSStreamStride=FALSE, CS110StreamStride=FALSE;

#define LOADDW(nam) do {\
		nam=(DWORD)GetPrivateProfileIntW(L"DefSpace", L#nam, nam, strIni.c_str()); \
	}while(0)

	LOADDW(UHF);
	LOADDW(CATV);
	LOADDW(VHF);
	LOADDW(BS);
	LOADDW(CS110);
	LOADDW(BSStreams);
	LOADDW(CS110Streams);
	LOADDW(BSStreamStride);
	LOADDW(CS110StreamStride);

#undef LOADDW

	DWORD spc=0 ;
	auto entry_spc = [&](const wchar_t *space_name) {
		SPACE_DATA item;
		item.wszName=space_name;
		item.dwSpace=spc++;
		m_chSet.spaceMap.insert( pair<DWORD, SPACE_DATA>(item.dwSpace,item) );
	};

	if(m_isISDB_S) {  // BS / CS110

		DWORD i,ch,ts,pt1offs;
		auto entry_ch = [&](const wchar_t *prefix, bool suffix) {
			CH_DATA item ;
			Format(item.wszName,suffix?L"%s%02d/TS%d":L"%s%02d",prefix,ch,ts);
			item.dwSpace=spc;
			item.dwCh=i;
			item.dwPT1Ch=(ch-1)/2+pt1offs;
			item.dwTSID=ts;
			DWORD iKey = (item.dwSpace<<16) | item.dwCh;
			m_chSet.chMap.insert( pair<DWORD, CH_DATA>(iKey,item) );
		};

		if(BS) {
			pt1offs=0;
			if(BSStreamStride) {
				for(i=0,ts=0;ts<(BSStreams>0?BSStreams:1);ts++)
				for(ch=1;ch<=23;ch+=2,i++)
					entry_ch(L"BS",BSStreams>0);
			}else {
				for(i=0,ch=1;ch<=23;ch+=2)
				for(ts=0;ts<(BSStreams>0?BSStreams:1);ts++,i++)
					entry_ch(L"BS",BSStreams>0);
			}
			entry_spc(L"BS");
		}

		if(CS110) {
			pt1offs=12;
			if(CS110StreamStride) {
				for(i=0,ts=0;ts<(CS110Streams>0?CS110Streams:1);ts++)
				for(ch=2;ch<=24;ch+=2,i++)
					entry_ch(L"ND",CS110Streams>0);
			}else {
				for(i=0,ch=2;ch<=24;ch+=2)
				for(ts=0;ts<(CS110Streams>0?CS110Streams:1);ts++,i++)
					entry_ch(L"ND",CS110Streams>0);
			}
			entry_spc(L"CS110");
		}

	}else { // �n�f�W

		DWORD i,offs,C;
		auto entry_ch = [&](DWORD (*pt1conv)(DWORD i)) {
			CH_DATA item;
			Format(item.wszName,C?L"C%dCh":L"%dCh",i+offs);
			item.dwSpace=spc;
			item.dwCh=i;
			item.dwPT1Ch=pt1conv(i);
			DWORD iKey = (item.dwSpace<<16) | item.dwCh;
			m_chSet.chMap.insert( pair<DWORD, CH_DATA>(iKey,item) );
		};

		if(UHF) {
			for(offs=13,C=i=0;i<50;i++) entry_ch([](DWORD i){return i+63;});
			entry_spc(L"�n�f�W(UHF)") ;
		}

		if(CATV) {
			for(offs=13,C=1,i=0;i<51;i++) entry_ch([](DWORD i){return i+(i>=10?12:3);});
			entry_spc(L"�n�f�W(CATV)") ;
		}

		if(VHF) {
			for(offs=1,C=i=0;i<12;i++) entry_ch([](DWORD i){return i+(i>=3?10:0);});
			entry_spc(L"�n�f�W(VHF)") ;
		}

	}
}

BOOL CBonTuner::LaunchPTCtrl(int iPT)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	ZeroMemory(&si,sizeof(si));
	si.cb=sizeof(si);

	wstring strPTCtrlExe = m_strDirPath ;
	wstring mutexName ;

	switch(iPT) {
	case 1:
		strPTCtrlExe += L"PTCtrl.exe" ;
		mutexName = PT1_CTRL_MUTEX ;
		break ;
	case 3:
		strPTCtrlExe += L"PT3Ctrl.exe" ;
		mutexName = PT3_CTRL_MUTEX ;
		break ;
	case 2:
		strPTCtrlExe += L"PTwCtrl.exe" ;
		mutexName = PT2_CTRL_MUTEX ;
		break ;
	}

	if(HANDLE Mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, mutexName.c_str())) {
		// ���ɋN����
		CloseHandle(Mutex) ;
		return TRUE ;
	}

	if(!PathFileExists(strPTCtrlExe.c_str())) {
		// ���s�t�@�C�������݂��Ȃ�
		return FALSE;
	}

	BOOL bRet = CreateProcessW( NULL, (LPWSTR)strPTCtrlExe.c_str(), NULL, NULL, FALSE, GetPriorityClass(GetCurrentProcess()), NULL, NULL, &si, &pi );
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	_RPT3(_CRT_WARN, "*** CBonTuner::LaunchPTCtrl() ***\nbRet[%s]", bRet ? "TRUE" : "FALSE");

	return bRet ;
}

BOOL CBonTuner::TryOpenTuner(int iTunerID, int *piID)
{
	DWORD dwRet;
	if( iTunerID >= 0 ){
		dwRet = m_pCmdSender->OpenTuner2(m_isISDB_S, iTunerID, piID);
	}else{
		dwRet = m_pCmdSender->OpenTuner(m_isISDB_S, piID);
	}

	_RPT3(_CRT_WARN, "*** CBonTuner::TryOpenTuner() ***\ndwRet[%u]\n", dwRet);

	if( dwRet != CMD_SUCCESS ){
		return FALSE;
	}

	return TRUE;
}

const BOOL CBonTuner::OpenTuner(void)
{
	//�C�x���g
	m_hOnStreamEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	_RPT3(_CRT_WARN, "*** CBonTuner::OpenTuner() ***\nm_hOnStreamEvent[%p]\n", m_hOnStreamEvent);

	if(!m_iPT) { // PTx ( PT1/2/3 - auto detect )

		//PTx�������o�@�\�̒ǉ�
		//(added by 2021 LVhJPic0JSk5LiQ1ITskKVk9UGBg)
		BOOL opened = FALSE ;
		int tid = m_iTunerID ;
		for(int i=0;i<2;i++) {
			int iPT = m_bXFirstPT3 ? (i?1:3) : (i?3:1) ;
			if(!LaunchPTCtrl(iPT))
				continue;
			switch(iPT) {
			case 1:	m_pCmdSender = &PT1CmdSender; break;
			case 3:	m_pCmdSender = &PT3CmdSender; break;
			}
			DWORD dwNumTuner=0;
			if(m_pCmdSender->GetTotalTunerCount(&dwNumTuner) == CMD_SUCCESS) {
				if(tid>=0 && DWORD(tid)>=dwNumTuner) {
					tid-=dwNumTuner ;
					continue;
				}
				m_iID=-1 ;
				if(TryOpenTuner(tid, &m_iID)) {
					opened = TRUE; break;
				}else if(m_bTrySpares) {
					if(tid>=0) tid=-1, i=-1 ;
				}
				if(tid>=0) break;
			}
		}
		if(!opened) return FALSE ;

	}else { // PT1/2/3 or pt2wdm ( manual )

		if(!LaunchPTCtrl(m_iPT)) return FALSE;
		if(!TryOpenTuner(m_iTunerID, &m_iID)){
			if(m_iTunerID<0 || !m_bTrySpares || !TryOpenTuner(-1, &m_iID))
				return FALSE;

		}

	}

	m_hThread = (HANDLE)_beginthreadex(NULL, 0, RecvThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
	ResumeThread(m_hThread);

	return TRUE;
}

void CBonTuner::CloseTuner(void)
{
	if( m_hThread != NULL ){
		::SetEvent(m_hStopEvent);
		// �X���b�h�I���҂�
		if ( ::WaitForSingleObject(m_hThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hThread, 0xffffffff);
		}
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	m_dwCurSpace = 0xFF;
	m_dwCurChannel = 0xFF;
	m_hasStream = TRUE;

	::CloseHandle(m_hOnStreamEvent);
	m_hOnStreamEvent = NULL;

	if( m_iID != -1 ){
		m_pCmdSender->CloseTuner(m_iID);
		m_iID = -1;
	}

	//�o�b�t�@���
	::EnterCriticalSection(&m_CriticalSection);
	while (!m_TsBuff.empty()){
		TS_DATA *p = m_TsBuff.front();
		m_TsBuff.pop_front();
		delete p;
	}
	::LeaveCriticalSection(&m_CriticalSection);
}

const BOOL CBonTuner::SetChannel(const BYTE bCh)
{
	return TRUE;
}

const float CBonTuner::GetSignalLevel(void)
{
	if( m_iID == -1 || !m_hasStream){
		return 0;
	}
	DWORD dwCn100;
	if( m_pCmdSender->GetSignal(m_iID, &dwCn100) == CMD_SUCCESS ){
		return ((float)dwCn100) / 100.0f;
	}else{
		return 0;
	}
}

const DWORD CBonTuner::WaitTsStream(const DWORD dwTimeOut)
{
	if( m_hOnStreamEvent == NULL ){
		return WAIT_ABANDONED;
	}
	// �C�x���g���V�O�i����ԂɂȂ�̂�҂�
	const DWORD dwRet = ::WaitForSingleObject(m_hOnStreamEvent, (dwTimeOut)? dwTimeOut : INFINITE);

	switch(dwRet){
		case WAIT_ABANDONED :
			// �`���[�i������ꂽ
			return WAIT_ABANDONED;

		case WAIT_OBJECT_0 :
		case WAIT_TIMEOUT :
			// �X�g���[���擾�\
			return dwRet;

		case WAIT_FAILED :
		default:
			// ��O
			return WAIT_FAILED;
	}
}

const DWORD CBonTuner::GetReadyCount(void)
{
	DWORD dwCount = 0;
	::EnterCriticalSection(&m_CriticalSection);
	if(m_hasStream) dwCount = (DWORD)m_TsBuff.size();
	::LeaveCriticalSection(&m_CriticalSection);
	return dwCount;
}

const BOOL CBonTuner::GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	BYTE *pSrc = NULL;

	if(GetTsStream(&pSrc, pdwSize, pdwRemain)){
		if(*pdwSize){
			::CopyMemory(pDst, pSrc, *pdwSize);
		}
		return TRUE;
	}
	return FALSE;
}

const BOOL CBonTuner::GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	BOOL bRet;
	::EnterCriticalSection(&m_CriticalSection);
	if( m_hasStream && m_TsBuff.size() != 0 ){
		delete m_LastBuff;
		m_LastBuff = m_TsBuff.front();
		m_TsBuff.pop_front();
		*pdwSize = m_LastBuff->dwSize;
		*ppDst = m_LastBuff->pbBuff;
		*pdwRemain = (DWORD)m_TsBuff.size();
		bRet = TRUE;
	}else{
		*pdwSize = 0;
		*pdwRemain = 0;
		bRet = FALSE;
	}
	::LeaveCriticalSection(&m_CriticalSection);
	return bRet;
}

void CBonTuner::PurgeTsStream(void)
{
	//�o�b�t�@���
	::EnterCriticalSection(&m_CriticalSection);
	while (!m_TsBuff.empty()){
		TS_DATA *p = m_TsBuff.front();
		m_TsBuff.pop_front();
		delete p;
	}
	::LeaveCriticalSection(&m_CriticalSection);
}

LPCTSTR CBonTuner::GetTunerName(void)
{
	return m_strTunerName.c_str();
}

const BOOL CBonTuner::IsTunerOpening(void)
{
	return FALSE;
}

LPCTSTR CBonTuner::EnumTuningSpace(const DWORD dwSpace)
{
	map<DWORD, SPACE_DATA>::iterator itr;
	itr = m_chSet.spaceMap.find(dwSpace);
	if( itr == m_chSet.spaceMap.end() ){
		return NULL;
	}else{
		return itr->second.wszName.c_str();
	}
}

LPCTSTR CBonTuner::EnumChannelName(const DWORD dwSpace, const DWORD dwChannel)
{
	DWORD key = dwSpace<<16 | dwChannel;
	map<DWORD, CH_DATA>::iterator itr;
	itr = m_chSet.chMap.find(key);
	if( itr == m_chSet.chMap.end() ){
		return NULL;
	}else{
		return itr->second.wszName.c_str();
	}
}

const BOOL CBonTuner::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	DWORD key = dwSpace<<16 | dwChannel;
	map<DWORD, CH_DATA>::iterator itr;
	itr = m_chSet.chMap.find(key);
	if (itr == m_chSet.chMap.end()) {
		return FALSE;
	}

	m_hasStream=FALSE ;

	DWORD dwRet=CMD_ERR;
	if( m_iID != -1 ){
		dwRet=m_pCmdSender->SetCh(m_iID, itr->second.dwPT1Ch, itr->second.dwTSID);
	}else{
		return FALSE;
	}

	if (m_dwSetChDelay)
		Sleep(m_dwSetChDelay);

	PurgeTsStream();

	m_hasStream = (dwRet&CMD_BIT_NON_STREAM) ? FALSE : TRUE ;
	dwRet &= ~CMD_BIT_NON_STREAM ;

	if( dwRet==CMD_SUCCESS ){
		m_dwCurSpace = dwSpace;
		m_dwCurChannel = dwChannel;
		if(m_bSpeedyScan) return m_hasStream ? TRUE : FALSE ;
		return TRUE ;
	}

	return FALSE;
}

const DWORD CBonTuner::GetCurSpace(void)
{
	return m_dwCurSpace;
}

const DWORD CBonTuner::GetCurChannel(void)
{
	return m_dwCurChannel;
}

void CBonTuner::Release()
{
	delete this;
}

UINT WINAPI CBonTuner::RecvThread(LPVOID pParam)
{
	CBonTuner* pSys = (CBonTuner*)pParam;


	while (1) {
		if (::WaitForSingleObject( pSys->m_hStopEvent, 0 ) != WAIT_TIMEOUT) {
			//���~
			break;
		}
		DWORD dwSize;
		BYTE *pbBuff;
		if ((pSys->m_pCmdSender->SendData(pSys->m_iID, &pbBuff, &dwSize) == CMD_SUCCESS) && (dwSize != 0)) {
			if(pSys->m_hasStream) {
				TS_DATA *pData = new TS_DATA(pbBuff, dwSize);
				::EnterCriticalSection(&pSys->m_CriticalSection);
				while (pSys->m_TsBuff.size() > MAX_BUFF_COUNT) {
					TS_DATA *p = pSys->m_TsBuff.front();
					pSys->m_TsBuff.pop_front();
					delete p;
				}
				pSys->m_TsBuff.push_back(pData);
				::LeaveCriticalSection(&pSys->m_CriticalSection);
				::SetEvent(pSys->m_hOnStreamEvent);
			}else {
				//�x�~
				delete [] pbBuff ;
			}
		}else{
			if(!pSys->m_hasStream) pSys->PurgeTsStream();
			::Sleep(5);
		}
	}

	return 0;
}

void CBonTuner::GetTunerCounters(DWORD *lpdwTotal, DWORD *lpdwActive)
{
	if(m_iTunerID>=0) { // ID�Œ�`���[�i�[
		if(lpdwTotal) *lpdwTotal = 1 ;
		if(lpdwActive) *lpdwActive = m_hThread ? 1 : 0 ;
	}else { // ID�������蓖�ă`���[�i�[
		if(lpdwTotal) *lpdwTotal=0;
		if(lpdwActive) *lpdwActive=0;
		for(int i=1;i<=3;i++) {
			if((!m_iPT&&i!=2)||m_iPT==i) {
				if(LaunchPTCtrl(i)) {
					CPTSendCtrlCmd *sender;
					switch(i) {
					case 1:	sender = &PT1CmdSender; break;
					case 3:	sender = &PT3CmdSender; break;
					case 2:	sender = &PTwCmdSender; break;
					}
					DWORD dwNumTuner=0;
					if(lpdwTotal && sender->GetTotalTunerCount(&dwNumTuner) == CMD_SUCCESS) {
						*lpdwTotal += dwNumTuner ;
					}
					dwNumTuner=0;
					if(lpdwActive && sender->GetActiveTunerCount(m_isISDB_S,&dwNumTuner) == CMD_SUCCESS) {
						*lpdwActive += dwNumTuner ;
					}
				}
			}
		}
	}
}

	//IBonDriver3�̋@�\��ǉ�
	//(added by 2021 LVhJPic0JSk5LiQ1ITskKVk9UGBg)

const DWORD CBonTuner::GetTotalDeviceNum(void)
{
	DWORD nTotal=0;
	GetTunerCounters(&nTotal,NULL);
	return nTotal;
}

const DWORD CBonTuner::GetActiveDeviceNum(void)
{
	DWORD nActive=0;
	GetTunerCounters(NULL,&nActive);
	return nActive;
}

const BOOL CBonTuner::SetLnbPower(const BOOL bEnable)
{
	//�`���[�i�[���I�[�v��������ԂŌĂ΂Ȃ��Ɛ�����behavior�͊��҂ł��Ȃ�
	if(!m_isISDB_S) return FALSE;
	if(!m_bBon3Lnb) return TRUE;
	if(!m_hThread) return FALSE;
	if(m_iID<0) return FALSE;
	return m_pCmdSender->SetLnbPower(m_iID,bEnable) == CMD_SUCCESS ? TRUE : FALSE ;
}

