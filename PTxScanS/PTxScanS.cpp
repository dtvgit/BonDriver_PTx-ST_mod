// PTxScanS.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include "stdafx.h"
#include <algorithm>
#include <vector>
#include <string>
#include <memory>
#include <time.h>
#include "../Common/IBonTransponder.h"
#include "../Common/IBonPTx.h"

using namespace std;

// ���g�������ɔ�₷�ő厞��(ms)
DWORD MAXDUR_FREQ=1000;
// TMCC�擾�ɔ�₷�ő厞��(ms)
DWORD MAXDUR_TMCC=1500;

using str_vector = vector<string> ;
using tsid_vector = vector<DWORD> ;
typedef IBonDriver *(*CREATEBONDRIVER)() ;

struct TRANSPONDER {
	string Name ;
	tsid_vector TSIDs;
	TRANSPONDER(string name="") : Name(name) {}
};
using TRANSPONDERS = vector<TRANSPONDER> ;

string AppPath = "" ;
string BonFile = "" ;

HMODULE BonModule = NULL;
IBonDriver2 *BonTuner = NULL;
IBonTransponder *BonTransponder = NULL;
IBonPTx *BonPTx = NULL;

string wcs2mbcs(wstring src, UINT code_page=CP_ACP)
{
  int mbLen = WideCharToMultiByte(code_page, 0, src.c_str(), (int)src.length(), NULL, 0, NULL, NULL) ;
  if(mbLen>0) {
	char* mbcs = (char*) alloca(mbLen) ;
	WideCharToMultiByte(code_page, 0, src.c_str(), (int)src.length(), mbcs, mbLen, NULL, NULL);
	return string(mbcs,mbLen);
  }
  return string("");
}

int EnumCandidates(str_vector &candidates, string filter)
{
	int n = 0 ;
	WIN32_FIND_DATAA Data ;
	HANDLE HFind = FindFirstFileA((AppPath+filter).c_str(),&Data) ;
	if(HFind!=INVALID_HANDLE_VALUE) {
		do {
			if(Data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) continue ;
			candidates.push_back(Data.cFileName) ;
			n++;
		}while(FindNextFileA(HFind,&Data)) ;
		FindClose(HFind) ;
	}
	return n;
}

string DoSelectS()
{
	str_vector candidates;
	EnumCandidates(candidates,"BonDriver_PTx-S*.dll");
	EnumCandidates(candidates,"BonDriver_PT-S*.dll");
	EnumCandidates(candidates,"BonDriver_PT3-S*.dll");
	EnumCandidates(candidates,"BonDriver_PTw-S*.dll");
	int choice=-1;
	for(size_t i=0;i<candidates.size();i+=10) {
		size_t n=min(candidates.size(),i+10) ;
		for(;;) {
			for(size_t j=i;j<n;j++) {
				printf("  %d) %s\n",int(j-i),candidates[j].c_str());
			}
			if(n<candidates.size())
				printf("�T�e���C�g�X�L�����������I��[0�`%d][q=���f][n=����]> ",int(n-i-1));
			else
				printf("�T�e���C�g�X�L�����������I��[0�`%d][q=���f]> ",int(n-i-1));
			char c = tolower(getchar());
			while(c<0x20) c = tolower(getchar());
			putchar('\n');
			if(n<candidates.size()&&c=='n') break ;
			else if(c=='q') {
				puts("���f���܂����B\n");
				return "";
			}
			else if(c>='0'&&c<'0'+char(n-i)) {
				choice = c-'0' + char(i);
				break;
			}else {
				puts("���͂����l������������܂���B������x��蒼���Ă��������B\n");
			}
		}
		if(choice>=0) break;
	}
	if(choice>=0)
		return candidates[choice] ;
	return "";
}

bool LoadBon()
{
	printf("�}�h���C�o \"%s\" �����[�h���Ă��܂�...\n\n",BonFile.c_str());
	string path = AppPath + BonFile ;
	BonModule = LoadLibraryA(path.c_str()) ;
	if(BonModule==NULL) {
		printf("�`���[�i�[�t�@�C�� \"%s\" �̃��[�h�Ɏ��s�B\n\n",path.c_str());
		return false ;
	}
	CREATEBONDRIVER CreateBonDriver_
	  = (CREATEBONDRIVER) GetProcAddress(BonModule,"CreateBonDriver") ;
	if(!CreateBonDriver_) {
		printf("�}�h���C�o \"%s\" �̃C���^���X�����Ɏ��s�B\n\n",BonFile.c_str());
		return false ;
	}
	try {
		BonTuner = dynamic_cast<IBonDriver2*>(CreateBonDriver_()) ;
	}catch(bad_cast &e) {
		printf("Cast error: %s\n",e.what());
		puts("IBonDriver2�C���^�[�t�F�C�X���m�F�ł��܂���ł����B\n");
		return false;
	}
	printf("�}�h���C�o \"%s\" �̃C���^���X�����ɐ����B\n\n",BonFile.c_str());
	return true;
}

void FreeBon()
{
	if(BonTuner!=NULL) { BonTuner->Release(); BonTuner=NULL; }
	if(BonModule!=NULL) { FreeLibrary(BonModule); BonModule=NULL; }
}

bool SelectTransponder(DWORD spc, DWORD tp)
{
	BOOL res;
	if(!(res=BonTransponder->TransponderSelect(spc,tp))) {
	  Sleep(MAXDUR_FREQ);
	  res = BonTransponder->TransponderSelect(spc,tp) ;
	}
	if(res)
		BonTuner->PurgeTsStream();
	return res ? true:false ;
}

bool MakeIDList(tsid_vector &idList)
{
	DWORD num_ids=0;
	if(!BonTransponder->TransponderGetIDList(NULL, &num_ids)||!num_ids)
		return false;

	DWORD *ids = (DWORD*) alloca(sizeof(DWORD)*num_ids);

	Sleep(MAXDUR_TMCC);

	if(!BonTransponder->TransponderGetIDList(ids, &num_ids))
		return false;

	for(DWORD i=0;i<num_ids;i++) {
		DWORD id = ids[i]&0xFFFF ;
		if(id==0||id==0xFFFF) continue;
		idList.push_back(id);
	}

	//�O�ׁ̈A�\�[�g���Ă���
	stable_sort(idList.begin(), idList.end(),
		[](DWORD lhs, DWORD rhs) -> bool {
			return (lhs&7) < (rhs&7) ;
		}
	);

	return true ;
}

bool OutChSet(string out_file, const str_vector &spaces, const vector<TRANSPONDERS> &transponders_list, bool out_transponders)
{
	string filename = AppPath + out_file ;
	FILE *st=NULL;
	fopen_s(&st,filename.c_str(),"wt");
	if(!st) return false ;
	fputs("; -*- This file was generated automatically by PTxScanS -*-\n",st);
	time_t now = time(NULL) ;
	char asct_buff[80];
	tm now_tm; localtime_s(&now_tm,&now);
	asctime_s(asct_buff,80,&now_tm);
	fputs(";\t@ ",st); fputs(asct_buff,st);
	fputs(";BS/CS�p\n",st);
	fputs(";�`���[�i�[���(�^�u��؂�F$����\tBonDriver�Ƃ��Ẵ`���[�i���\n",st);
	for(size_t spc=0;spc<spaces.size();spc++)
		fprintf(st,"$%s\t%d\n",spaces[spc].c_str(),(int)spc);
	if(out_transponders) {
		fputs(";BS/CS110�g�����X�|���_���[mod5]\n",st);
		fputs(
			";�g�����X�|���_(�^�u��؂�F%����\t"
			"BonDriver�Ƃ��Ẵ`���[�i���\t"
			"BonDriver�Ƃ��Ẵ`�����l��\t"
			"PTx�Ƃ��Ẵ`�����l��)\n",st);
		for(size_t spc=0;spc<transponders_list.size();spc++) {
			const TRANSPONDERS &trapons = transponders_list[spc];
			for(size_t tp=0;tp<trapons.size();tp++) {
				fputc('%',st) ;
				const TRANSPONDER &trpn = trapons[tp] ;
				DWORD ptxch = BonPTx->TransponderGetPTxCh((DWORD)spc,(DWORD)tp);
				fprintf(st,"%s\t%d\t%d\t%d\n",trpn.Name.c_str(),(int)spc,(int)tp,(int)ptxch);
			}
		}
	}
	fputs(
		";�`�����l��(�^�u��؂�F����\t"
		"BonDriver�Ƃ��Ẵ`���[�i���\t"
		"BonDriver�Ƃ��Ẵ`�����l��\t"
		"PTx�Ƃ��Ẵ`�����l��\t"
		"TSID(10�i���ŉq���g�ȊO��0))\n",st);
	for(size_t spc=0;spc<transponders_list.size();spc++) {
		const TRANSPONDERS &trapons = transponders_list[spc];
		for(size_t tp=0,n=0;tp<trapons.size();tp++) {
			const TRANSPONDER &trpn = trapons[tp] ;
			DWORD ptxch = BonPTx->TransponderGetPTxCh((DWORD)spc,(DWORD)tp);
			for(size_t ts=0;ts<trpn.TSIDs.size();ts++) {
				DWORD id = trpn.TSIDs[ts] ;
				fprintf(st,"%s/TS%d\t%d\t%d\t%d\t%d\n",
					trpn.Name.c_str(),
					(int)id&7,(int)spc,(int)n,(int)ptxch,(int)id);
				n++;
			}
		}
	}
	fclose(st);
	return true;
}

bool DoScanS()
{
	try {
		BonTransponder = dynamic_cast<IBonTransponder*>(BonTuner) ;
	}catch(bad_cast &e) {
		printf("Cast error: %s\n",e.what());
		puts("IBonTransponder�C���^�[�t�F�C�X���m�F�ł��܂���ł����B\n");
		BonTransponder = NULL ;
		return false;
	}
	puts("IBonTransponder�C���^�[�t�F�C�X: �L��\n");

	try {
		BonPTx = dynamic_cast<IBonPTx*>(BonTuner) ;
	}catch(bad_cast &e) {
		printf("Cast error: %s\n",e.what());
		puts("IBonPTx�C���^�[�t�F�C�X���m�F�ł��܂���ł����B\n");
		BonTransponder = NULL ;
		BonPTx = NULL ;
		return false;
	}
	puts("IBonPTx�C���^�[�t�F�C�X: �L��\n");

	puts("�X�y�[�X����񋓂��Ă��܂��B");
	str_vector spaces ;
	for(DWORD spc=0;;spc++) {
		LPCTSTR space = BonTuner->EnumTuningSpace(spc);
		if(!space) break;
		string name = wcs2mbcs(space);
		printf(" %s",name.c_str());
		spaces.push_back(name);
	}
	printf("\n�v %d �̃X�y�[�X���W�v���܂����B\n\n",(int)spaces.size());

	puts("�g�����X�|���_��񋓂��Ă��܂��B");
	vector<TRANSPONDERS> transponders_list;
	int ntrapons=0;
	for(DWORD spc=0;spc<(DWORD)spaces.size();spc++) {
		printf(" �X�y�[�X(%d): %s\n ",spc,spaces[spc].c_str());
		TRANSPONDERS trapons;
		for(DWORD tp=0;;tp++) {
			LPCTSTR tra = BonTransponder->TransponderEnumerate(spc,tp);
			if(!tra) break;
			string name = wcs2mbcs(tra);
			printf(" %s",name.c_str());
			trapons.push_back(TRANSPONDER(name));
			ntrapons++;
		}
		transponders_list.push_back(trapons);
		putchar('\n');
	}
	printf("�v %d �̃g�����X�|���_���W�v���܂����B\n\n",ntrapons);

	puts("�`���[�i�[���I�[�v�����Ă��܂�...");
	BOOL opened = BonTuner->OpenTuner();
	bool res = true ;
	if(!opened) {
		puts("�`���[�i�[�̃I�[�v���Ɏ��s�B");
		res=false;
	}else do {
		puts("�`���[�i�[�̃I�[�v���ɐ������܂����B");
		puts("�g�����X�|���_�X�L�������J�n���܂��B");
		for(DWORD spc=0;spc<(DWORD)transponders_list.size();spc++) {
			printf(" �X�y�[�X(%d): %s\n",spc,spaces[spc].c_str());
			TRANSPONDERS &trapons = transponders_list[spc];
			for(DWORD tp=0;tp<trapons.size();tp++) {
				printf("  �g�����X�|���_(%d): %s\n",tp,trapons[tp].Name.c_str());
				if(SelectTransponder(spc,tp)) {
					tsid_vector idList;
					if(MakeIDList(idList)) {
						putchar('\t');
						for(size_t i=0;i<idList.size();i++) {
							printf(" TS%d:%04x",(int)idList[i]&7,(int)idList[i]);
						}
						trapons[tp].TSIDs = idList ;
						putchar('\n');
					}else {
						puts("\t<TSID���o�s��>");
					}
				}else {
					puts("\t<�I��s��>");
				}
			}
		}
		puts("�g�����X�|���_�X�L�������������܂����B");
	}while(0);
	if(opened) {
		puts("�`���[�i�[���N���[�Y���Ă��܂�...");
		BonTuner->CloseTuner();
		puts("�`���[�i�[���N���[�Y���܂����B");
	}

	if(res) {
		string out_file ;
		{

			CHAR szFname[_MAX_FNAME];
			CHAR szExt[_MAX_EXT];
			_splitpath_s( (AppPath+BonFile).c_str(), NULL, 0, NULL, 0, szFname, _MAX_FNAME, szExt, _MAX_EXT );
			out_file = szFname ;
			out_file += ".ChSet.txt" ;
		}
		printf("\n�t�@�C�� \"%s\" �ɃX�L�������ʂ��o�͂��܂����H[y/N]> ",out_file.c_str());
		char c=tolower(getchar());
		while(c<0x20) c = tolower(getchar());
		if(c=='y') {
			printf("�g�����X�|���_�����o�͂��܂����H[y/N]> ");
			char c=tolower(getchar());
			while(c<0x20) c = tolower(getchar());
			printf("�X�L�������ʂ��t�@�C�� \"%s\" �ɏo�͂��Ă��܂�...\n",out_file.c_str());
			if(!OutChSet(out_file, spaces, transponders_list, c=='y')) {
				puts("�X�L�������ʂ̏o�͂Ɏ��s�B") ;
				res=false ;
			}else {
				printf("�X�L�������ʂ��t�@�C�� \"%s\" �ɏo�͂��܂����B\n",out_file.c_str());
			}
			Sleep(1000);
		}
	}

	BonTransponder=NULL;
	BonPTx=NULL;

	return res;
}

int _tmain(int argc, _TCHAR* argv[])
{
	{
		WCHAR szPath[_MAX_PATH];    // �p�X
		WCHAR szDrive[_MAX_DRIVE];
		WCHAR szDir[_MAX_DIR];
		WCHAR szFname[_MAX_FNAME];
		WCHAR szExt[_MAX_EXT];
		_tsplitpath_s( argv[0], szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
		_tmakepath_s(  szPath, _MAX_PATH, szDrive, szDir, NULL, NULL );
		AppPath = wcs2mbcs(szPath);
	}

	string iniFile = AppPath;
	iniFile += "BonDriver_PTx-ST.ini";
	MAXDUR_FREQ = GetPrivateProfileIntA("SET", "MAXDUR_FREQ", MAXDUR_FREQ, iniFile.c_str() ); //���g�������ɔ�₷�ő厞��(msec)
	MAXDUR_TMCC = GetPrivateProfileIntA("SET", "MAXDUR_TMCC", MAXDUR_TMCC, iniFile.c_str() ); //TMCC�擾�ɔ�₷�ő厞��(msec)

	BonFile = DoSelectS() ;
	if(BonFile.empty()) return 1 ;
	if(!LoadBon()) { FreeBon(); return 2 ;}
	if(!DoScanS()) { FreeBon(); return 3 ;}
	FreeBon();
	return 0;
}

