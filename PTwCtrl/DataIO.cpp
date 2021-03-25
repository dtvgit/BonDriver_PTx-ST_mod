#include "StdAfx.h"
#include "DataIO.h"
#include <process.h>

CDataIO::CDataIO(BOOL bMemStreaming)
  : m_T0Buff(MAX_DATA_BUFF_COUNT,1), m_T1Buff(MAX_DATA_BUFF_COUNT,1),
	m_S0Buff(MAX_DATA_BUFF_COUNT,1), m_S1Buff(MAX_DATA_BUFF_COUNT,1)
{
	VIRTUAL_COUNT = 8;

	m_hStopEvent = _CreateEvent(FALSE, FALSE, NULL);
	m_hThread = INVALID_HANDLE_VALUE;

	m_pcDevice = NULL;

	mQuit = false;

	m_hEvent1 = _CreateEvent(FALSE, TRUE, NULL );
	m_hEvent2 = _CreateEvent(FALSE, TRUE, NULL );
	m_hEvent3 = _CreateEvent(FALSE, TRUE, NULL );
	m_hEvent4 = _CreateEvent(FALSE, TRUE, NULL );

	m_dwT0OverFlowCount = 0;
	m_dwT1OverFlowCount = 0;
	m_dwS0OverFlowCount = 0;
	m_dwS1OverFlowCount = 0;

	m_bDMABuff = NULL;

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
	if( m_hThread != INVALID_HANDLE_VALUE ){
		::SetEvent(m_hStopEvent);
		// �X���b�h�I���҂�
		if ( ::WaitForSingleObject(m_hThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hThread, 0xffffffff);
		}
		CloseHandle(m_hThread);
		m_hThread = INVALID_HANDLE_VALUE;
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
	SAFE_DELETE(m_T0MemStreamer);
	SAFE_DELETE(m_T1MemStreamer);
	SAFE_DELETE(m_S0MemStreamer);
	SAFE_DELETE(m_S1MemStreamer);

	if( m_hStopEvent != NULL ){
		::CloseHandle(m_hStopEvent);
		m_hStopEvent = NULL;
	}

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

	SAFE_DELETE_ARRAY(m_bDMABuff);

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

void CDataIO::ClearBuff(int iID)
{
	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	uint iTuner = iID&0x000000FF;

	auto clear = [&](DWORD dwID) {
		Lock(dwID);
		OverFlowCount(dwID) = 0;
		Flush(Buff(dwID));
		UnLock(dwID);
	};

	if( enISDB == PT::Device::ISDB_T )
		clear(iTuner == 0 ? 0 : 1 ) ;
	else
		clear(iTuner == 0 ? 2 : 3 ) ;
}

void CDataIO::Run()
{
	if( m_hThread != INVALID_HANDLE_VALUE ){
		return ;
	}
	status enStatus;

	bool bEnalbe = true;
	enStatus = m_pcDevice->GetTransferEnable(&bEnalbe);
	if( enStatus != PT::STATUS_OK ){
		return ;
	}
	if( bEnalbe ){
		enStatus = m_pcDevice->SetTransferEnable(false);
		if( enStatus != PT::STATUS_OK ){
			return ;
		}
	}

	enStatus = m_pcDevice->SetBufferInfo(NULL);
	if( enStatus != PT::STATUS_OK ){
		return ;
	}
	PT::Device::BufferInfo bufferInfo;
	bufferInfo.VirtualSize  = VIRTUAL_IMAGE_COUNT;
	bufferInfo.VirtualCount = VIRTUAL_COUNT;
	bufferInfo.LockSize     = LOCK_SIZE;
	enStatus = m_pcDevice->SetBufferInfo(&bufferInfo);
	if( enStatus != PT::STATUS_OK ){
		return ;
	}

	if( m_bDMABuff == NULL ){
		m_bDMABuff = new BYTE[READ_BLOCK_SIZE];
	}

	// DMA �]�����ǂ��܂Ői�񂾂��𒲂ׂ邽�߁A�e�u���b�N�̖����� 0 �ŃN���A����
	for (uint i=0; i<VIRTUAL_COUNT; i++) {
		for (uint j=0; j<VIRTUAL_IMAGE_COUNT; j++) {
			for (uint k=0; k<READ_BLOCK_COUNT; k++) {
				Clear(i, j, k);
			}
		}
	}

	// �]���J�E���^�����Z�b�g����
	enStatus = m_pcDevice->ResetTransferCounter();
	if( enStatus != PT::STATUS_OK ){
		return ;
	}

	// �]���J�E���^���C���N�������g����
	for (uint i=0; i<VIRTUAL_IMAGE_COUNT*VIRTUAL_COUNT; i++) {
		enStatus = m_pcDevice->IncrementTransferCounter();
		if( enStatus != PT::STATUS_OK ){
			return ;
		}

	}

	// DMA �]����������
	enStatus = m_pcDevice->SetTransferEnable(true);
	if( enStatus != PT::STATUS_OK ){
		return ;
	}

	::ResetEvent(m_hStopEvent);
	mQuit = false;
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, RecvThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
	if(m_hThread != INVALID_HANDLE_VALUE) {
		SetThreadPriority( m_hThread, THREAD_PRIORITY_ABOVE_NORMAL );
		ResumeThread(m_hThread);
	}

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

void CDataIO::ResetDMA()
{
	status enStatus;

	bool bEnalbe = true;
	enStatus = m_pcDevice->GetTransferEnable(&bEnalbe);
	if( enStatus != PT::STATUS_OK ){
		return ;
	}
	if( bEnalbe ){
		enStatus = m_pcDevice->SetTransferEnable(false);
		if( enStatus != PT::STATUS_OK ){
			return ;
		}
	}

	mVirtualIndex=0;
	mImageIndex=0;
	mBlockIndex=0;

	// DMA �]�����ǂ��܂Ői�񂾂��𒲂ׂ邽�߁A�e�u���b�N�̖����� 0 �ŃN���A����
	for (uint i=0; i<VIRTUAL_COUNT; i++) {
		for (uint j=0; j<VIRTUAL_IMAGE_COUNT; j++) {
			for (uint k=0; k<READ_BLOCK_COUNT; k++) {
				Clear(i, j, k);
			}
		}
	}

	// �]���J�E���^�����Z�b�g����
	enStatus = m_pcDevice->ResetTransferCounter();
	if( enStatus != PT::STATUS_OK ){
		return ;
	}

	// �]���J�E���^���C���N�������g����
	for (uint i=0; i<VIRTUAL_IMAGE_COUNT*VIRTUAL_COUNT; i++) {
		enStatus = m_pcDevice->IncrementTransferCounter();
		if( enStatus != PT::STATUS_OK ){
			return ;
		}

	}

	// DMA �]����������
	enStatus = m_pcDevice->SetTransferEnable(true);
	if( enStatus != PT::STATUS_OK ){
		return ;
	}
}

void CDataIO::Stop()
{
	if( m_hThread != INVALID_HANDLE_VALUE ){
		mQuit = true;
		::SetEvent(m_hStopEvent);
		// �X���b�h�I���҂�
		if ( ::WaitForSingleObject(m_hThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hThread, 0xffffffff);
		}
		CloseHandle(m_hThread);
		m_hThread = INVALID_HANDLE_VALUE;
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

	status enStatus;

	bool bEnalbe = true;
	enStatus = m_pcDevice->GetTransferEnable(&bEnalbe);
	if( enStatus != PT::STATUS_OK ){
		return ;
	}
	if( bEnalbe ){
		enStatus = m_pcDevice->SetTransferEnable(false);
		if( enStatus != PT::STATUS_OK ){
			return ;
		}
	}
}

void CDataIO::EnableTuner(int iID, BOOL bEnable)
{
	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	uint iTuner = iID&0x000000FF;

	wstring strPipe = L"";
	wstring strEvent = L"";
	Format(strPipe, L"%s%d", CMD_PT1_DATA_PIPE, iID );
	Format(strEvent, L"%s%d", CMD_PT1_DATA_EVENT_WAIT_CONNECT, iID );

	// MemStreamer
	wstring strStreamerName;
	Format(strStreamerName, SHAREDMEM_TRANSPORT_STREAM_FORMAT, PT_VER, iID);

	if( bEnable ){

		auto enable = [&](DWORD dwID) {
			Lock(dwID);
			OverFlowCount(dwID) = 0;
			Flush(Buff(dwID), TRUE);
			auto &st = MemStreamer(dwID) ;
			if(m_bMemStreaming&&st==NULL)
				st = new CSharedTransportStreamer(
					strStreamerName, FALSE, SHAREDMEM_TRANSPORT_PACKET_SIZE,
					SHAREDMEM_TRANSPORT_PACKET_NUM);
			UnLock(dwID);
			if(!m_bMemStreaming)
				Pipe(dwID).StartServer(strEvent.c_str(), strPipe.c_str(),
					OutsideCmdCallback(dwID), this, THREAD_PRIORITY_ABOVE_NORMAL);
		};

		if( enISDB == PT::Device::ISDB_T )
			enable(iTuner == 0 ? 0 : 1 ) ;
		else
			enable(iTuner == 0 ? 2 : 3 ) ;

	}else{ // Disable

		auto disable = [&](DWORD dwID) {
			if(!m_bMemStreaming) Pipe(dwID).StopServer();
			Lock(dwID);
			auto &st = MemStreamer(dwID);
			SAFE_DELETE(st);
			OverFlowCount(dwID) = 0;
			Flush(Buff(dwID));
			UnLock(dwID);
		};

		if( enISDB == PT::Device::ISDB_T )
			disable(iTuner == 0 ? 0 : 1 ) ;
		else
			disable(iTuner == 0 ? 2 : 3 ) ;

	}
}

UINT WINAPI CDataIO::RecvThread(LPVOID pParam)
{
	CDataIO* pSys = (CDataIO*)pParam;

	HANDLE hCurThread = GetCurrentThread();
	SetThreadPriority(hCurThread, THREAD_PRIORITY_HIGHEST);

	pSys->mVirtualIndex = 0;
	pSys->mImageIndex = 0;
	pSys->mBlockIndex = 0;

	while (true) {
		DWORD dwRes = WaitForSingleObject(pSys->m_hStopEvent, 0);
		if( dwRes == WAIT_OBJECT_0 ){
			//STOP
			break;
		}

		bool b;

		b = pSys->WaitBlock();
		if (b == false) break;

		pSys->CopyBlock();

		b = pSys->DispatchBlock();
		if (b == false) break;
	}

	return 0;
}

// 1�u���b�N�� DMA �]�����I��邩 mQuit �� true �ɂȂ�܂ő҂�
bool CDataIO::WaitBlock()
{
	bool b = true;

	while (true) {
		if (mQuit) {
			b = false;
			break;
		}

		// �u���b�N�̖����� 0 �łȂ���΁A���̃u���b�N�� DMA �]���������������ƂɂȂ�
		if (Read(mVirtualIndex, mImageIndex, mBlockIndex) != 0) break;
		Sleep(3);
	}
	//::wprintf(L"(mVirtualIndex, mImageIndex, mBlockIndex) = (%d, %d, %d) �̓]�����I���܂����B\n", mVirtualIndex, mImageIndex, mBlockIndex);

	return b;
}

// 1�u���b�N���̃f�[�^���e���|�����̈�ɃR�s�[����BCPU �����猩�� DMA �o�b�t�@�̓L���b�V���������Ȃ����߁A
// �L���b�V���������������̈�ɃR�s�[���Ă���A�N�Z�X����ƌ��������܂�܂��B
void CDataIO::CopyBlock()
{
	status enStatus;

	void *voidPtr;
	enStatus = m_pcDevice->GetBufferPtr(mVirtualIndex, &voidPtr);
	if( enStatus != PT::STATUS_OK ){
		return ;
	}
	DWORD dwOffset = ((TRANSFER_SIZE*mImageIndex) + (READ_BLOCK_SIZE*mBlockIndex));

	memcpy(m_bDMABuff, (BYTE*)voidPtr+dwOffset, READ_BLOCK_SIZE);

	// �R�s�[���I������̂ŁA�u���b�N�̖����� 0 �ɂ��܂��B
	uint *ptr = static_cast<uint *>(voidPtr);
	ptr[Offset(mImageIndex, mBlockIndex, READ_BLOCK_SIZE)-1] = 0;

	if (READ_BLOCK_COUNT <= ++mBlockIndex) {
		mBlockIndex = 0;

		// �]���J�E���^�� OS::Memory::PAGE_SIZE * PT::Device::BUFFER_PAGE_COUNT �o�C�g���ƂɃC���N�������g���܂��B
		enStatus = m_pcDevice->IncrementTransferCounter();
		if( enStatus != PT::STATUS_OK ){
			return ;
		}

		if (VIRTUAL_IMAGE_COUNT <= ++mImageIndex) {
			mImageIndex = 0;
			if (VIRTUAL_COUNT <= ++mVirtualIndex) {
				mVirtualIndex = 0;
			}
		}
	}
}

void CDataIO::Clear(uint virtualIndex, uint imageIndex, uint blockIndex)
{
	void *voidPtr;
	status enStatus = m_pcDevice->GetBufferPtr(virtualIndex, &voidPtr);
	if( enStatus != PT::STATUS_OK ){
		return ;
	}

	uint *ptr = static_cast<uint *>(voidPtr);
	ptr[Offset(imageIndex, blockIndex, READ_BLOCK_SIZE)-1] = 0;
}

uint CDataIO::Read(uint virtualIndex, uint imageIndex, uint blockIndex) const
{
	void *voidPtr;
	status enStatus = m_pcDevice->GetBufferPtr(virtualIndex, &voidPtr);
	if( enStatus != PT::STATUS_OK ){
		return 0;
	}

	volatile const uint *ptr = static_cast<volatile const uint *>(voidPtr);
	return ptr[Offset(imageIndex, blockIndex, READ_BLOCK_SIZE)-1];
}

uint CDataIO::Offset(uint imageIndex, uint blockIndex, uint additionalOffset) const
{
	uint offset = ((TRANSFER_SIZE*imageIndex) + (READ_BLOCK_SIZE*blockIndex) + additionalOffset) / sizeof(uint);

	return offset;
}

bool CDataIO::DispatchBlock()
{
	const uint *ptr = (uint*)m_bDMABuff;

	for (uint i=0; i<READ_BLOCK_SIZE; i+=4) {
		uint packetError = BIT_SHIFT_MASK(m_bDMABuff[i+3], 0, 1);

		if (packetError) {
			// �G���[�̌����𒲂ׂ�
			PT::Device::TransferInfo info;
			status enStatus = m_pcDevice->GetTransferInfo(&info);
			if( enStatus == PT::STATUS_OK ){
				if (info.TransferCounter0) {
					OutputDebugString(L"���]���J�E���^�� 0 �ł���̂����o�����B");
					ResetDMA();
					break;
				} else if (info.TransferCounter1) {
					OutputDebugString(L"���]���J�E���^�� 1 �ȉ��ɂȂ�܂����B");
					ResetDMA();
					break;
				} else if (info.BufferOverflow) {
					OutputDebugString(L"��PCI �o�X�𒷊��ɓn��m�ۂł��Ȃ��������߁A�{�[�h��� FIFO �����܂����B");
					ResetDMA();
					break;
				} else {
					OutputDebugString(L"���]���G���[���������܂����B");
					break;
				}
			}else{
				OutputDebugString(L"GetTransferInfo() err");
				break;
			}
		}else{
			MicroPacket(m_bDMABuff+i);
		}
	}

	return true;
}

void CDataIO::MicroPacket(BYTE* pbPacket)
{
	uint packetId      = BIT_SHIFT_MASK(pbPacket[3], 5,  3);

	auto init_head = [](PTBUFFER_OBJECT *head) {
		if(head->size()>=head->capacity()) {
			head->resize(0);
			head->growup(DATA_BUFF_SIZE);
		}
	};

	BOOL bCreate1TS = FALSE;

	DWORD dwID;
	switch(packetId){
	case 2: dwID=0; break;
	case 4: dwID=1; break;
	case 1: dwID=2; break;
	case 3: dwID=3; break;
	default: return;
	}

	auto &micro = Micro(dwID);
	auto &buf = Buff(dwID);
	auto &overflow = OverFlowCount(dwID);

	bCreate1TS = micro.MicroPacket(pbPacket);
	if( bCreate1TS && buf.head() != NULL){
		Lock(dwID);
		auto head = buf.head(); init_head(head);
		auto sz = head->size() ;
		head->resize(sz+188);
		memcpy(head->data()+sz, micro.Get1TS(), 188);
		if( head->size() >= head->capacity() ){
			buf.push();
			if(buf.no_pool()) { // overflow
				buf.pull();
				overflow++;
				OutputDebugString(IdentStr(dwID,L" Buff Full").c_str());
			}else{
				overflow ^= overflow;
			}
		}
		UnLock(dwID);
	}
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

	if(Lock(dwID)) {
		auto &buf = Buff(dwID);
		if( buf.size() > 0 ){
			auto p = buf.pull();
			pResParam->dwSize = (DWORD)p->size();
			pResParam->bData = p->data();
			*pbResDataAbandon = TRUE;
			bSend = TRUE;
		}
		UnLock(dwID);
	}

	if( bSend == FALSE ){
		pResParam->dwParam = CMD_ERR_BUSY;
	}
}

DWORD CDataIO::GetOverFlowCount(int iID)
{
	int iDevID = iID>>16;
	PT::Device::ISDB enISDB = (PT::Device::ISDB)((iID&0x0000FF00)>>8);
	uint iTuner = iID&0x000000FF;

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

		auto tx = [&](PTBUFFER &buf, CSharedTransportStreamer *st) {
			if(!buf.empty()) {
				if(st!=NULL) {
					auto p = buf.pull() ;
					if(!st->Tx(p->data(),(DWORD)p->size(),CmdWait))
						buf.pull_undo();
					if(!buf.empty()) cnt++;
				}
			}
		};

		for(DWORD dwID=0; dwID<4; dwID++) {
			if(Lock(dwID,CmdWait)) {
				tx(Buff(dwID), MemStreamer(dwID)) ;
				UnLock(dwID);
			}else cnt++;
		}

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
			buf.head()->growup(DATA_BUFF_SIZE);
			buf.push();
		}
	}
	buf.clear();
}

