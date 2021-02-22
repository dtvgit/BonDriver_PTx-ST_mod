#include "stdafx.h"
#include "PTSendCtrlCmdUtil.h"

DWORD WaitConnect(LPCWSTR lpwszName, DWORD dwConnectTimeOut)
{
	HANDLE hEvent = _CreateEvent(FALSE, FALSE, lpwszName);
	if( hEvent == NULL ){
		return CMD_ERR;
	}
	DWORD dwRet = CMD_SUCCESS;
	if(WaitForSingleObject(hEvent, dwConnectTimeOut) != WAIT_OBJECT_0){
		dwRet = CMD_ERR_TIMEOUT;
	}
	CloseHandle(hEvent);

	return dwRet;
}

DWORD SendDefCmd(LPCWSTR lpwszEventName, LPCWSTR lpwszPipeName, DWORD dwConnectTimeOut, CMD_STREAM* pstSend, CMD_STREAM* pstRes)
{
	DWORD dwRet = CMD_SUCCESS;
	if( pstSend == NULL || pstRes == NULL ){
		return CMD_ERR_INVALID_ARG;
	}

	dwRet = WaitConnect(lpwszEventName, dwConnectTimeOut);
	if( dwRet != CMD_SUCCESS ){
		return dwRet;
	}
	HANDLE hPipe = _CreateFile(lpwszPipeName, GENERIC_READ|GENERIC_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if( hPipe == INVALID_HANDLE_VALUE ){
		return CMD_ERR_CONNECT;
	}
	DWORD dwWrite = 0;
	DWORD dwRead = 0;

	if( WriteFile(hPipe, pstSend, sizeof(DWORD)*2, &dwWrite, NULL ) == FALSE ){
		CloseHandle(hPipe);
		return CMD_ERR;
	}
	if( pstSend->dwSize > 0 ){
		if( pstSend->bData == NULL ){
			CloseHandle(hPipe);
			return CMD_ERR_INVALID_ARG;
		}
		DWORD dwSendNum = 0;
		while(dwSendNum < pstSend->dwSize ){
			DWORD dwSendSize;
			if( pstSend->dwSize-dwSendNum < SEND_BUFF_SIZE ){
				dwSendSize = pstSend->dwSize-dwSendNum;
			}else{
				dwSendSize = SEND_BUFF_SIZE;
			}
			if( WriteFile(hPipe, pstSend->bData+dwSendNum, dwSendSize, &dwWrite, NULL ) == FALSE ){
				CloseHandle(hPipe);
				return CMD_ERR;
			}
			dwSendNum+=dwWrite;
		}
	}
	if( ReadFile(hPipe, pstRes, sizeof(DWORD)*2, &dwRead, NULL ) == FALSE ){
		return CMD_ERR;
	}
	if( pstRes->dwSize > 0 ){
		pstRes->bData = new BYTE[pstRes->dwSize];
		DWORD dwReadNum = 0;
		while(dwReadNum < pstRes->dwSize ){
			DWORD dwReadSize;
			if( pstRes->dwSize-dwReadNum < RES_BUFF_SIZE ){
				dwReadSize = pstRes->dwSize-dwReadNum;
			}else{
				dwReadSize = RES_BUFF_SIZE;
			}
			if( ReadFile(hPipe, pstRes->bData+dwReadNum, dwReadSize, &dwRead, NULL ) == FALSE ){
				CloseHandle(hPipe);
				return CMD_ERR;
			}
			dwReadNum+=dwRead;
		}
	}
	CloseHandle(hPipe);

	return pstRes->dwParam;
}

DWORD SendCloseExe(DWORD dwConnectTimeOut)
{
	CMD_STREAM stSend;
	CMD_STREAM stRes;

	stSend.dwParam = CMD_CLOSE_EXE;

	DWORD dwRet = SendDefCmd(CMD_PT1_CTRL_EVENT_WAIT_CONNECT, CMD_PT1_CTRL_PIPE, dwConnectTimeOut, &stSend, &stRes);

	return dwRet;
}

DWORD SendGetTotalTunerCount(DWORD* pdwNumTuner, DWORD dwConnectTimeOut)
{
	CMD_STREAM stSend;
	CMD_STREAM stRes;

	stSend.dwParam = CMD_GET_TOTAL_TUNER_COUNT;

	DWORD dwRet = SendDefCmd(CMD_PT1_CTRL_EVENT_WAIT_CONNECT, CMD_PT1_CTRL_PIPE, dwConnectTimeOut, &stSend, &stRes);
	if( dwRet == CMD_SUCCESS ){
		CopyDefData(pdwNumTuner, stRes.bData);
	}

	return dwRet;
}

DWORD SendGetActiveTunerCount(BOOL bSate, DWORD* pdwNumTuner, DWORD dwConnectTimeOut)
{
	CMD_STREAM stSend;
	CMD_STREAM stRes;

	stSend.dwParam = CMD_GET_ACTIVE_TUNER_COUNT;
	CreateDefStream(bSate, &stSend);

	DWORD dwRet = SendDefCmd(CMD_PT1_CTRL_EVENT_WAIT_CONNECT, CMD_PT1_CTRL_PIPE, dwConnectTimeOut, &stSend, &stRes);
	if( dwRet == CMD_SUCCESS ){
		CopyDefData(pdwNumTuner, stRes.bData);
	}

	return dwRet;
}

DWORD SendSetLnbPower(int iID, BOOL bEnabled, DWORD dwConnectTimeOut)
{
	CMD_STREAM stSend;
	CMD_STREAM stRes;

	stSend.dwParam = CMD_SET_LNB_POWER;
	CreateDefStream2(iID, bEnabled, &stSend);

	DWORD dwRet = SendDefCmd(CMD_PT1_CTRL_EVENT_WAIT_CONNECT, CMD_PT1_CTRL_PIPE, dwConnectTimeOut, &stSend, &stRes);

	return dwRet;
}

DWORD SendOpenTuner(BOOL bSate, int* piID, DWORD dwConnectTimeOut)
{
	CMD_STREAM stSend;
	CMD_STREAM stRes;

	stSend.dwParam = CMD_OPEN_TUNER;
	CreateDefStream(bSate, &stSend);

	DWORD dwRet = SendDefCmd(CMD_PT1_CTRL_EVENT_WAIT_CONNECT, CMD_PT1_CTRL_PIPE, dwConnectTimeOut, &stSend, &stRes);
	if( dwRet == CMD_SUCCESS ){
		CopyDefData((DWORD*)piID, stRes.bData);
	}

	return dwRet;
}

DWORD SendOpenTuner2(BOOL bSate, int iTunerID, int* piID, DWORD dwConnectTimeOut)
{
	CMD_STREAM stSend;
	CMD_STREAM stRes;

	stSend.dwParam = CMD_OPEN_TUNER2;
	CreateDefStream2(bSate, iTunerID, &stSend);

	DWORD dwRet = SendDefCmd(CMD_PT1_CTRL_EVENT_WAIT_CONNECT, CMD_PT1_CTRL_PIPE, dwConnectTimeOut, &stSend, &stRes);
	if( dwRet == CMD_SUCCESS ){
		CopyDefData((DWORD*)piID, stRes.bData);
	}

	return dwRet;
}

DWORD SendCloseTuner(int iID, DWORD dwConnectTimeOut)
{
	CMD_STREAM stSend;
	CMD_STREAM stRes;

	stSend.dwParam = CMD_CLOSE_TUNER;
	CreateDefStream(iID, &stSend);

	DWORD dwRet = SendDefCmd(CMD_PT1_CTRL_EVENT_WAIT_CONNECT, CMD_PT1_CTRL_PIPE, dwConnectTimeOut, &stSend, &stRes);

	return dwRet;
}

DWORD SendSetCh(int iID, DWORD dwCh, DWORD dwTSID, DWORD dwConnectTimeOut)
{
	CMD_STREAM stSend;
	CMD_STREAM stRes;

	stSend.dwParam = CMD_SET_CH;
	CreateDefStream3(iID, dwCh, dwTSID, &stSend);

	DWORD dwRet = SendDefCmd(CMD_PT1_CTRL_EVENT_WAIT_CONNECT, CMD_PT1_CTRL_PIPE, dwConnectTimeOut, &stSend, &stRes);

	return dwRet;
}

DWORD SendGetSignal(int iID, DWORD* pdwCn100, DWORD dwConnectTimeOut)
{
	CMD_STREAM stSend;
	CMD_STREAM stRes;

	stSend.dwParam = CMD_GET_SIGNAL;
	CreateDefStream(iID, &stSend);

	DWORD dwRet = SendDefCmd(CMD_PT1_CTRL_EVENT_WAIT_CONNECT, CMD_PT1_CTRL_PIPE, dwConnectTimeOut, &stSend, &stRes);
	if( dwRet == CMD_SUCCESS ){
		CopyDefData(pdwCn100, stRes.bData);
	}

	return dwRet;
}

DWORD SendSendData(int iID, BYTE** pbData, DWORD* pdwSize, wstring strEvent, wstring strPipe, DWORD dwConnectTimeOut )
{
	CMD_STREAM stSend;
	CMD_STREAM stRes;

	stSend.dwParam = CMD_SEND_DATA;

	DWORD dwRet = SendDefCmd(strEvent.c_str(), strPipe.c_str(), dwConnectTimeOut, &stSend, &stRes);
	if( dwRet == CMD_SUCCESS ){
		*pbData = stRes.bData;
		*pdwSize = stRes.dwSize;
		stRes.bData = NULL;		// �|�C���^�ŕԂ��̂�stRes�̃f�X�g���N�^��delete����Ȃ��悤��
	}

	return dwRet;
}
