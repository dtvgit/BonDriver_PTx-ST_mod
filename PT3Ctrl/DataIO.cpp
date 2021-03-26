#include "StdAfx.h"
#include "DataIO.h"
#include <process.h>

#define NOT_SYNC_BYTE		0x74

CDataIO::CDataIO(BOOL bMemStreaming)
 : m_T0Buff(MAX_DATA_BUFF_COUNT, 1), m_T1Buff(MAX_DATA_BUFF_COUNT, 1),
 	m_S0Buff(MAX_DATA_BUFF_COUNT, 1), m_S1Buff(MAX_DATA_BUFF_COUNT, 1)
{
	VIRTUAL_COUNT = 8*8;

	m_hThread1 = INVALID_HANDLE_VALUE;
	m_hThread2 = INVALID_HANDLE_VALUE;
	m_hThread3 = INVALID_HANDLE_VALUE;
	m_hThread4 = INVALID_HANDLE_VALUE;

	m_pcDevice = NULL;

	m_T0SetBuff = NULL;
	m_T1SetBuff = NULL;
	m_S0SetBuff = NULL;
	m_S1SetBuff = NULL;

	m_hEvent1 = _CreateEvent(FALSE, TRUE, NULL );
	m_hEvent2 = _CreateEvent(FALSE, TRUE, NULL );
	m_hEvent3 = _CreateEvent(FALSE, TRUE, NULL );
	m_hEvent4 = _CreateEvent(FALSE, TRUE, NULL );

	m_hBuffEvent1 = _CreateEvent(FALSE, TRUE, NULL );
	m_hBuffEvent2 = _CreateEvent(FALSE, TRUE, NULL );
	m_hBuffEvent3 = _CreateEvent(FALSE, TRUE, NULL );
	m_hBuffEvent4 = _CreateEvent(FALSE, TRUE, NULL );

	m_dwT0OverFlowCount = 0;
	m_dwT1OverFlowCount = 0;
	m_dwS0OverFlowCount = 0;
	m_dwS1OverFlowCount = 0;

	for(auto &c : m_fDataCarry) c = false ;

	// MemStreaming
	m_bMemStreaming = bMemStreaming ;
	m_bMemStreamingTerm = TRUE ;
	m_T0MemStreamer = NULL ;
	m_T1MemStreamer = NULL ;
	m_S0MemStreamer = NULL ;
	m_S1MemStreamer = NULL ;
	m_hT0MemStreamingThread = INVALID_HANDLE_VALUE ;
	m_hT1MemStreamingThread = INVALID_HANDLE_VALUE ;
	m_hS0MemStreamingThread = INVALID_HANDLE_VALUE ;
	m_hS1MemStreamingThread = INVALID_HANDLE_VALUE ;
}

CDataIO::~CDataIO(void)
{
	Stop();

	// MemStreaming
	SAFE_DELETE(m_T0MemStreamer);
	SAFE_DELETE(m_T1MemStreamer);
	SAFE_DELETE(m_S0MemStreamer);
	SAFE_DELETE(m_S1MemStreamer);

	SAFE_DELETE(m_T0SetBuff);
	SAFE_DELETE(m_T1SetBuff);
	SAFE_DELETE(m_S0SetBuff);
	SAFE_DELETE(m_S1SetBuff);

	Flush(m_T0Buff, TRUE);
	Flush(m_T1Buff, TRUE);
	Flush(m_S0Buff, TRUE);
	Flush(m_S1Buff, TRUE);

	if( m_hEvent1 != NULL ){
		UnLock1();
		CloseHandle(m_hEvent1);
		m_hEvent1 = NULL;
	}
	if( m_hEvent2 != NULL ){
		UnLock2();
		CloseHandle(m_hEvent2);
		m_hEvent2 = NULL;
	}
	if( m_hEvent3 != NULL ){
		UnLock3();
		CloseHandle(m_hEvent3);
		m_hEvent3 = NULL;
	}
	if( m_hEvent4 != NULL ){
		UnLock4();
		CloseHandle(m_hEvent4);
		m_hEvent4 = NULL;
	}

	if( m_hBuffEvent1 != NULL ){
		BuffUnLock1();
		CloseHandle(m_hBuffEvent1);
		m_hBuffEvent1 = NULL;
	}
	if( m_hBuffEvent2 != NULL ){
		BuffUnLock2();
		CloseHandle(m_hBuffEvent2);
		m_hBuffEvent2 = NULL;
	}
	if( m_hBuffEvent3 != NULL ){
		BuffUnLock3();
		CloseHandle(m_hBuffEvent3);
		m_hBuffEvent3 = NULL;
	}
	if( m_hBuffEvent4 != NULL ){
		BuffUnLock4();
		CloseHandle(m_hBuffEvent4);
		m_hBuffEvent4 = NULL;
	}
}

bool CDataIO::Lock1(DWORD timeout)
{
	if( m_hEvent1 == NULL ){
		return false ;
	}
	if( WaitForSingleObject(m_hEvent1, timeout) == WAIT_TIMEOUT ){
		OutputDebugString(L"time out1");
		return false ;
	}
	return true ;
}

void CDataIO::UnLock1()
{
	if( m_hEvent1 != NULL ){
		SetEvent(m_hEvent1);
	}
}

bool CDataIO::Lock2(DWORD timeout)
{
	if( m_hEvent2 == NULL ){
		return false ;
	}
	if( WaitForSingleObject(m_hEvent2, timeout) == WAIT_TIMEOUT ){
		OutputDebugString(L"time out2");
		return false ;
	}
	return true ;
}

void CDataIO::UnLock2()
{
	if( m_hEvent2 != NULL ){
		SetEvent(m_hEvent2);
	}
}

bool CDataIO::Lock3(DWORD timeout)
{
	if( m_hEvent3 == NULL ){
		return false ;
	}
	if( WaitForSingleObject(m_hEvent3, timeout) == WAIT_TIMEOUT ){
		OutputDebugString(L"time out3");
		return false ;
	}
	return true ;
}

void CDataIO::UnLock3()
{
	if( m_hEvent3 != NULL ){
		SetEvent(m_hEvent3);
	}
}

bool CDataIO::Lock4(DWORD timeout)
{
	if( m_hEvent4 == NULL ){
		return false ;
	}
	if( WaitForSingleObject(m_hEvent4, timeout) == WAIT_TIMEOUT ){
		OutputDebugString(L"time out4");
		return false ;
	}
	return true ;
}

void CDataIO::UnLock4()
{
	if( m_hEvent4 != NULL ){
		SetEvent(m_hEvent4);
	}
}

bool CDataIO::BuffLock1(DWORD timeout)
{
	if( m_hBuffEvent1 == NULL ){
		return false ;
	}
	if( WaitForSingleObject(m_hBuffEvent1, timeout) == WAIT_TIMEOUT ){
		OutputDebugString(L"time out1b");
		return false ;
	}
	return true ;
}

void CDataIO::BuffUnLock1()
{
	if( m_hBuffEvent1 != NULL ){
		SetEvent(m_hBuffEvent1);
	}
}

bool CDataIO::BuffLock2(DWORD timeout)
{
	if( m_hBuffEvent2 == NULL ){
		return false ;
	}
	if( WaitForSingleObject(m_hBuffEvent2, timeout) == WAIT_TIMEOUT ){
		OutputDebugString(L"time out2b");
		return false ;
	}
	return true ;
}

void CDataIO::BuffUnLock2()
{
	if( m_hBuffEvent2 != NULL ){
		SetEvent(m_hBuffEvent2);
	}
}

bool CDataIO::BuffLock3(DWORD timeout)
{
	if( m_hBuffEvent3 == NULL ){
		return false ;
	}
	if( WaitForSingleObject(m_hBuffEvent3, timeout) == WAIT_TIMEOUT ){
		OutputDebugString(L"time out3b");
		return false ;
	}
	return true ;
}

void CDataIO::BuffUnLock3()
{
	if( m_hBuffEvent3 != NULL ){
		SetEvent(m_hBuffEvent3);
	}
}

bool CDataIO::BuffLock4(DWORD timeout)
{
	if( m_hBuffEvent4 == NULL ){
		return false ;
	}
	if( WaitForSingleObject(m_hBuffEvent4, timeout) == WAIT_TIMEOUT ){
		OutputDebugString(L"time out4b");
		return false ;
	}
	return true ;
}

void CDataIO::BuffUnLock4()
{
	if( m_hBuffEvent4 != NULL ){
		SetEvent(m_hBuffEvent4);
	}
}


void CDataIO::ClearBuff(int iID)
{
	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	uint32 iTuner = iID&0x000000FF;

	auto clear = [&](DWORD dwID) {
		Lock(dwID);
		BuffLock(dwID);
		OverFlowCount(dwID) = 0;
		Flush(Buff(dwID));
		BuffUnLock(dwID);
		UnLock(dwID);
	};

	if( enISDB == PT::Device::ISDB_T )
		clear( iTuner==0 ? 0 : 1 );
	else
		clear( iTuner==0 ? 2 : 3 );
}

void CDataIO::Run(int iID)
{
	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	uint32 iTuner = iID&0x000000FF;

	if( m_hThread1 == INVALID_HANDLE_VALUE &&
		m_hThread2 == INVALID_HANDLE_VALUE &&
		m_hThread3 == INVALID_HANDLE_VALUE &&
		m_hThread4 == INVALID_HANDLE_VALUE ) {

		m_bThTerm=FALSE;
	}

	if(enISDB == PT::Device::ISDB_T) {
		// Thread 1 - T0
		if(iTuner==0 && m_hThread1==INVALID_HANDLE_VALUE) {
			m_hThread1 = (HANDLE)_beginthreadex(NULL, 0, RecvThreadProc,
				(LPVOID) new RECVTHREAD_PARAM(this,0), CREATE_SUSPENDED, NULL);
			SetThreadPriority( m_hThread1, THREAD_PRIORITY_ABOVE_NORMAL );
			ResumeThread(m_hThread1);
			// MemStreaming
			StartMemStreaming(0);
		}
		// Thread 2 - T1
		else if(iTuner==1 && m_hThread2==INVALID_HANDLE_VALUE) {
			m_hThread2 = (HANDLE)_beginthreadex(NULL, 0, RecvThreadProc,
				(LPVOID) new RECVTHREAD_PARAM(this,1), CREATE_SUSPENDED, NULL);
			SetThreadPriority( m_hThread2, THREAD_PRIORITY_ABOVE_NORMAL );
			ResumeThread(m_hThread2);
			// MemStreaming
			StartMemStreaming(1);
		}
	}else if(enISDB == PT::Device::ISDB_S) {
		// Thread 3 - S0
		if(iTuner==0 && m_hThread3==INVALID_HANDLE_VALUE) {
			m_hThread3 = (HANDLE)_beginthreadex(NULL, 0, RecvThreadProc,
				(LPVOID) new RECVTHREAD_PARAM(this,2), CREATE_SUSPENDED, NULL);
			SetThreadPriority( m_hThread3, THREAD_PRIORITY_ABOVE_NORMAL );
			ResumeThread(m_hThread3);
			// MemStreaming
			StartMemStreaming(2);
		}
		// Thread 4 - S1
		else if(iTuner==1 && m_hThread4==INVALID_HANDLE_VALUE) {
			m_hThread4 = (HANDLE)_beginthreadex(NULL, 0, RecvThreadProc,
				(LPVOID) new RECVTHREAD_PARAM(this,3), CREATE_SUSPENDED, NULL);
			SetThreadPriority( m_hThread4, THREAD_PRIORITY_ABOVE_NORMAL );
			ResumeThread(m_hThread4);
			// MemStreaming
			StartMemStreaming(3);
		}
	}
}

void CDataIO::Stop()
{
	if( m_hThread1 != INVALID_HANDLE_VALUE ||
		m_hThread2 != INVALID_HANDLE_VALUE ||
		m_hThread3 != INVALID_HANDLE_VALUE ||
		m_hThread4 != INVALID_HANDLE_VALUE ){
		// �X���b�h�I���҂�
		HANDLE handles[4];
		DWORD cnt=0;
		if(m_hThread1!=INVALID_HANDLE_VALUE) handles[cnt++]=m_hThread1 ;
		if(m_hThread2!=INVALID_HANDLE_VALUE) handles[cnt++]=m_hThread2 ;
		if(m_hThread3!=INVALID_HANDLE_VALUE) handles[cnt++]=m_hThread3 ;
		if(m_hThread4!=INVALID_HANDLE_VALUE) handles[cnt++]=m_hThread4 ;
		m_bThTerm=TRUE;
		if ( ::WaitForMultipleObjects(cnt,handles,TRUE, 15000) == WAIT_TIMEOUT ){
			for(DWORD i=0;i<cnt;i++)
				if(::WaitForSingleObject(handles[i],0)!=WAIT_OBJECT_0)
					::TerminateThread(handles[i], 0xffffffff);
		}
		for(DWORD i=0;i<cnt;i++)
			CloseHandle(handles[i]);
		m_hThread1 = INVALID_HANDLE_VALUE;
		m_hThread2 = INVALID_HANDLE_VALUE;
		m_hThread3 = INVALID_HANDLE_VALUE;
		m_hThread4 = INVALID_HANDLE_VALUE;
	}

	// MemStreaming
	StopMemStreaming();
}

void CDataIO::StartPipeServer(int iID)
{
	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	uint32 iTuner = iID&0x000000FF;

	wstring strPipe = L"";
	wstring strEvent = L"";
	Format(strPipe, L"%s%d", CMD_PT1_DATA_PIPE, iID );
	Format(strEvent, L"%s%d", CMD_PT1_DATA_EVENT_WAIT_CONNECT, iID );

	// MemStreaming
	wstring strStreamerName;
	Format(strStreamerName, SHAREDMEM_TRANSPORT_STREAM_FORMAT, PT_VER, iID);

	status enStatus = m_pcDevice->SetTransferTestMode(enISDB, iTuner);
	if( enStatus != PT::STATUS_OK ){
		return ;
	}

	auto start = [&](DWORD dwID) {
		Lock(dwID);
		auto &buff = SetBuff(dwID);
		if( buff == NULL ){
			buff = new EARTH::EX::Buffer(m_pcDevice);
			buff->Alloc(DATA_UNIT_SIZE, VIRTUAL_COUNT);
			WriteIndex(dwID) = 0;

			uint8 *ptr = static_cast<uint8 *>(buff->Ptr(0));
			for (uint32 i=0; i<VIRTUAL_COUNT; i++) {
				ptr[DATA_UNIT_SIZE*i] = NOT_SYNC_BYTE;	// �����o�C�g������ NOT_SYNC_BYTE ����������
				buff->SyncCpu(i);		// CPU �L���b�V�����t���b�V��
			}

			uint64 pageAddress = buff->PageDescriptorAddress();

			enStatus = m_pcDevice->SetTransferPageDescriptorAddress(
				enISDB, iTuner, pageAddress);

			BuffLock(dwID);
			OverFlowCount(dwID) = 0;
			auto &st = MemStreamer(dwID) ;
			if(m_bMemStreaming&&st==NULL)
				st = new CSharedTransportStreamer(
					strStreamerName, FALSE, SHAREDMEM_TRANSPORT_PACKET_SIZE,
					SHAREDMEM_TRANSPORT_PACKET_NUM);
			BuffUnLock(dwID);
		}
		UnLock(dwID);
		if(!m_bMemStreaming)
			Pipe(dwID).StartServer(strEvent.c_str(), strPipe.c_str(),
				OutsideCmdCallback(dwID), this, THREAD_PRIORITY_ABOVE_NORMAL);
	};


	if( enISDB == PT::Device::ISDB_T )
		start( iTuner == 0 ? 0 : 1 );
	else
		start( iTuner == 0 ? 2 : 3 );
}

void CDataIO::StopPipeServer(int iID)
{
	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	uint32 iTuner = iID&0x000000FF;

	auto stop = [&](DWORD dwID) {
		if(!m_bMemStreaming) Pipe(dwID).StopServer();
		Lock(dwID);
		BuffLock(dwID);
		m_fDataCarry[dwID] = false;
		auto &st = MemStreamer(dwID);
		SAFE_DELETE(st);
		OverFlowCount(dwID) = 0;
		auto &buff = SetBuff(dwID);
		SAFE_DELETE(buff);
		Flush(Buff(dwID));
		BuffUnLock(dwID);
		UnLock(dwID);
	};

	if( enISDB == PT::Device::ISDB_T )
		stop( iTuner == 0 ? 0 : 1 );
	else
		stop( iTuner == 0 ? 2 : 3 );
}

BOOL CDataIO::EnableTuner(int iID, BOOL bEnable)
{
	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	uint32 iTuner = iID&0x000000FF;

	status enStatus = PT::STATUS_OK;

	if( bEnable ){

		auto enable = [&](DWORD dwID) {
			Lock(dwID);
			BuffLock(dwID);
			if( SetBuff(dwID) != NULL ){
				OverFlowCount(dwID) = 0;
				WriteIndex(dwID) = 0;
			}
			Flush(Buff(dwID), TRUE);
			BuffUnLock(dwID);
			UnLock(dwID);
		};

		if( enISDB == PT::Device::ISDB_T )
			enable( iTuner == 0 ? 0 : 1 );
		else
			enable( iTuner == 0 ? 2 : 3 );

		enStatus = m_pcDevice->SetTransferTestMode(enISDB, iTuner);
		enStatus = m_pcDevice->SetTransferEnabled(enISDB, iTuner, true);
		_OutputDebugString(L"Device::SetTransferEnabled ISDB:%d tuner:%d enable:true��",enISDB,iTuner);
		if( enStatus != PT::STATUS_OK ){
			_OutputDebugString(L"��SetTransferEnabled true err");
			return FALSE;
		}

	}else{

		enStatus = m_pcDevice->SetTransferEnabled(enISDB, iTuner, false);
		_OutputDebugString(L"Device::SetTransferEnabled ISDB:%d tuner:%d enable:false��",enISDB,iTuner);
		if( enStatus != PT::STATUS_OK ){
			_OutputDebugString(L"��SetTransferEnabled false err");
//			return FALSE;
		}

		auto disable = [&](DWORD dwID) {
			Lock(dwID);
			BuffLock(dwID);
			m_fDataCarry[dwID] = false;
			OverFlowCount(dwID) = 0;
			Flush(Buff(dwID));
			BuffUnLock(dwID);
			UnLock(dwID);
		};

		if( enISDB == PT::Device::ISDB_T )
			disable( iTuner == 0 ? 0 : 1 );
		else
			disable( iTuner == 0 ? 2 : 3 );

	}
	return TRUE;
}

void CDataIO::ChkTransferInfo()
{
	BOOL err = FALSE;
	PT::Device::TransferInfo transferInfo;

	if( m_T0SetBuff != NULL ){
		ZeroMemory(&transferInfo, sizeof(PT::Device::TransferInfo));
		m_pcDevice->GetTransferInfo((PT::Device::ISDB)0, 0, &transferInfo);

		if( transferInfo.InternalFIFO_A_Overflow ||
			transferInfo.InternalFIFO_A_Underflow ||
			transferInfo.InternalFIFO_B_Overflow ||
			transferInfo.InternalFIFO_B_Underflow ||
			transferInfo.ExternalFIFO_Overflow ||
			transferInfo.Status >= 0x100
			){
				_OutputDebugString(L"��TransferInfo err : isdb:%d, tunerIndex:%d status:%d InternalFIFO_A_Overflow:%d InternalFIFO_A_Underflow:%d InternalFIFO_B_Overflow:%d InternalFIFO_B_Underflow:%d ExternalFIFO_Overflow:%d",
					0, 0,transferInfo.Status,
					transferInfo.InternalFIFO_A_Overflow,
					transferInfo.InternalFIFO_A_Underflow,
					transferInfo.InternalFIFO_B_Overflow,
					transferInfo.InternalFIFO_B_Underflow,
					transferInfo.ExternalFIFO_Overflow
					);
				err = TRUE;
		}
	}
	if( m_T1SetBuff != NULL ){
		ZeroMemory(&transferInfo, sizeof(PT::Device::TransferInfo));
		m_pcDevice->GetTransferInfo((PT::Device::ISDB)0, 1, &transferInfo);

		if( transferInfo.InternalFIFO_A_Overflow ||
			transferInfo.InternalFIFO_A_Underflow ||
			transferInfo.InternalFIFO_B_Overflow ||
			transferInfo.InternalFIFO_B_Underflow ||
			transferInfo.ExternalFIFO_Overflow ||
			transferInfo.Status >= 0x100
			){
				_OutputDebugString(L"��TransferInfo err : isdb:%d, tunerIndex:%d status:%d InternalFIFO_A_Overflow:%d InternalFIFO_A_Underflow:%d InternalFIFO_B_Overflow:%d InternalFIFO_B_Underflow:%d ExternalFIFO_Overflow:%d",
					0, 1,transferInfo.Status,
					transferInfo.InternalFIFO_A_Overflow,
					transferInfo.InternalFIFO_A_Underflow,
					transferInfo.InternalFIFO_B_Overflow,
					transferInfo.InternalFIFO_B_Underflow,
					transferInfo.ExternalFIFO_Overflow
					);
				err = TRUE;
		}
	}
	if( m_S0SetBuff != NULL ){
		ZeroMemory(&transferInfo, sizeof(PT::Device::TransferInfo));
		m_pcDevice->GetTransferInfo((PT::Device::ISDB)1, 0, &transferInfo);

		if( transferInfo.InternalFIFO_A_Overflow ||
			transferInfo.InternalFIFO_A_Underflow ||
			transferInfo.InternalFIFO_B_Overflow ||
			transferInfo.InternalFIFO_B_Underflow ||
			transferInfo.ExternalFIFO_Overflow ||
			transferInfo.Status >= 0x100
			){
				_OutputDebugString(L"��TransferInfo err : isdb:%d, tunerIndex:%d status:%d InternalFIFO_A_Overflow:%d InternalFIFO_A_Underflow:%d InternalFIFO_B_Overflow:%d InternalFIFO_B_Underflow:%d ExternalFIFO_Overflow:%d",
					1, 0,transferInfo.Status,
					transferInfo.InternalFIFO_A_Overflow,
					transferInfo.InternalFIFO_A_Underflow,
					transferInfo.InternalFIFO_B_Overflow,
					transferInfo.InternalFIFO_B_Underflow,
					transferInfo.ExternalFIFO_Overflow
					);
				err = TRUE;
		}
	}
	if( m_S1SetBuff != NULL ){
		ZeroMemory(&transferInfo, sizeof(PT::Device::TransferInfo));
		m_pcDevice->GetTransferInfo((PT::Device::ISDB)1, 1, &transferInfo);

		if( transferInfo.InternalFIFO_A_Overflow ||
			transferInfo.InternalFIFO_A_Underflow ||
			transferInfo.InternalFIFO_B_Overflow ||
			transferInfo.InternalFIFO_B_Underflow ||
			transferInfo.ExternalFIFO_Overflow ||
			transferInfo.Status >= 0x100
			){
				_OutputDebugString(L"��TransferInfo err : isdb:%d, tunerIndex:%d status:%d InternalFIFO_A_Overflow:%d InternalFIFO_A_Underflow:%d InternalFIFO_B_Overflow:%d InternalFIFO_B_Underflow:%d ExternalFIFO_Overflow:%d",
					1, 1,transferInfo.Status,
					transferInfo.InternalFIFO_A_Overflow,
					transferInfo.InternalFIFO_A_Underflow,
					transferInfo.InternalFIFO_B_Overflow,
					transferInfo.InternalFIFO_B_Underflow,
					transferInfo.ExternalFIFO_Overflow
					);
				err = TRUE;
		}
	}

	if( err ){
		for(int i=0; i<2; i++ ){
			for(int j=0; j<2; j++ ){
				int iID = (i<<8) | (j&0x000000FF);
				EnableTuner(iID, false);
			}
		}
		if( m_T0SetBuff != NULL ){
			int iID = (0<<8) | (0&0x000000FF);
			EnableTuner(iID, true);
		}
		if( m_T1SetBuff != NULL ){
			int iID = (0<<8) | (1&0x000000FF);
			EnableTuner(iID, true);
		}
		if( m_S0SetBuff != NULL ){
			int iID = (1<<8) | (0&0x000000FF);
			EnableTuner(iID, true);
		}
		if( m_S1SetBuff != NULL ){
			int iID = (1<<8) | (1&0x000000FF);
			EnableTuner(iID, true);
		}
	}
}

bool CDataIO::CheckReady(DWORD dwID)
{
	auto buffer = SetBuff(dwID) ;
	auto index = WriteIndex(dwID) ;

	status status = PT::STATUS_OK;
	uint32 nextIndex = index+1;
	if( nextIndex >= VIRTUAL_COUNT ){
		nextIndex = 0;
	}
	volatile uint8 *ptr = static_cast<uint8 *>(buffer->Ptr(nextIndex));
	status = buffer->SyncCpu(nextIndex);
	if( status != PT::STATUS_OK){
		return false;
	}

	uint8 data = ptr[0];	// ���ԓI��SyncCpu() -> �ǂݏo���̕������ϓI�Ȃ̂Łc

	if (data == 0x47) return true;
//	if (data == NOT_SYNC_BYTE) return false;

	return false;
}

bool CDataIO::ReadAddBuff(DWORD dwID)
{
	auto buffer = SetBuff(dwID) ;
	auto index = WriteIndex(dwID) ;
	auto &tsBuff = Buff(dwID) ;
	auto &OverFlow = OverFlowCount(dwID) ;

	status status = PT::STATUS_OK;
	status = buffer->SyncIo(index);
	if( status != PT::STATUS_OK){
		return false;
	}
	uint8 *ptr = static_cast<uint8 *>(buffer->Ptr(index));
	/*
	HANDLE hFile = CreateFile(L"test.ts", GENERIC_WRITE , FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	SetFilePointer(hFile, 0, 0, FILE_END);
	DWORD w;
	WriteFile(hFile, ptr, UNIT_SIZE, &w, NULL);
	CloseHandle(hFile);
	*/
	BuffLock(dwID);

	for (uint32 i = 0; i<DATA_UNIT_SIZE; i += DATA_BUFF_SIZE){
		if(tsBuff.no_pool()) { // overflow
			tsBuff.pull();
			OverFlow++ ;
			OutputDebugString(IdentStr(dwID,L" Buff Full").c_str());
		}else {
			OverFlow = 0 ;
		}
		BuffUnLock(dwID);
		auto head = tsBuff.head();
		head->resize(DATA_BUFF_SIZE);
		memcpy(head->data(), ptr+i, DATA_BUFF_SIZE);
		BuffLock(dwID);
		tsBuff.push();
	}

	m_fDataCarry[dwID] = true ;
	BuffUnLock(dwID);

	ptr[0] = NOT_SYNC_BYTE;
	status = buffer->SyncCpu(index);

	return true;
}


UINT WINAPI CDataIO::RecvThreadProc(LPVOID pParam)
{
	RECVTHREAD_PARAM *p = (RECVTHREAD_PARAM*) pParam;
	CDataIO* pSys = p->pSys ;
	DWORD dwID = p->dwID ;
	delete p;

	HANDLE hCurThread = GetCurrentThread();
	SetThreadPriority(hCurThread, THREAD_PRIORITY_HIGHEST);

	while(!pSys->m_bThTerm) {
		DWORD sleepy=10 ;
		pSys->Lock(dwID);
		if( pSys->SetBuff(dwID) != NULL ){
			if( pSys->CheckReady(dwID) ){
				if( pSys->ReadAddBuff(dwID) ){
					pSys->WriteIndex(dwID)++;
					if (pSys->VIRTUAL_COUNT <= pSys->WriteIndex(dwID)) {
						pSys->WriteIndex(dwID) = 0;
					}
					sleepy=0;
				}
			}
		}else sleepy=100 ;
		pSys->UnLock(dwID);
		if(sleepy) Sleep(sleepy);
	}

	return 0;
}

int CALLBACK CDataIO::OutsideCmdCallbackT0(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam, BOOL* pbResDataAbandon)
{
	CDataIO* pSys = (CDataIO*)pParam;
	switch( pCmdParam->dwParam ){
		case CMD_SEND_DATA:
			pSys->CmdSendData(0, pCmdParam, pResParam, pbResDataAbandon);
			break;
		default:
			pResParam->dwParam = CMD_NON_SUPPORT;
			break;
	}
	return 0;
}

int CALLBACK CDataIO::OutsideCmdCallbackT1(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam, BOOL* pbResDataAbandon)
{
	CDataIO* pSys = (CDataIO*)pParam;
	switch( pCmdParam->dwParam ){
		case CMD_SEND_DATA:
			pSys->CmdSendData(1, pCmdParam, pResParam, pbResDataAbandon);
			break;
		default:
			pResParam->dwParam = CMD_NON_SUPPORT;
			break;
	}
	return 0;
}

int CALLBACK CDataIO::OutsideCmdCallbackS0(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam, BOOL* pbResDataAbandon)
{
	CDataIO* pSys = (CDataIO*)pParam;
	switch( pCmdParam->dwParam ){
		case CMD_SEND_DATA:
			pSys->CmdSendData(2, pCmdParam, pResParam, pbResDataAbandon);
			break;
		default:
			pResParam->dwParam = CMD_NON_SUPPORT;
			break;
	}
	return 0;
}

int CALLBACK CDataIO::OutsideCmdCallbackS1(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam, BOOL* pbResDataAbandon)
{
	CDataIO* pSys = (CDataIO*)pParam;
	switch( pCmdParam->dwParam ){
		case CMD_SEND_DATA:
			pSys->CmdSendData(3, pCmdParam, pResParam, pbResDataAbandon);
			break;
		default:
			pResParam->dwParam = CMD_NON_SUPPORT;
			break;
	}
	return 0;
}

void CDataIO::CmdSendData(DWORD dwID, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam, BOOL* pbResDataAbandon)
{
	if(!m_fDataCarry[dwID]) {
		pResParam->dwParam = CMD_ERR_BUSY;
		return;
	}

	pResParam->dwParam = CMD_SUCCESS;
	BOOL bSend = FALSE;

	auto tx = [&](PTBUFFER &buf) {
		if( buf.size() > 0 ){
			auto p = buf.pull();
			pResParam->dwSize = (DWORD)p->size();
			pResParam->bData = p->data();
			*pbResDataAbandon = TRUE;
			bSend = TRUE;
		}
		if(buf.empty())
			m_fDataCarry[dwID] = false ;
	};

	BuffLock(dwID);
	tx(Buff(dwID));
	BuffUnLock(dwID);

	if( bSend == FALSE ){
		pResParam->dwParam = CMD_ERR_BUSY;
	}
}

DWORD CDataIO::GetOverFlowCount(int iID)
{
	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	uint32 iTuner = iID&0x000000FF;

	DWORD dwRet = 0;
	if( enISDB == PT::Device::ISDB_T ){
		if( iTuner == 0 ){
			dwRet = m_dwT0OverFlowCount;
		}else{
			dwRet = m_dwT1OverFlowCount;
		}
	}else{
		if( iTuner == 0 ){
			dwRet = m_dwS0OverFlowCount;
		}else{
			dwRet = m_dwS1OverFlowCount;
		}
	}
	return dwRet;
}

// MemStreaming
UINT CDataIO::MemStreamingThreadProcMain(DWORD dwID)
{
	const DWORD CmdWait = 100 ;

	while (!m_bMemStreamingTerm) {

		auto tx = [&](PTBUFFER &buf, CSharedTransportStreamer *st) -> bool {
			bool res = false ;
			if(Lock(dwID,CmdWait)) {
				if(st!=NULL) {
					while((res = !buf.empty())!=false) {
						auto p = buf.pull() ;
						auto data = p->data() ;
						auto size = p->size() ;
						UnLock(dwID);
						if(!st->Tx(data,(DWORD)size,CmdWait)) {
							buf.pull_undo();  return false;
						}
						if(m_bMemStreamingTerm||!Lock(dwID,CmdWait))
							return res;
					}
				}
				UnLock(dwID);
			}
			return res;
		};

		if(m_fDataCarry[dwID]) {
			if(!tx(Buff(dwID), MemStreamer(dwID)))
				m_fDataCarry[dwID] = false ;
		}

		if(!m_fDataCarry[dwID])
			Sleep(MemStreamer(dwID)?10:CmdWait);
	}

	return 0;
}

// MemStreaming
UINT WINAPI CDataIO::MemStreamingThreadProc(LPVOID pParam)
{
	auto p = static_cast<MEMSTREAMINGTHREAD_PARAM*>(pParam);
	CDataIO* pSys = p->pSys;
	DWORD dwID = p->dwID ;
	delete p;

	HANDLE hCurThread = GetCurrentThread();
	SetThreadPriority(hCurThread, THREAD_PRIORITY_HIGHEST);

	return pSys->MemStreamingThreadProcMain(dwID);
}

// MemStreaming
void CDataIO::StartMemStreaming(DWORD dwID)
{
	if( MemStreamingThread(0) == INVALID_HANDLE_VALUE &&
		MemStreamingThread(1) == INVALID_HANDLE_VALUE &&
		MemStreamingThread(2) == INVALID_HANDLE_VALUE &&
		MemStreamingThread(3) == INVALID_HANDLE_VALUE )
			m_bMemStreamingTerm = FALSE;

	HANDLE &hThread = MemStreamingThread(dwID) ;
	if(m_bMemStreaming&&hThread==INVALID_HANDLE_VALUE) {
		hThread = (HANDLE)_beginthreadex(NULL, 0, MemStreamingThreadProc,
			(LPVOID) new MEMSTREAMINGTHREAD_PARAM(this, dwID), CREATE_SUSPENDED, NULL);
		if(hThread != INVALID_HANDLE_VALUE) {
			SetThreadPriority( hThread, THREAD_PRIORITY_ABOVE_NORMAL );
			ResumeThread(hThread);
		}
	}
}

// MemStreaming
void CDataIO::StopMemStreaming()
{
	if( MemStreamingThread(0) != INVALID_HANDLE_VALUE ||
		MemStreamingThread(1) != INVALID_HANDLE_VALUE ||
		MemStreamingThread(2) != INVALID_HANDLE_VALUE ||
		MemStreamingThread(3) != INVALID_HANDLE_VALUE ){
		// �X���b�h�I���҂�
		HANDLE handles[4];
		DWORD cnt=0;
		for(DWORD dwID=0;dwID<4;dwID++)
			if(MemStreamingThread(dwID)!=INVALID_HANDLE_VALUE)
				handles[cnt++]=MemStreamingThread(dwID);
		m_bMemStreamingTerm=TRUE;
		if ( ::WaitForMultipleObjects(cnt,handles,TRUE, 15000) == WAIT_TIMEOUT ){
			for(DWORD i=0;i<cnt;i++)
				if(::WaitForSingleObject(handles[i],0)!=WAIT_OBJECT_0)
					::TerminateThread(handles[i], 0xffffffff);
		}
		for(DWORD i=0;i<cnt;i++)
			CloseHandle(handles[i]);
		for(DWORD dwID=0;dwID<4;dwID++)
			MemStreamingThread(dwID)=INVALID_HANDLE_VALUE;
	}
}

void CDataIO::Flush(PTBUFFER &buf, BOOL dispose)
{
	if(dispose) {
		buf.dispose();
		for(size_t i=0; i<INI_DATA_BUFF_COUNT; i++) {
			buf.head()->growup(DATA_BUFF_SIZE) ;
			buf.push();
		}
	}
	buf.clear();
}

