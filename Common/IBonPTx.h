// IBonPTx.h: IBonPTx �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#if !defined(_IBONPTX_H_)
#define _IBONPTX_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// �}PTx�C���^�[�t�F�C�X (PTx�ŗL�̋@�\���܂Ƃ߂�����)
class IBonPTx
{
public:
	// �e�g�����X�|���_�ɑΉ�����PTx�ŗL�̃`�����l���ԍ���Ԃ�
	virtual const DWORD TransponderGetPTxCh(const DWORD dwSpace, const DWORD dwTransponder) = 0;
	// �e�`�����l���ɑΉ�����PTx�ŗL�̃`�����l������Ԃ�
	virtual const DWORD GetPTxCh(const DWORD dwSpace, const DWORD dwChannel, DWORD *lpdwTSID=NULL) = 0;
};

#endif // !defined(_IBONPTX_H_)
