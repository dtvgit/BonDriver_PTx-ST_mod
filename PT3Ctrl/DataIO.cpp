#include "StdAfx.h"
#include "DataIO.h"
#include <process.h>

#define UNIT_SIZE			(4096 * 47)	// 4096��188�̍ŏ����{��
#define DATA_BUFF_SIZE		(188 * 256)	// UNIT_SIZE������؂��l�ł��鎖
#define MAX_DATA_BUFF_COUNT	500
#define NOT_SYNC_BYTE		0x74

CDataIO::CDataIO(void)
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

	m_dwT0OverFlowCount = 0;
	m_dwT1OverFlowCount = 0;
	m_dwS0OverFlowCount = 0;
	m_dwS1OverFlowCount = 0;
}

CDataIO::~CDataIO(void)
{
	Stop();

	/*if( m_hStopEvent != NULL ){
		::CloseHandle(m_hStopEvent);
		m_hStopEvent = NULL;
	}*/

	SAFE_DELETE(m_T0SetBuff);
	SAFE_DELETE(m_T1SetBuff);
	SAFE_DELETE(m_S0SetBuff);
	SAFE_DELETE(m_S1SetBuff);

	Flush(m_T0Buff);
	Flush(m_T1Buff);
	Flush(m_S0Buff);
	Flush(m_S1Buff);

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
}

void CDataIO::Lock1()
{
	if( m_hEvent1 == NULL ){
		return ;
	}
	if( WaitForSingleObject(m_hEvent1, 10*1000) == WAIT_TIMEOUT ){
		OutputDebugString(L"time out1");
	}
}

void CDataIO::UnLock1()
{
	if( m_hEvent1 != NULL ){
		SetEvent(m_hEvent1);
	}
}

void CDataIO::Lock2()
{
	if( m_hEvent2 == NULL ){
		return ;
	}
	if( WaitForSingleObject(m_hEvent2, 10*1000) == WAIT_TIMEOUT ){
		OutputDebugString(L"time out2");
	}
}

void CDataIO::UnLock2()
{
	if( m_hEvent2 != NULL ){
		SetEvent(m_hEvent2);
	}
}

void CDataIO::Lock3()
{
	if( m_hEvent3 == NULL ){
		return ;
	}
	if( WaitForSingleObject(m_hEvent3, 10*1000) == WAIT_TIMEOUT ){
		OutputDebugString(L"time out3");
	}
}

void CDataIO::UnLock3()
{
	if( m_hEvent3 != NULL ){
		SetEvent(m_hEvent3);
	}
}

void CDataIO::Lock4()
{
	if( m_hEvent4 == NULL ){
		return ;
	}
	if( WaitForSingleObject(m_hEvent4, 10*1000) == WAIT_TIMEOUT ){
		OutputDebugString(L"time out4");
	}
}

void CDataIO::UnLock4()
{
	if( m_hEvent4 != NULL ){
		SetEvent(m_hEvent4);
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
			m_dwT0OverFlowCount = 0;
			Flush(m_T0Buff);
			UnLock1();
		}else{
			Lock2();
			m_dwT1OverFlowCount = 0;
			Flush(m_T1Buff);
			UnLock2();
		}
	}else{
		if( iTuner == 0 ){
			Lock3();
			m_dwS0OverFlowCount = 0;
			Flush(m_S0Buff);
			UnLock3();
		}else{
			Lock4();
			m_dwS1OverFlowCount = 0;
			Flush(m_S1Buff);
			UnLock4();
		}
	}
}

void CDataIO::Run()
{
	if( m_hThread1 != INVALID_HANDLE_VALUE ||
		m_hThread2 != INVALID_HANDLE_VALUE ||
		m_hThread3 != INVALID_HANDLE_VALUE ||
		m_hThread4 != INVALID_HANDLE_VALUE ){
		return ;
	}
	m_bThTerm=FALSE;
	// Thread 1
	m_hThread1 = (HANDLE)_beginthreadex(NULL, 0, RecvThread1, (LPVOID)this, CREATE_SUSPENDED, NULL);
	SetThreadPriority( m_hThread1, THREAD_PRIORITY_ABOVE_NORMAL );
	ResumeThread(m_hThread1);
	// Thread 2
	m_hThread2 = (HANDLE)_beginthreadex(NULL, 0, RecvThread2, (LPVOID)this, CREATE_SUSPENDED, NULL);
	SetThreadPriority( m_hThread2, THREAD_PRIORITY_ABOVE_NORMAL );
	ResumeThread(m_hThread2);
	// Thread 3
	m_hThread2 = (HANDLE)_beginthreadex(NULL, 0, RecvThread3, (LPVOID)this, CREATE_SUSPENDED, NULL);
	SetThreadPriority( m_hThread3, THREAD_PRIORITY_ABOVE_NORMAL );
	ResumeThread(m_hThread3);
	// Thread 4
	m_hThread2 = (HANDLE)_beginthreadex(NULL, 0, RecvThread4, (LPVOID)this, CREATE_SUSPENDED, NULL);
	SetThreadPriority( m_hThread4, THREAD_PRIORITY_ABOVE_NORMAL );
	ResumeThread(m_hThread4);
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

				m_dwT0OverFlowCount = 0;
			}
			UnLock1();
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

				m_dwT1OverFlowCount = 0;
			}
			UnLock2();
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

				m_dwS0OverFlowCount = 0;
			}
			UnLock3();
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

				m_dwS1OverFlowCount = 0;
			}
			UnLock4();
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
			m_cPipeT0.StopServer();
			Lock1();
			m_dwT0OverFlowCount = 0;
			SAFE_DELETE(m_T0SetBuff);
			Flush(m_T0Buff);
			UnLock1();
		}else{
			m_cPipeT1.StopServer();
			Lock2();
			m_dwT1OverFlowCount = 0;
			SAFE_DELETE(m_T1SetBuff);
			Flush(m_T1Buff);
			UnLock2();
		}
	}else{
		if( iTuner == 0 ){
			m_cPipeS0.StopServer();
			Lock3();
			m_dwS0OverFlowCount = 0;
			SAFE_DELETE(m_S0SetBuff);
			Flush(m_S0Buff);
			UnLock3();
		}else{
			m_cPipeS1.StopServer();
			Lock4();
			m_dwS1OverFlowCount = 0;
			SAFE_DELETE(m_S1SetBuff);
			Flush(m_S1Buff);
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
				if( m_T0SetBuff != NULL ){
					m_dwT0OverFlowCount = 0;
					m_T0WriteIndex = 0;
				}
				UnLock1();
			}else{
				Lock2();
				if( m_T1SetBuff != NULL ){
					m_dwT1OverFlowCount = 0;
					m_T1WriteIndex = 0;
				}
				UnLock2();
			}
		}else{
			if( iTuner == 0 ){
				Lock3();
				if( m_S0SetBuff != NULL ){
					m_dwS0OverFlowCount = 0;
					m_S0WriteIndex = 0;
				}
				UnLock3();
			}else{
				Lock4();
				if( m_S1SetBuff != NULL ){
					m_dwS1OverFlowCount = 0;
					m_S1WriteIndex = 0;
				}
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
				m_dwT0OverFlowCount = 0;
				Flush(m_T0Buff);
				UnLock1();
			}else{
				Lock2();
				m_dwT1OverFlowCount = 0;
				Flush(m_T1Buff);
				UnLock2();
			}
		}else{
			if( iTuner == 0 ){
				Lock3();
				m_dwS0OverFlowCount = 0;
				Flush(m_S0Buff);
				UnLock3();
			}else{
				Lock4();
				m_dwS1OverFlowCount = 0;
				Flush(m_S1Buff);
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

bool CDataIO::ReadAddBuff(EARTH::EX::Buffer* buffer, uint32 index, deque<BUFF_DATA*> &tsBuff)
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
	for (uint32 i = 0; i<UNIT_SIZE; i += DATA_BUFF_SIZE){
		BUFF_DATA* pDataBuff = new BUFF_DATA(DATA_BUFF_SIZE);
		memcpy(pDataBuff->pbBuff, ptr+i, DATA_BUFF_SIZE);
		tsBuff.push_back(pDataBuff);
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
			while( pSys->CheckReady(pSys->m_T0SetBuff, pSys->m_T0WriteIndex) ){
				if( pSys->ReadAddBuff(pSys->m_T0SetBuff, pSys->m_T0WriteIndex, pSys->m_T0Buff) ){
					if( pSys->m_T0Buff.size() > MAX_DATA_BUFF_COUNT ){
						BUFF_DATA *p = pSys->m_T0Buff.front();
						pSys->m_T0Buff.pop_front();
						delete p;
						pSys->m_dwT0OverFlowCount++;
						OutputDebugString(L"T0 Buff Full");
					}else{
						pSys->m_dwT0OverFlowCount = 0;
					}

					pSys->m_T0WriteIndex++;
					if (pSys->VIRTUAL_COUNT <= pSys->m_T0WriteIndex) {
						pSys->m_T0WriteIndex = 0;
					}
				}else { sleepy=0; break; }
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
			while( pSys->CheckReady(pSys->m_T1SetBuff, pSys->m_T1WriteIndex) ){
				if( pSys->ReadAddBuff(pSys->m_T1SetBuff, pSys->m_T1WriteIndex, pSys->m_T1Buff) ){
					if( pSys->m_T1Buff.size() > MAX_DATA_BUFF_COUNT ){
						BUFF_DATA *p = pSys->m_T1Buff.front();
						pSys->m_T1Buff.pop_front();
						delete p;
						pSys->m_dwT1OverFlowCount++;
						OutputDebugString(L"T1 Buff Full");
					}else{
						pSys->m_dwT1OverFlowCount = 0;
					}

					pSys->m_T1WriteIndex++;
					if (pSys->VIRTUAL_COUNT <= pSys->m_T1WriteIndex) {
						pSys->m_T1WriteIndex = 0;
					}
				}else { sleepy=0; break; }
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
			while( pSys->CheckReady(pSys->m_S0SetBuff, pSys->m_S0WriteIndex) ){
				if( pSys->ReadAddBuff(pSys->m_S0SetBuff, pSys->m_S0WriteIndex, pSys->m_S0Buff) ){
					if( pSys->m_S0Buff.size() > MAX_DATA_BUFF_COUNT ){
						BUFF_DATA *p = pSys->m_S0Buff.front();
						pSys->m_S0Buff.pop_front();
						delete p;
						pSys->m_dwS0OverFlowCount++;
						OutputDebugString(L"S0 Buff Full");
					}else{
						pSys->m_dwS0OverFlowCount = 0;
					}

					pSys->m_S0WriteIndex++;
					if (pSys->VIRTUAL_COUNT <= pSys->m_S0WriteIndex) {
						pSys->m_S0WriteIndex = 0;
					}
				}else { sleepy=0; break; }
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
			while( pSys->CheckReady(pSys->m_S1SetBuff, pSys->m_S1WriteIndex) ){
				if( pSys->ReadAddBuff(pSys->m_S1SetBuff, pSys->m_S1WriteIndex, pSys->m_S1Buff) ){
					if( pSys->m_S1Buff.size() > MAX_DATA_BUFF_COUNT ){
						BUFF_DATA *p = pSys->m_S1Buff.front();
						pSys->m_S1Buff.pop_front();
						delete p;
						pSys->m_dwS1OverFlowCount++;
						OutputDebugString(L"S1 Buff Full");
					}else{
						pSys->m_dwS1OverFlowCount = 0;
					}

					pSys->m_S1WriteIndex++;
					if (pSys->VIRTUAL_COUNT <= pSys->m_S1WriteIndex) {
						pSys->m_S1WriteIndex = 0;
					}
				}else { sleepy=0; break; }
			}
		}else sleepy=250 ;
		pSys->UnLock4();
		if(sleepy) Sleep(sleepy);
	}

	return 0;
}

int CALLBACK CDataIO::OutsideCmdCallbackT0(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	CDataIO* pSys = (CDataIO*)pParam;
	switch( pCmdParam->dwParam ){
		case CMD_SEND_DATA:
			pSys->CmdSendData(0, pCmdParam, pResParam);
			break;
		default:
			pResParam->dwParam = CMD_NON_SUPPORT;
			break;
	}
	return 0;
}

int CALLBACK CDataIO::OutsideCmdCallbackT1(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	CDataIO* pSys = (CDataIO*)pParam;
	switch( pCmdParam->dwParam ){
		case CMD_SEND_DATA:
			pSys->CmdSendData(1, pCmdParam, pResParam);
			break;
		default:
			pResParam->dwParam = CMD_NON_SUPPORT;
			break;
	}
	return 0;
}

int CALLBACK CDataIO::OutsideCmdCallbackS0(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	CDataIO* pSys = (CDataIO*)pParam;
	switch( pCmdParam->dwParam ){
		case CMD_SEND_DATA:
			pSys->CmdSendData(2, pCmdParam, pResParam);
			break;
		default:
			pResParam->dwParam = CMD_NON_SUPPORT;
			break;
	}
	return 0;
}

int CALLBACK CDataIO::OutsideCmdCallbackS1(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	CDataIO* pSys = (CDataIO*)pParam;
	switch( pCmdParam->dwParam ){
		case CMD_SEND_DATA:
			pSys->CmdSendData(3, pCmdParam, pResParam);
			break;
		default:
			pResParam->dwParam = CMD_NON_SUPPORT;
			break;
	}
	return 0;
}

void CDataIO::CmdSendData(DWORD dwID, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam)
{
	pResParam->dwParam = CMD_SUCCESS;
	BOOL bSend = FALSE;

	switch(dwID){
		case 0:
			Lock1();
			if( m_T0Buff.size() > 0 ){
				BUFF_DATA *p = m_T0Buff.front();
				m_T0Buff.pop_front();
				pResParam->dwSize = p->dwSize;
				pResParam->bData = p->pbBuff;
				p->pbBuff = NULL;	// �|�C���^���R�s�[���Ă�̂�delete p�ō폜����Ȃ��悤�ɂ���
				delete p;
				bSend = TRUE;
			}
			UnLock1();
			break;
		case 1:
			Lock2();
			if( m_T1Buff.size() > 0 ){
				BUFF_DATA *p = m_T1Buff.front();
				m_T1Buff.pop_front();
				pResParam->dwSize = p->dwSize;
				pResParam->bData = p->pbBuff;
				p->pbBuff = NULL;	// �|�C���^���R�s�[���Ă�̂�delete p�ō폜����Ȃ��悤�ɂ���
				delete p;
				bSend = TRUE;
			}
			UnLock2();
			break;
		case 2:
			Lock3();
			if( m_S0Buff.size() > 0 ){
				BUFF_DATA *p = m_S0Buff.front();
				m_S0Buff.pop_front();
				pResParam->dwSize = p->dwSize;
				pResParam->bData = p->pbBuff;
				p->pbBuff = NULL;	// �|�C���^���R�s�[���Ă�̂�delete p�ō폜����Ȃ��悤�ɂ���
				delete p;
				bSend = TRUE;
			}
			UnLock3();
			break;
		case 3:
			Lock4();
			if( m_S1Buff.size() > 0 ){
				BUFF_DATA *p = m_S1Buff.front();
				m_S1Buff.pop_front();
				pResParam->dwSize = p->dwSize;
				pResParam->bData = p->pbBuff;
				p->pbBuff = NULL;	// �|�C���^���R�s�[���Ă�̂�delete p�ō폜����Ȃ��悤�ɂ���
				delete p;
				bSend = TRUE;
			}
			UnLock4();
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
