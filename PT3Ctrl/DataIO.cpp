#include "StdAfx.h"
#include "DataIO.h"
#include <process.h>

#define UNIT_SIZE			(4096 * 47)	// 4096��188�̍ŏ����{��
#define DATA_BUFF_SIZE		(188 * 256)	// UNIT_SIZE������؂��l�ł��鎖
#define INI_DATA_BUFF_COUNT	50
#define MAX_DATA_BUFF_COUNT	500
#define NOT_SYNC_BYTE		0x74

CDataIO::CDataIO(BOOL bMemStreaming)
 : m_T0Buff(MAX_DATA_BUFF_COUNT, 1), m_T1Buff(MAX_DATA_BUFF_COUNT, 1),
 	m_S0Buff(MAX_DATA_BUFF_COUNT, 1), m_S1Buff(MAX_DATA_BUFF_COUNT, 1)
{
	VIRTUAL_COUNT = 8*8;

	//m_hStopEvent = _CreateEvent(FALSE, FALSE, NULL);
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

	// MemStreamer
	m_bMemStreaming = bMemStreaming ;
	m_bMemStreamingTerm = TRUE ;
	m_hMemStreamingThread = INVALID_HANDLE_VALUE ;
	m_T0MemStreamer = NULL ;
	m_T1MemStreamer = NULL ;
	m_S0MemStreamer = NULL ;
	m_S1MemStreamer = NULL ;
}

CDataIO::~CDataIO(void)
{
	Stop();

	/*if( m_hStopEvent != NULL ){
		::CloseHandle(m_hStopEvent);
		m_hStopEvent = NULL;
	}*/

	// MemStreamer
	if(m_T0MemStreamer != NULL) {
		SAFE_DELETE(m_T0MemStreamer);
	}
	if(m_T1MemStreamer != NULL) {
		SAFE_DELETE(m_T1MemStreamer);
	}
	if(m_S0MemStreamer != NULL) {
		SAFE_DELETE(m_S0MemStreamer);
	}
	if(m_S1MemStreamer != NULL) {
		SAFE_DELETE(m_S1MemStreamer);
	}

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

	if( enISDB == PT::Device::ISDB_T ){
		if( iTuner == 0 ){
			Lock1();
			BuffLock1();
			m_dwT0OverFlowCount = 0;
			Flush(m_T0Buff);
			BuffUnLock1();
			UnLock1();
		}else{
			Lock2();
			BuffLock2();
			m_dwT1OverFlowCount = 0;
			Flush(m_T1Buff);
			BuffUnLock2();
			UnLock2();
		}
	}else{
		if( iTuner == 0 ){
			Lock3();
			BuffLock3();
			m_dwS0OverFlowCount = 0;
			Flush(m_S0Buff);
			BuffUnLock3();
			UnLock3();
		}else{
			Lock4();
			BuffLock4();
			m_dwS1OverFlowCount = 0;
			Flush(m_S1Buff);
			BuffUnLock4();
			UnLock4();
		}
	}
}

void CDataIO::Run(PT::Device::ISDB enISDB, uint32 iTuner)
{
	if( m_hThread1 == INVALID_HANDLE_VALUE &&
		m_hThread2 == INVALID_HANDLE_VALUE &&
		m_hThread3 == INVALID_HANDLE_VALUE &&
		m_hThread4 == INVALID_HANDLE_VALUE ) {

		m_bThTerm=FALSE;

		// MemStreamer
		if(m_bMemStreaming) {
			m_hMemStreamingThread = (HANDLE)_beginthreadex(NULL, 0, MemStreamingThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
			if(m_hMemStreamingThread != INVALID_HANDLE_VALUE) {
				m_bMemStreamingTerm = FALSE;
				SetThreadPriority( m_hMemStreamingThread, THREAD_PRIORITY_ABOVE_NORMAL );
				ResumeThread(m_hMemStreamingThread);
			}
		}
	}

	if(enISDB == PT::Device::ISDB_T) {
		// Thread 1 - T0
		if(iTuner==0 && m_hThread1==INVALID_HANDLE_VALUE) {
			m_hThread1 = (HANDLE)_beginthreadex(NULL, 0, RecvThread1, (LPVOID)this, CREATE_SUSPENDED, NULL);
			SetThreadPriority( m_hThread1, THREAD_PRIORITY_ABOVE_NORMAL );
			ResumeThread(m_hThread1);
		}
		// Thread 2 - T1
		else if(iTuner==1 && m_hThread2==INVALID_HANDLE_VALUE) {
			m_hThread2 = (HANDLE)_beginthreadex(NULL, 0, RecvThread2, (LPVOID)this, CREATE_SUSPENDED, NULL);
			SetThreadPriority( m_hThread2, THREAD_PRIORITY_ABOVE_NORMAL );
			ResumeThread(m_hThread2);
		}
	}else if(enISDB == PT::Device::ISDB_S) {
		// Thread 3 - S0
		if(iTuner==0 && m_hThread3==INVALID_HANDLE_VALUE) {
			m_hThread3 = (HANDLE)_beginthreadex(NULL, 0, RecvThread3, (LPVOID)this, CREATE_SUSPENDED, NULL);
			SetThreadPriority( m_hThread3, THREAD_PRIORITY_ABOVE_NORMAL );
			ResumeThread(m_hThread3);
		}
		// Thread 4 - S1
		else if(iTuner==1 && m_hThread4==INVALID_HANDLE_VALUE) {
			m_hThread4 = (HANDLE)_beginthreadex(NULL, 0, RecvThread4, (LPVOID)this, CREATE_SUSPENDED, NULL);
			SetThreadPriority( m_hThread4, THREAD_PRIORITY_ABOVE_NORMAL );
			ResumeThread(m_hThread4);
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

	// MemStreamer
	if( m_hMemStreamingThread != INVALID_HANDLE_VALUE) {
		// �X���b�h�I���҂�
		m_bMemStreamingTerm = TRUE ;
		if ( ::WaitForSingleObject(m_hMemStreamingThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hMemStreamingThread, 0xffffffff);
		}
		CloseHandle(m_hMemStreamingThread) ;
		m_hMemStreamingThread = INVALID_HANDLE_VALUE ;
	}
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

	// MemStreamer
	wstring strStreamerName;
	Format(strStreamerName, SHAREDMEM_TRANSPORT_STREAM_FORMAT, PT_VER, iID);

	status enStatus = m_pcDevice->SetTransferTestMode(enISDB, iTuner);
	if( enStatus != PT::STATUS_OK ){
		return ;
	}
	if( enISDB == PT::Device::ISDB_T ){
		if( iTuner == 0 ){
			Lock1();
			if( m_T0SetBuff == NULL ){
				m_T0SetBuff = new EARTH::EX::Buffer(m_pcDevice);
				m_T0SetBuff->Alloc(UNIT_SIZE, VIRTUAL_COUNT);
				m_T0WriteIndex = 0;

				uint8 *ptr = static_cast<uint8 *>(m_T0SetBuff->Ptr(0));
				for (uint32 i=0; i<VIRTUAL_COUNT; i++) {
					ptr[UNIT_SIZE*i] = NOT_SYNC_BYTE;	// �����o�C�g������ NOT_SYNC_BYTE ����������
					m_T0SetBuff->SyncCpu(i);		// CPU �L���b�V�����t���b�V��
				}

				uint64 pageAddress = m_T0SetBuff->PageDescriptorAddress();

				enStatus = m_pcDevice->SetTransferPageDescriptorAddress(enISDB, iTuner, pageAddress);

				BuffLock1();
				m_dwT0OverFlowCount = 0;
				if(m_bMemStreaming&&m_T0MemStreamer==NULL)
					m_T0MemStreamer = new CSharedTransportStreamer(
						strStreamerName, FALSE, SHAREDMEM_TRANSPORT_PACKET_SIZE,
						SHAREDMEM_TRANSPORT_PACKET_NUM);
				BuffUnLock1();
			}
			UnLock1();
			if(!m_bMemStreaming)
				m_cPipeT0.StartServer(strEvent.c_str(), strPipe.c_str(), OutsideCmdCallbackT0, this, THREAD_PRIORITY_ABOVE_NORMAL);
		}else{
			Lock2();
			if( m_T1SetBuff == NULL ){
				m_T1SetBuff = new EARTH::EX::Buffer(m_pcDevice);
				m_T1SetBuff->Alloc(UNIT_SIZE, VIRTUAL_COUNT);
				m_T1WriteIndex = 0;

				uint8 *ptr = static_cast<uint8 *>(m_T1SetBuff->Ptr(0));
				for (uint32 i=0; i<VIRTUAL_COUNT; i++) {
					ptr[UNIT_SIZE*i] = NOT_SYNC_BYTE;	// �����o�C�g������ NOT_SYNC_BYTE ����������
					m_T1SetBuff->SyncCpu(i);		// CPU �L���b�V�����t���b�V��
				}

				uint64 pageAddress = m_T1SetBuff->PageDescriptorAddress();

				enStatus = m_pcDevice->SetTransferPageDescriptorAddress(enISDB, iTuner, pageAddress);

				BuffLock2();
				m_dwT1OverFlowCount = 0;
				if(m_bMemStreaming&&m_T1MemStreamer==NULL)
					m_T1MemStreamer = new CSharedTransportStreamer(
						strStreamerName, FALSE, SHAREDMEM_TRANSPORT_PACKET_SIZE,
						SHAREDMEM_TRANSPORT_PACKET_NUM);
				BuffUnLock2();
			}
			UnLock2();
			if(!m_bMemStreaming)
				m_cPipeT1.StartServer(strEvent.c_str(), strPipe.c_str(), OutsideCmdCallbackT1, this, THREAD_PRIORITY_ABOVE_NORMAL);
		}
	}else{
		if( iTuner == 0 ){
			Lock3();
			if( m_S0SetBuff == NULL ){
				m_S0SetBuff = new EARTH::EX::Buffer(m_pcDevice);
				m_S0SetBuff->Alloc(UNIT_SIZE, VIRTUAL_COUNT);
				m_S0WriteIndex = 0;

				uint8 *ptr = static_cast<uint8 *>(m_S0SetBuff->Ptr(0));
				for (uint32 i=0; i<VIRTUAL_COUNT; i++) {
					ptr[UNIT_SIZE*i] = NOT_SYNC_BYTE;	// �����o�C�g������ NOT_SYNC_BYTE ����������
					m_S0SetBuff->SyncCpu(i);		// CPU �L���b�V�����t���b�V��
				}

				uint64 pageAddress = m_S0SetBuff->PageDescriptorAddress();

				enStatus = m_pcDevice->SetTransferPageDescriptorAddress(enISDB, iTuner, pageAddress);

				BuffLock3();
				m_dwS0OverFlowCount = 0;
				if(m_bMemStreaming&&m_S0MemStreamer==NULL)
					m_S0MemStreamer = new CSharedTransportStreamer(
						strStreamerName, FALSE, SHAREDMEM_TRANSPORT_PACKET_SIZE,
						SHAREDMEM_TRANSPORT_PACKET_NUM);
				BuffUnLock3();
			}
			UnLock3();
			if(!m_bMemStreaming)
				m_cPipeS0.StartServer(strEvent.c_str(), strPipe.c_str(), OutsideCmdCallbackS0, this, THREAD_PRIORITY_ABOVE_NORMAL);
		}else{
			Lock4();
			if( m_S1SetBuff == NULL ){
				m_S1SetBuff = new EARTH::EX::Buffer(m_pcDevice);
				m_S1SetBuff->Alloc(UNIT_SIZE, VIRTUAL_COUNT);
				m_S1WriteIndex = 0;

				uint8 *ptr = static_cast<uint8 *>(m_S1SetBuff->Ptr(0));
				for (uint32 i=0; i<VIRTUAL_COUNT; i++) {
					ptr[UNIT_SIZE*i] = NOT_SYNC_BYTE;	// �����o�C�g������ NOT_SYNC_BYTE ����������
					m_S1SetBuff->SyncCpu(i);		// CPU �L���b�V�����t���b�V��
				}

				uint64 pageAddress = m_S1SetBuff->PageDescriptorAddress();

				enStatus = m_pcDevice->SetTransferPageDescriptorAddress(enISDB, iTuner, pageAddress);

				BuffLock4();
				m_dwS1OverFlowCount = 0;
				if(m_bMemStreaming&&m_S1MemStreamer==NULL)
					m_S1MemStreamer = new CSharedTransportStreamer(
						strStreamerName, FALSE, SHAREDMEM_TRANSPORT_PACKET_SIZE,
						SHAREDMEM_TRANSPORT_PACKET_NUM);
				BuffUnLock4();
			}
			UnLock4();
			if(!m_bMemStreaming)
				m_cPipeS1.StartServer(strEvent.c_str(), strPipe.c_str(), OutsideCmdCallbackS1, this, THREAD_PRIORITY_ABOVE_NORMAL);
		}
	}
}

void CDataIO::StopPipeServer(int iID)
{
	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	uint32 iTuner = iID&0x000000FF;

	if( enISDB == PT::Device::ISDB_T ){
		if( iTuner == 0 ){
			if(!m_bMemStreaming) m_cPipeT0.StopServer();
			Lock1();
			BuffLock1();
			if(m_bMemStreaming&&m_T0MemStreamer)
				SAFE_DELETE(m_T0MemStreamer);
			m_dwT0OverFlowCount = 0;
			SAFE_DELETE(m_T0SetBuff);
			Flush(m_T0Buff);
			BuffUnLock1();
			UnLock1();
		}else{
			if(!m_bMemStreaming) m_cPipeT1.StopServer();
			Lock2();
			BuffLock2();
			if(m_bMemStreaming&&m_T1MemStreamer)
				SAFE_DELETE(m_T1MemStreamer);
			m_dwT1OverFlowCount = 0;
			SAFE_DELETE(m_T1SetBuff);
			Flush(m_T1Buff);
			BuffUnLock2();
			UnLock2();
		}
	}else{
		if( iTuner == 0 ){
			if(!m_bMemStreaming) m_cPipeS0.StopServer();
			Lock3();
			BuffLock3();
			if(m_bMemStreaming&&m_S0MemStreamer)
				SAFE_DELETE(m_S0MemStreamer);
			m_dwS0OverFlowCount = 0;
			SAFE_DELETE(m_S0SetBuff);
			Flush(m_S0Buff);
			BuffUnLock3();
			UnLock3();
		}else{
			if(!m_bMemStreaming) m_cPipeS1.StopServer();
			Lock4();
			BuffLock4();
			if(m_bMemStreaming&&m_S1MemStreamer)
				SAFE_DELETE(m_S1MemStreamer);
			m_dwS1OverFlowCount = 0;
			SAFE_DELETE(m_S1SetBuff);
			Flush(m_S1Buff);
			BuffUnLock4();
			UnLock4();
		}
	}
}

BOOL CDataIO::EnableTuner(int iID, BOOL bEnable)
{
	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	uint32 iTuner = iID&0x000000FF;

	status enStatus = PT::STATUS_OK;

	if( bEnable ){
		if( enISDB == PT::Device::ISDB_T ){
			if( iTuner == 0 ){
				Lock1();
				BuffLock1();
				if( m_T0SetBuff != NULL ){
					m_dwT0OverFlowCount = 0;
					m_T0WriteIndex = 0;
				}
				Flush(m_T0Buff, TRUE);
				BuffUnLock1();
				UnLock1();
			}else{
				Lock2();
				BuffLock2();
				if( m_T1SetBuff != NULL ){
					m_dwT1OverFlowCount = 0;
					m_T1WriteIndex = 0;
				}
				Flush(m_T1Buff, TRUE);
				BuffUnLock2();
				UnLock2();
			}
		}else{
			if( iTuner == 0 ){
				Lock3();
				BuffLock3();
				if( m_S0SetBuff != NULL ){
					m_dwS0OverFlowCount = 0;
					m_S0WriteIndex = 0;
				}
				Flush(m_S0Buff, TRUE);
				BuffUnLock3();
				UnLock3();
			}else{
				Lock4();
				BuffLock4();
				if( m_S1SetBuff != NULL ){
					m_dwS1OverFlowCount = 0;
					m_S1WriteIndex = 0;
				}
				Flush(m_S1Buff, TRUE);
				BuffUnLock4();
				UnLock4();
			}
		}
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
		if( enISDB == PT::Device::ISDB_T ){
			if( iTuner == 0 ){
				Lock1();
				BuffLock1();
				m_dwT0OverFlowCount = 0;
				Flush(m_T0Buff);
				BuffUnLock1();
				UnLock1();
			}else{
				Lock2();
				BuffLock2();
				m_dwT1OverFlowCount = 0;
				Flush(m_T1Buff);
				BuffUnLock2();
				UnLock2();
			}
		}else{
			if( iTuner == 0 ){
				Lock3();
				BuffLock3();
				m_dwS0OverFlowCount = 0;
				Flush(m_S0Buff);
				BuffUnLock3();
				UnLock3();
			}else{
				Lock4();
				BuffLock4();
				m_dwS1OverFlowCount = 0;
				Flush(m_S1Buff);
				BuffUnLock4();
				UnLock4();
			}
		}
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

bool CDataIO::CheckReady(EARTH::EX::Buffer* buffer, uint32 index)
{
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

bool CDataIO::ReadAddBuff(EARTH::EX::Buffer* buffer, uint32 index, PTBUFFER &tsBuff, DWORD dwID, DWORD &OverFlow)
{
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
	switch(dwID) {
	case 0: BuffLock1(); break;
	case 1: BuffLock2(); break;
	case 2: BuffLock3(); break;
	case 3: BuffLock4(); break;
	}

	for (uint32 i = 0; i<UNIT_SIZE; i += DATA_BUFF_SIZE){
		if(tsBuff.no_pool()) { // overflow
			tsBuff.pull();
			OverFlow++ ;
			switch(dwID) {
			case 0: OutputDebugString(L"T0 Buff Full"); break;
			case 1: OutputDebugString(L"T1 Buff Full"); break;
			case 2: OutputDebugString(L"S0 Buff Full"); break;
			case 3: OutputDebugString(L"S1 Buff Full"); break;
			}
		}else {
			OverFlow = 0 ;
		}
		auto head = tsBuff.head();
		head->resize(DATA_BUFF_SIZE);
		memcpy(head->data(), ptr+i, DATA_BUFF_SIZE);
		tsBuff.push();
	}

	switch(dwID) {
	case 0: BuffUnLock1(); break;
	case 1: BuffUnLock2(); break;
	case 2: BuffUnLock3(); break;
	case 3: BuffUnLock4(); break;
	}

	ptr[0] = NOT_SYNC_BYTE;
	status = buffer->SyncCpu(index);

	return true;
}

UINT WINAPI CDataIO::RecvThread1(LPVOID pParam)
{
	CDataIO* pSys = (CDataIO*)pParam;

	HANDLE hCurThread = GetCurrentThread();
	SetThreadPriority(hCurThread, THREAD_PRIORITY_HIGHEST);

	while(!pSys->m_bThTerm) {
		DWORD sleepy=10 ;
		pSys->Lock1();
		if( pSys->m_T0SetBuff != NULL ){
			if( pSys->CheckReady(pSys->m_T0SetBuff, pSys->m_T0WriteIndex) ){
				if( pSys->ReadAddBuff(pSys->m_T0SetBuff, pSys->m_T0WriteIndex,
						pSys->m_T0Buff, 0, pSys->m_dwT0OverFlowCount) ){
					pSys->m_T0WriteIndex++;
					if (pSys->VIRTUAL_COUNT <= pSys->m_T0WriteIndex) {
						pSys->m_T0WriteIndex = 0;
					}
					sleepy=0;
				}
			}
		}else sleepy=250 ;
		pSys->UnLock1();
		if(sleepy) Sleep(sleepy);
	}

	return 0;
}

UINT WINAPI CDataIO::RecvThread2(LPVOID pParam)
{
	CDataIO* pSys = (CDataIO*)pParam;

	HANDLE hCurThread = GetCurrentThread();
	SetThreadPriority(hCurThread, THREAD_PRIORITY_HIGHEST);

	while(!pSys->m_bThTerm) {
		DWORD sleepy=10 ;
		pSys->Lock2();
		if( pSys->m_T1SetBuff != NULL ){
			if( pSys->CheckReady(pSys->m_T1SetBuff, pSys->m_T1WriteIndex) ){
				if( pSys->ReadAddBuff(pSys->m_T1SetBuff, pSys->m_T1WriteIndex,
						pSys->m_T1Buff, 1, pSys->m_dwT1OverFlowCount) ){
					pSys->m_T1WriteIndex++;
					if (pSys->VIRTUAL_COUNT <= pSys->m_T1WriteIndex) {
						pSys->m_T1WriteIndex = 0;
					}
					sleepy=0;
				}
			}
		}else sleepy=250 ;
		pSys->UnLock2();
		if(sleepy) Sleep(sleepy);
	}

	return 0;
}

UINT WINAPI CDataIO::RecvThread3(LPVOID pParam)
{
	CDataIO* pSys = (CDataIO*)pParam;

	HANDLE hCurThread = GetCurrentThread();
	SetThreadPriority(hCurThread, THREAD_PRIORITY_HIGHEST);

	while(!pSys->m_bThTerm) {
		DWORD sleepy=10 ;
		pSys->Lock3();
		if( pSys->m_S0SetBuff != NULL ){
			if( pSys->CheckReady(pSys->m_S0SetBuff, pSys->m_S0WriteIndex) ){
				if( pSys->ReadAddBuff(pSys->m_S0SetBuff, pSys->m_S0WriteIndex,
						pSys->m_S0Buff, 2, pSys->m_dwS0OverFlowCount) ){
					pSys->m_S0WriteIndex++;
					if (pSys->VIRTUAL_COUNT <= pSys->m_S0WriteIndex) {
						pSys->m_S0WriteIndex = 0;
					}
					sleepy=0;
				}
			}
		}else sleepy=250 ;
		pSys->UnLock3();
		if(sleepy) Sleep(sleepy);
	}

	return 0;
}

UINT WINAPI CDataIO::RecvThread4(LPVOID pParam)
{
	CDataIO* pSys = (CDataIO*)pParam;

	HANDLE hCurThread = GetCurrentThread();
	SetThreadPriority(hCurThread, THREAD_PRIORITY_HIGHEST);

	while(!pSys->m_bThTerm) {
		DWORD sleepy=10 ;
		pSys->Lock4();
		if( pSys->m_S1SetBuff != NULL ){
			if( pSys->CheckReady(pSys->m_S1SetBuff, pSys->m_S1WriteIndex) ){
				if( pSys->ReadAddBuff(pSys->m_S1SetBuff, pSys->m_S1WriteIndex,
						pSys->m_S1Buff, 3, pSys->m_dwS1OverFlowCount) ){
					pSys->m_S1WriteIndex++;
					if (pSys->VIRTUAL_COUNT <= pSys->m_S1WriteIndex) {
						pSys->m_S1WriteIndex = 0;
					}
					sleepy=0;
				}
			}
		}else sleepy=250 ;
		pSys->UnLock4();
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
	pResParam->dwParam = CMD_SUCCESS;
	BOOL bSend = FALSE;

	switch(dwID){
		case 0:
			BuffLock1();
			if( m_T0Buff.size() > 0 ){
				auto p = m_T0Buff.pull();
				pResParam->dwSize = (DWORD)p->size();
				pResParam->bData = p->data();
				*pbResDataAbandon = TRUE;
				bSend = TRUE;
			}
			BuffUnLock1();
			break;
		case 1:
			BuffLock2();
			if( m_T1Buff.size() > 0 ){
				auto p = m_T1Buff.pull();
				pResParam->dwSize = (DWORD)p->size();
				pResParam->bData = p->data();
				*pbResDataAbandon = TRUE;
				bSend = TRUE;
			}
			BuffUnLock2();
			break;
		case 2:
			BuffLock3();
			if( m_S0Buff.size() > 0 ){
				auto p = m_S0Buff.pull();
				pResParam->dwSize = (DWORD)p->size();
				pResParam->bData = p->data();
				*pbResDataAbandon = TRUE;
				bSend = TRUE;
			}
			BuffUnLock3();
			break;
		case 3:
			BuffLock4();
			if( m_S1Buff.size() > 0 ){
				auto p = m_S1Buff.pull();
				pResParam->dwSize = (DWORD)p->size();
				pResParam->bData = p->data();
				*pbResDataAbandon = TRUE;
				bSend = TRUE;
			}
			BuffUnLock4();
			break;
	}

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

// MemStreamer
UINT CDataIO::MemStreamingThreadMain()
{
	const DWORD CmdWait = 50 ;

	while (!m_bMemStreamingTerm) {
		int cnt=0;

		if(BuffLock1(CmdWait)) {
			if(!m_T0Buff.empty()) {
				if(m_T0MemStreamer!=NULL) {
					auto p = m_T0Buff.pull() ;
					if(!m_T0MemStreamer->Tx(p->data(),(DWORD)p->size(),CmdWait))
						m_T0Buff.pull_undo();
					if(!m_T0Buff.empty()) cnt++;
				}
			}
			BuffUnLock1();
		}else cnt++;

		if(BuffLock2(CmdWait)) {
			if(!m_T1Buff.empty()) {
				if(m_T1MemStreamer!=NULL) {
					auto p = m_T1Buff.pull() ;
					if(!m_T1MemStreamer->Tx(p->data(),(DWORD)p->size(),CmdWait))
						m_T1Buff.pull_undo();
					if(!m_T1Buff.empty()) cnt++;
				}
			}
			BuffUnLock2();
		}else cnt++;

		if(BuffLock3(CmdWait)) {
			if(!m_S0Buff.empty()) {
				if(m_S0MemStreamer!=NULL) {
					auto p = m_S0Buff.pull() ;
					if(!m_S0MemStreamer->Tx(p->data(),(DWORD)p->size(),CmdWait))
						m_S0Buff.pull_undo();
					if(!m_S0Buff.empty()) cnt++;
				}
			}
			BuffUnLock3();
		}else cnt++;

		if(BuffLock4(CmdWait)) {
			if(!m_S1Buff.empty()) {
				if(m_S1MemStreamer!=NULL) {
					auto p = m_S1Buff.pull() ;
					if(!m_S1MemStreamer->Tx(p->data(),(DWORD)p->size(),CmdWait))
						m_S1Buff.pull_undo();
					if(!m_S1Buff.empty()) cnt++;
				}
			}
			BuffUnLock4();
		}else cnt++;

		if(!cnt) Sleep(10);
	}

	return 0;
}

// MemStreamer
UINT WINAPI CDataIO::MemStreamingThread(LPVOID pParam)
{
	CDataIO* pSys = (CDataIO*)pParam;

	HANDLE hCurThread = GetCurrentThread();
	SetThreadPriority(hCurThread, THREAD_PRIORITY_HIGHEST);

	return pSys->MemStreamingThreadMain();
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

