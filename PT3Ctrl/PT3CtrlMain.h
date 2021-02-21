#pragma once

#include "../Common/PT1SendCtrlCmdUtil.h"
#include "../Common/PipeServer.h"
#include "PT3Manager.h"

class CPT3CtrlMain
{
public:
	CPT3CtrlMain(void);
	~CPT3CtrlMain(void);

	void StartMain(BOOL bService);
	void StopMain();

	BOOL IsFindOpen();

protected:
	HANDLE m_hStopEvent;
	CPT3Manager m_cPT3;

	BOOL m_bService;

protected:
	static int CALLBACK OutsideCmdCallback(void* pParam, CMD_STREAM* pCmdParam, CMD_STREAM* pResParam);

	//CMD_CLOSE_EXE PT1Ctrl.exe�̋����I���R�}���h �ʏ�͎g�p���Ȃ�
	void CmdCloseExe(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam);
	//CMD_GET_TOTAL_TUNER_COUNT GetTotalTunerCount
	void CmdGetTotalTunerCount(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam);
	//CMD_GET_ACTIVE_TUNER_COUNT GetActiveTunerCount
	void CmdGetActiveTunerCount(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam);
	//CMD_SET_LNB_POWER SetLnbPower
	void CmdSetLnbPower(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam);
	//CMD_OPEN_TUNER OpenTuner
	void CmdOpenTuner(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam);
	//CMD_CLOSE_TUNER CloseTuner
	void CmdCloseTuner(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam);
	//CMD_SET_CH SetChannel
	void CmdSetCh(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam);
	//CMD_GET_SIGNAL GetSignalLevel
	void CmdGetSignal(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam);
	//CMD_OPEN_TUNER2 OpenTuner2
	void CmdOpenTuner2(CMD_STREAM* pCmdParam, CMD_STREAM* pResParam);
};
