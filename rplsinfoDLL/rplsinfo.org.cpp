// rplsinfo.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <new.h>
#include <locale.h>
#include <wchar.h>
#include "rplsinfo.h"
#include "proginfo.h"
#include "convtounicode.h"

#include <conio.h>


// �萔�Ȃ�

#define		MAXFLAGNUM				256
#define		FILE_INVALID			0
#define		FILE_RPLS				1
#define		FILE_188TS				188
#define		FILE_192TS				192
#define		DEFAULTTSFILEPOS		50

#define		NAMESTRING				L"\nrplsinfo version 1.3 "

#ifdef _WIN64
	#define		NAMESTRING2				L"(64bit)\n"
#else
	#define		NAMESTRING2				L"(32bit)\n"
#endif


// �\���̐錾

typedef struct {
	int		argSrc;
	int		argDest;
	int		separator;
	int		flags[MAXFLAGNUM + 1];
	BOOL	bNoControl;
	BOOL	bNoComma;
	BOOL	bDQuot;
	BOOL	bItemName;
	BOOL	bDisplay;
	BOOL	bCharSize;
	int		tsfilepos;
} CopyParams;


// �v���g�^�C�v�錾

void	initCopyParams(CopyParams*);
BOOL	parseCopyParams(int, _TCHAR*[], CopyParams*);
int		rplsTsCheck(HANDLE);
BOOL	rplsMakerCheck(unsigned char*, int);
void	readRplsProgInfo(HANDLE, ProgInfo*, BOOL);
void	outputProgInfo(HANDLE, ProgInfo*, CopyParams*);
int		putGenreStr(WCHAR*, int, int*, int*);
int		getRecSrcIndex(int);
int		putRecSrcStr(WCHAR*, int, int);
int		convforcsv(WCHAR*, int, WCHAR*, int, BOOL, BOOL, BOOL, BOOL);


// ���C��

int _tmain(int argc, _TCHAR* argv[])
{
	_tsetlocale(LC_ALL, _T(""));


	// �p�����[�^�`�F�b�N

	if (argc == 1) {
		wprintf(NAMESTRING NAMESTRING2);
		exit(1);
	}

	CopyParams		param;
	initCopyParams(&param);

	if( !parseCopyParams(argc, argv, &param) ) exit(1);																	// �p�����[�^�ُ�Ȃ�I��


	// �ԑg���ǂݍ��݌��t�@�C�����J��

	HANDLE	hReadFile = CreateFile(argv[param.argSrc], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hReadFile == INVALID_HANDLE_VALUE) {
		wprintf(L"�t�@�C�� %s ���J���̂Ɏ��s���܂���.\n", argv[param.argSrc]);
		exit(1);
	}

	int		sfiletype = rplsTsCheck(hReadFile);
	if(sfiletype == FILE_INVALID) {																						// �����ȃt�@�C���̏ꍇ
		wprintf(L"�t�@�C�� %s �͗L����RPLS�t�@�C���CTS�t�@�C���ł͂���܂���.\n", argv[param.argSrc]);
		CloseHandle(hReadFile);
		exit(1);
	}


	// �ԑg���̓ǂݍ���

	ProgInfo	*proginfo;
	try {
		proginfo = new ProgInfo;
	}
	catch(...) {
		wprintf(L"�A�v���P�[�V�����������̊m�ۂɎ��s���܂���.\n");
		exit(1);
	}
	memset(proginfo, 0, sizeof(ProgInfo)); 

	_wfullpath(proginfo->fullpath, argv[param.argSrc], _MAX_PATH);														// �t���p�X���擾
	_wsplitpath_s(proginfo->fullpath, NULL, 0, NULL, 0, proginfo->fname, _MAX_PATH, proginfo->fext, _MAX_PATH);			// �x�[�X�t�@�C�����Ɗg���q

	if(sfiletype == FILE_RPLS) {
		readRplsProgInfo(hReadFile, proginfo, param.bCharSize);
	} else 	{
		BOOL bResult = readTsProgInfo(hReadFile, proginfo, sfiletype, param.tsfilepos, param.bCharSize);
		if(!bResult) {
			wprintf(L"TS�t�@�C�� %s ����L���Ȕԑg�������o�ł��܂���ł���.\n", argv[param.argSrc]);
			CloseHandle(hReadFile);
			delete proginfo;
			exit(1);
		}
	}

	CloseHandle(hReadFile);


	// �K�v�Ȃ�o�̓t�@�C�����J��

	DWORD	dwNumWrite;
	HANDLE	hWriteFile = INVALID_HANDLE_VALUE;

	if(!param.bDisplay) {

		hWriteFile = CreateFile(argv[param.argDest], GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hWriteFile == INVALID_HANDLE_VALUE) {
			wprintf(L"�ۑ���t�@�C�� %s ���J���܂���ł���.\n", argv[param.argDest]);
			delete proginfo;
			exit(1);
		}

		LARGE_INTEGER	fsize;
		GetFileSizeEx(hWriteFile, &fsize);

		if(fsize.QuadPart == 0) {
			WCHAR	bom = 0xFEFF;
			WriteFile(hWriteFile, &bom, sizeof(bom), &dwNumWrite, NULL);									// �t�@�C������Ȃ�BOM�o��
		} else {
			SetFilePointerEx(hWriteFile, fsize, NULL, FILE_BEGIN);											// ��łȂ��Ȃ�t�@�C���̍Ō���Ɉړ�
		}

	}


	// �I�v�V�����w��ɉ����Ĕԑg���e���o��

	outputProgInfo(hWriteFile, proginfo, &param);


	// �I������

	if(!param.bDisplay) CloseHandle(hWriteFile);
	delete proginfo;

	return 0;
}


void initCopyParams(CopyParams *param)
{
	param->argSrc			= 0;
	param->argDest			= 0;

	param->separator		= S_NORMAL;
	param->bNoControl		= TRUE;
	param->bNoComma			= TRUE;
	param->bDQuot			= FALSE;
	param->bItemName		= FALSE;

	param->bDisplay			= FALSE;
	param->bCharSize		= FALSE;

	param->tsfilepos		= DEFAULTTSFILEPOS;

	memset(param->flags, 0,sizeof(param->flags));

	return;
}


BOOL parseCopyParams(int argn, _TCHAR *args[], CopyParams *param)
{
	int		fnum		= 0;
	BOOL	bArgSkip	= FALSE;

	for(int	i = 1; i < argn; i++) {

		if( args[i][0] == L'-') {

			for(int	j = 1; j < (int)wcslen(args[i]); j++) {

				switch(args[i][j])
				{
					case L'f':
						param->flags[fnum++] = F_FileName;
						break;
					case L'u':
						param->flags[fnum++] = F_FileNameFullPath;
						break;
					case L'd':
						param->flags[fnum++] = F_RecDate;
						break;
					case L't':
						param->flags[fnum++] = F_RecTime;
						break;
					case L'p':
						param->flags[fnum++] = F_RecDuration;
						break;
					case L'z':
						param->flags[fnum++] = F_RecTimeZone;
						break;
					case L'a':
						param->flags[fnum++] = F_MakerID;
						break;
					case L'o':
						param->flags[fnum++] = F_ModelCode;
						break;
					case L's':
						param->flags[fnum++] = F_RecSrc;
						break;
					case L'c':
						param->flags[fnum++] = F_ChannelName;
						break;
					case L'n':
						param->flags[fnum++] = F_ChannelNum;
						break;
					case L'b':
						param->flags[fnum++] = F_ProgName;
						break;
					case L'i':
						param->flags[fnum++] = F_ProgDetail;
						break;
					case L'e':
						param->flags[fnum++] = F_ProgExtend;
						break;
					case L'g':
						param->flags[fnum++] = F_ProgGenre;
						break;
					case L'T':
						param->separator	= S_TAB;
						param->bNoControl	= TRUE;
						param->bNoComma		= FALSE;
						param->bDQuot		= FALSE;
						param->bItemName	= FALSE;
						break;
					case L'S':
						param->separator	= S_SPACE;
						param->bNoControl	= TRUE;
						param->bNoComma		= FALSE;
						param->bDQuot		= FALSE;
						param->bItemName	= FALSE;
						break;
					case L'C':
						param->separator	= S_CSV;
						param->bNoControl	= FALSE;
						param->bNoComma		= FALSE;
						param->bDQuot		= TRUE;
						param->bItemName	= FALSE;
						break;
					case L'N':
						param->separator	= S_NEWLINE;
						param->bNoControl	= FALSE;
						param->bNoComma		= FALSE;
						param->bDQuot		= FALSE;
						param->bItemName	= FALSE;
						break;
					case L'I':
						param->separator	= S_ITEMNAME;
						param->bNoControl	= FALSE;
						param->bNoComma		= FALSE;
						param->bDQuot		= FALSE;
						param->bItemName	= TRUE;
						break;
					case L'y':
						param->bCharSize = TRUE;
						break;
					case L'F':
						if( (i == (argn - 1)) || (_wtoi(args[i + 1]) < 0) || (_wtoi(args[i + 1]) > 99) ) {
							wprintf(L"-F �I�v�V�����̈������ُ�ł�.\n");
							return	FALSE;
						}
						param->tsfilepos = _wtoi(args[i + 1]);
						bArgSkip = TRUE;
						break;
					default:
						wprintf(L"�����ȃX�C�b�`���w�肳��܂���.\n");
						return	FALSE;
				}

				if(fnum == MAXFLAGNUM) {
					wprintf(L"�X�C�b�`�w�肪�������܂�.\n");
					return	FALSE;
				}
			}
		
		} else {

			if(param->argSrc == 0) {
				param->argSrc = i;
			} 
			else if ( param->argDest == 0) {
				param->argDest = i;
			}
			else {
				wprintf(L"�p�����[�^���������܂�.\n");
				return FALSE;
			}
		}

		if(bArgSkip){
			i++;
			bArgSkip = FALSE;
		}

	}

	if(param->argSrc == 0) {
		wprintf(L"�p�����[�^������܂���.\n");
		return	FALSE;
	}

	if(param->argDest == 0)	param->bDisplay = TRUE;

	return	TRUE;
}


int rplsTsCheck(HANDLE hReadFile)
{
	DWORD			dwNumRead;
	
	unsigned char	buf[1024];
	memset(buf, 0, 1024);

	SetFilePointer(hReadFile, 0, NULL, FILE_BEGIN);
	ReadFile(hReadFile, buf, 1024, &dwNumRead, NULL);

	if( (buf[0] == 'P') && (buf[1] == 'L') && (buf[2] == 'S') && (buf[3] == 'T') ){
		return	FILE_RPLS;
	}
	else if( (buf[188 * 0] == 0x47) && (buf[188 * 1] == 0x47) && (buf[188 * 2] == 0x47) && (buf[188 * 3] == 0x47) ){
		return	FILE_188TS;
	}
	else if( (buf[192 * 0 + 4] == 0x47) && (buf[192 * 1 + 4] == 0x47) && (buf[192 * 2 + 4] == 0x47) && (buf[192 * 3 + 4] == 0x47) ){
		return	FILE_192TS;
	}

	return	FILE_INVALID;
}


BOOL rplsMakerCheck(unsigned char *buf, int idMaker)
{
	unsigned int	mpadr = (buf[ADR_MPDATA] << 24) + (buf[ADR_MPDATA + 1] << 16) + (buf[ADR_MPDATA + 2] << 8) + buf[ADR_MPDATA + 3];
	if(mpadr == 0) return FALSE;

	unsigned char	*mpdata = buf + mpadr;
	int		makerid = (mpdata[ADR_MPMAKERID] << 8) + mpdata[ADR_MPMAKERID + 1];

	if(makerid != idMaker) return FALSE;

	return TRUE;
}


void readRplsProgInfo(HANDLE hFile, ProgInfo *proginfo, BOOL bCharSize)
{
	int				fsize = GetFileSize(hFile, NULL);
	unsigned char	*buf;

	try {
		buf = new unsigned char[fsize];
	}
	catch(...) {
		wprintf(L"�ǂݍ��݃o�b�t�@�������̊m�ۂɎ��s���܂���.\n");
		exit(1);
	}

	DWORD		dwNumRead;

	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	ReadFile(hFile, buf, fsize, &dwNumRead, NULL);

	proginfo->bSonyRpls = rplsMakerCheck(buf, MAKERID_SONY);
	proginfo->bPanaRpls = rplsMakerCheck(buf, MAKERID_PANASONIC);

	unsigned char	*appinfo = buf + ADR_APPINFO;
	unsigned char	*mpdata = buf + (buf[ADR_MPDATA] << 24) + (buf[ADR_MPDATA + 1] << 16) + (buf[ADR_MPDATA + 2] << 8) + buf[ADR_MPDATA + 3];

	proginfo->recyear	= (appinfo[ADR_RECYEAR] >> 4) * 1000 + (appinfo[ADR_RECYEAR] & 0x0F) * 100 + (appinfo[ADR_RECYEAR + 1] >> 4) * 10 + (appinfo[ADR_RECYEAR + 1] & 0x0F);
	proginfo->recmonth	= (appinfo[ADR_RECMONTH] >> 4) * 10 + (appinfo[ADR_RECMONTH] & 0x0F);
	proginfo->recday	= (appinfo[ADR_RECDAY] >> 4) * 10 + (appinfo[ADR_RECDAY] & 0x0F);
	proginfo->rechour	= (appinfo[ADR_RECHOUR] >> 4) * 10 + (appinfo[ADR_RECHOUR] & 0x0F);
	proginfo->recmin	= (appinfo[ADR_RECMIN] >> 4) * 10 + (appinfo[ADR_RECMIN] & 0x0F);
	proginfo->recsec	= (appinfo[ADR_RECSEC] >> 4) * 10 + (appinfo[ADR_RECSEC] & 0x0F);

	proginfo->durhour	= (appinfo[ADR_DURHOUR] >> 4) * 10 + (appinfo[ADR_DURHOUR] & 0x0F);
	proginfo->durmin	= (appinfo[ADR_DURMIN] >> 4) * 10 + (appinfo[ADR_DURMIN] & 0x0F);
	proginfo->dursec	= (appinfo[ADR_DURSEC] >> 4) * 10 + (appinfo[ADR_DURSEC] & 0x0F);

	proginfo->rectimezone = appinfo[ADR_TIMEZONE];

	proginfo->makerid	= appinfo[ADR_MAKERID] * 256 + appinfo[ADR_MAKERID + 1];
	proginfo->modelcode	= appinfo[ADR_MODELCODE] * 256 + appinfo[ADR_MODELCODE + 1];

	if(proginfo->bPanaRpls) {
		proginfo->recsrc = getRecSrcIndex(mpdata[ADR_RECSRC_PANA] * 256 + mpdata[ADR_RECSRC_PANA + 1]);			// pana�Ȃ������ʏ��擾
	} else {
		proginfo->recsrc = -1;																					// ��pana�Ȃ������ʏ�񖳂�
	}

	proginfo->chnum		= appinfo[ADR_CHANNELNUM] * 256 + appinfo[ADR_CHANNELNUM + 1];

	proginfo->chnamelen	= conv_to_unicode(proginfo->chname, CONVBUFSIZE, appinfo + ADR_CHANNELNAME + 1, (int)appinfo[ADR_CHANNELNAME], bCharSize);
	proginfo->pnamelen	= conv_to_unicode(proginfo->pname, CONVBUFSIZE, appinfo + ADR_PNAME + 1, (int)appinfo[ADR_PNAME], bCharSize);

	int		pdetaillen		= appinfo[ADR_PDETAIL] * 256 + appinfo[ADR_PDETAIL + 1];
	proginfo->pdetaillen	= conv_to_unicode(proginfo->pdetail, CONVBUFSIZE, appinfo + ADR_PDETAIL + 2, pdetaillen, bCharSize); 

	if(proginfo->bSonyRpls) {
		int		pextendlen		= mpdata[ADR_PEXTENDLEN] * 256 + mpdata[ADR_PEXTENDLEN + 1];
		proginfo->pextendlen	= conv_to_unicode(proginfo->pextend, CONVBUFSIZE, appinfo + ADR_PDETAIL + 2 + pdetaillen, pextendlen, bCharSize);
		for(int i = 0; i < 3; i++) {
			proginfo->genretype[i]	= mpdata[ADR_GENRE + i * 4 + 0];
			proginfo->genre[i]		= mpdata[ADR_GENRE + i * 4 + 1];
		}
	}

	if(proginfo->bPanaRpls) {
		switch(mpdata[ADR_GENRE_PANA])
		{
			case 0xD5:
				proginfo->genretype[2] = 0x01;
				proginfo->genre[2] = ((mpdata[ADR_GENRE_PANA + 1] & 0x0F) << 4) + (mpdata[ADR_GENRE_PANA + 1] >> 4);
			case 0xE5:
				proginfo->genretype[1] = 0x01;
				proginfo->genre[1] = ((mpdata[ADR_GENRE_PANA + 2] & 0x0F) << 4) + (mpdata[ADR_GENRE_PANA + 2] >> 4);
			case 0xE9:
				proginfo->genretype[0] = 0x01;
				proginfo->genre[0] = ((mpdata[ADR_GENRE_PANA + 3] & 0x0F) << 4) + (mpdata[ADR_GENRE_PANA + 3] >> 4);
				break;
			default:
				break;
		}
	}

	delete [] buf;

	return;
}


void outputProgInfo(HANDLE hFile, ProgInfo *proginfo, CopyParams *param)
{
	DWORD	dwNumWrite;
	WCHAR	sstr[CONVBUFSIZE];
	WCHAR	dstr[CONVBUFSIZE];

	int		i = 0;

	while(param->flags[i] != 0) {
	
		int		slen = 0;

		// flags�ɉ����ďo�͂��鍀�ڂ�sstr�ɃR�s�[����

		switch(param->flags[i])
		{
			case F_FileName:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[�t�@�C����]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%s%s", proginfo->fname, proginfo->fext);
				break;
			case F_FileNameFullPath:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[�t���p�X�t�@�C����]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, proginfo->fullpath);
				break;
			case F_RecDate:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[�^����t]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%.4d/%.2d/%.2d", proginfo->recyear, proginfo->recmonth, proginfo->recday);
				break;
			case F_RecTime:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[�^�掞��]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%.2d:%.2d:%.2d", proginfo->rechour, proginfo->recmin, proginfo->recsec);
				break;
			case F_RecDuration:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[�^�����]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%.2d:%.2d:%.2d", proginfo->durhour, proginfo->durmin, proginfo->dursec);
				break;
			case F_RecTimeZone:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[�^�C���]�[��]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%d", proginfo->rectimezone);
				break;
			case F_MakerID:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[���[�J�[ID]\r\n");
				if(proginfo->makerid != -1) {
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%d", proginfo->makerid);
				} else {
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"n/a");
				}
				break;
			case F_ModelCode:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[���[�J�[�@��R�[�h]\r\n");
				if(proginfo->modelcode != -1) {
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%d", proginfo->modelcode);
				} else {
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"n/a");
				}
				break;
			case F_RecSrc:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[�������]\r\n");
				if(proginfo->recsrc == -1) {
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"n/a");
				} else {
					slen += putRecSrcStr(sstr + slen, CONVBUFSIZE - slen, proginfo->recsrc);
				}
				break;
			case F_ChannelNum:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[�`�����l���ԍ�]\r\n");
				slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"%.3dch", proginfo->chnum);
				break;
			case F_ChannelName:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[�����ǖ�]\r\n");
				wmemcpy_s(sstr + slen, CONVBUFSIZE - slen, proginfo->chname, proginfo->chnamelen);
				slen += proginfo->chnamelen;
				break;
			case F_ProgName:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[�ԑg��]\r\n");
				wmemcpy_s(sstr + slen, CONVBUFSIZE - slen, proginfo->pname, proginfo->pnamelen);
				slen += proginfo->pnamelen;
				break;
			case F_ProgDetail:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[�ԑg���e]\r\n");
				wmemcpy_s(sstr + slen, CONVBUFSIZE - slen, proginfo->pdetail, proginfo->pdetaillen);
				slen += proginfo->pdetaillen;
				break;
			case F_ProgExtend:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[���e�ڍ�]\r\n");
				if(!proginfo->bSonyRpls){
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"n/a");
				} else {
					wmemcpy_s(sstr + slen, CONVBUFSIZE - slen, proginfo->pextend, proginfo->pextendlen);
					slen += proginfo->pextendlen;
				}
				break;
			case F_ProgGenre:
				if(param->bItemName) slen = swprintf_s(sstr, CONVBUFSIZE, L"[�ԑg�W������]\r\n");
				if( (!proginfo->bSonyRpls && !proginfo->bPanaRpls) || (proginfo->genretype[0] != 0x01) ) {
					slen += swprintf_s(sstr + slen, CONVBUFSIZE - slen, L"n/a");
				} else {
					slen += putGenreStr(sstr + slen, CONVBUFSIZE - slen, proginfo->genretype, proginfo->genre);
				}
				break;
			default:
				slen = 0;
				break;
		}


		// �o�͌`���ɉ�����sstr�𒲐�

		int	dlen =  convforcsv(dstr, CONVBUFSIZE - 4, sstr, slen, param->bNoControl, param->bNoComma, param->bDQuot, param->bDisplay);


		// �Z�p���[�^�Ɋւ��鏈��

		if(param->flags[i + 1] != 0)
		{
			switch(param->separator)
			{
				case S_NORMAL:
				case S_CSV:
					dstr[dlen++] = L',';								// COMMA
					break;
				case S_TAB:
					dstr[dlen++] = 0x0009;								// TAB
					break;
				case S_SPACE:
					dstr[dlen++] = L' ';								// SPACE
					break;
				case S_NEWLINE:
				case S_ITEMNAME:
					if(!param->bDisplay) dstr[dlen++] = 0x000D;			// ���s2��
					dstr[dlen++] = 0x000A;
					if(!param->bDisplay) dstr[dlen++] = 0x000D;
					dstr[dlen++] = 0x000A;
					break;
				default:
					break;
			}
		}
		else
		{
			if(!param->bDisplay) dstr[dlen++] = 0x000D;											// �S���ڏo�͌�̉��s
			dstr[dlen++] = 0x000A;
			if( (param->separator == S_NEWLINE) || (param->separator == S_ITEMNAME) ) {			// �Z�p���[�^��S_NEWLINE, S_ITEMNAME�̏ꍇ�͂P�s�]���ɉ��s����
				if(!param->bDisplay) dstr[dlen++] = 0x000D;
				dstr[dlen++] = 0x000A;
			}
		}


		// �f�[�^�o��

		if(!param->bDisplay){
			WriteFile(hFile, dstr, dlen * sizeof(WCHAR), &dwNumWrite, NULL);					// �f�B�X�N�o��
		} else {
			dstr[dlen] = 0x0000;
			wprintf(L"%s", dstr);																// �R���\�[���o��
		}

		i++;
	}

	return;
}


int putGenreStr(WCHAR *buf, int bufsize, int *genretype, int *genre)
{
	WCHAR	*str_genreL[] = {
		L"�j���[�X�^��",			L"�X�|�[�c",	L"���^���C�h�V���[",	L"�h���}",
		L"���y",					L"�o���G�e�B",	L"�f��",				L"�A�j���^���B",
		L"�h�L�������^���[�^���{",	L"����^����",	L"��^����",			L"����",
		L"�\��",					L"�\��",		L"�g��",				L"���̑�"
	};

	WCHAR	*str_genreM[] = {
		L"�莞�E����", L"�V�C", L"���W�E�h�L�������g", L"�����E����", L"�o�ρE�s��", L"�C�O�E����", L"���", L"���_�E��k",
		L"�񓹓���", L"���[�J���E�n��", L"���", L"-", L"-", L"-", L"-", L"���̑�",

		L"�X�|�[�c�j���[�X", L"�싅", L"�T�b�J�[", L"�S���t", L"���̑��̋��Z", L"���o�E�i���Z", L"�I�����s�b�N�E���ۑ��", L"�}���\���E����E���j",
		L"���[�^�[�X�|�[�c", L"�}�����E�E�B���^�[�X�|�[�c", L"���n�E���c���Z", L"-", L"-", L"-", L"-", L"���̑�",

		L"�|�\�E���C�h�V���[", L"�t�@�b�V����", L"��炵�E�Z�܂�", L"���N�E���", L"�V���b�s���O�E�ʔ�", L"�O�����E����", L"�C�x���g", L"�ԑg�Љ�E���m�点",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�����h���}", L"�C�O�h���}", L"���㌀", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�������b�N�E�|�b�v�X", L"�C�O���b�N�E�|�b�v�X", L"�N���V�b�N�E�I�y��", L"�W���Y�E�t���[�W����", L"�̗w�ȁE����", L"���C�u�E�R���T�[�g", L"�����L���O�E���N�G�X�g", L"�J���I�P�E�̂ǎ���",
		L"���w�E�M�y", L"���w�E�L�b�Y", L"�������y�E���[���h�~���[�W�b�N", L"-", L"-", L"-", L"-", L"���̑�",

		L"�N�C�Y", L"�Q�[��", L"�g�[�N�o���G�e�B", L"���΂��E�R���f�B", L"���y�o���G�e�B", L"���o���G�e�B", L"�����o���G�e�B", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�m��", L"�M��", L"�A�j��", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�����A�j��", L"�C�O�A�j��", L"���B", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"�Љ�E����", L"���j�E�I�s", L"���R�E�����E��", L"�F���E�Ȋw�E��w", L"�J���`���[�E�`���|�\", L"���w�E���|", L"�X�|�[�c", L"�h�L�������^���[�S��",
		L"�C���^�r���[�E���_", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"���㌀�E�V��", L"�~���[�W�J��", L"�_���X�E�o���G", L"����E���|", L"�̕���E�ÓT", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"���E�ނ�E�A�E�g�h�A", L"���|�E�y�b�g�E��|", L"���y�E���p�E�H�|", L"�͌�E����", L"�����E�p�`���R", L"�ԁE�I�[�g�o�C", L"�R���s���[�^�E�s�u�Q�[��", L"��b�E��w",
		L"�c���E���w��", L"���w���E���Z��", L"��w���E��", L"���U����E���i", L"������", L"-", L"-", L"���̑�",

		L"�����", L"��Q��", L"�Љ��", L"�{�����e�B�A", L"��b", L"�����i�����j", L"�������", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"BS/�n��f�W�^�������p�ԑg�t�����", L"�L�ш�CS�f�W�^�������p�g��", L"�q���f�W�^�����������p�g��", L"�T�[�o�[�^�ԑg�t�����", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",

		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"-",
		L"-", L"-", L"-", L"-", L"-", L"-", L"-", L"���̑�"
	};

	int len;

	if(genretype[2] == 0x01) {
		len =  swprintf_s(buf, (size_t)bufsize, L"%s �k%s�l�@%s �k%s�l�@%s �k%s�l", str_genreL[genre[0] >> 4], str_genreM[genre[0]], str_genreL[genre[1] >> 4], str_genreM[genre[1]], str_genreL[genre[2] >> 4], str_genreM[genre[2]]);
	} else if(genretype[1] == 0x01) {
		len =  swprintf_s(buf, (size_t)bufsize, L"%s �k%s�l�@%s �k%s�l", str_genreL[genre[0] >> 4], str_genreM[genre[0]], str_genreL[genre[1] >> 4], str_genreM[genre[1]]);
	} else {
		len =  swprintf_s(buf, (size_t)bufsize, L"%s �k%s�l", str_genreL[genre[0] >> 4],  str_genreM[genre[0]]);
	}

	return len;
}


int getRecSrcIndex(int num)
{
	int		recsrc_table[] = {		// ������putRecSrcStr�ƑΉ�����K�v����
		0x5444,						// TD	�n��f�W�^��
		0x4244,						// BD	BS�f�W�^��
		0x4331,						// C1	CS�f�W�^��1
		0x4332,						// C2	CS�f�W�^��2
		0x694C,						// iL	i.LINK(TS)����
		0x4D56,						// MV	AVCHD
		0x534B,						// SK	�X�J�p�[(DLNA)
		0x4456,						// DV	DV����
		0x5441,						// TA	�n��A�i���O
		0x4E4C,						// NL	���C������
	};

	// �����ȃe�[�u���Ȃ̂ŏ����T����

	for(int i = 0; i < (sizeof(recsrc_table) / sizeof(int)); i++) {
		if(num == recsrc_table[i]) return i;
	}

	return	(sizeof(recsrc_table) / sizeof(int));			// �s����recsrc
}


int putRecSrcStr(WCHAR *buf, int bufsize, int index)
{
	WCHAR	*str_recsrc[] = {
		L"�n��f�W�^��",
		L"BS�f�W�^��",
		L"CS�f�W�^��1",
		L"CS�f�W�^��2",
		L"i.LINK(TS)",
		L"AVCHD",
		L"�X�J�p�[(DLNA)",
		L"DV����",
		L"�n��A�i���O",
		L"���C������",
		L"unknown",
	};

	int	len = 0;

	if( (index >= 0) && (index < (sizeof(str_recsrc) / sizeof(WCHAR*))) ) {
		len = swprintf_s(buf, (size_t)bufsize, L"%s", str_recsrc[index]);
	}

	return len;
}


int convforcsv(WCHAR *dbuf, int bufsize, WCHAR *sbuf, int slen, BOOL bNoControl, BOOL bNoComma, BOOL bDQuot, BOOL bDisplay)
{
	int dst = 0;

	if( bDQuot && (dst < bufsize) ) dbuf[dst++] = 0x0022;		//  �u"�v						// CSV�p�o�͂Ȃ獀�ڂ̑O����u"�v�ň͂�

	for(int	src = 0; src < slen; src++)
	{
		WCHAR	s = sbuf[src];
		BOOL	bOut = TRUE;

		if( bNoControl && (s < L' ') ) bOut = FALSE;											// ����R�[�h�͏o�͂��Ȃ�
		if( bNoComma && (s == L',') ) bOut = FALSE;												// �R���}�͏o�͂��Ȃ�
		if( bDisplay && (s == 0x000D) ) bOut = FALSE;											// �R���\�[���o�͂̏ꍇ�͉��s�R�[�h��0x000D�͏ȗ�����

		if( bDQuot && (s == 0x0022) && (dst < bufsize) ) dbuf[dst++] = 0x0022;					// CSV�p�o�͂Ȃ�u"�v�̑O�Ɂu"�v�ŃG�X�P�[�v
		if( bOut && (dst < bufsize) ) dbuf[dst++] = s;											// �o��
	}

	if( bDQuot && (dst < bufsize) ) dbuf[dst++] = 0x0022;		//  �u"�v

	if(dst < bufsize) dbuf[dst] = 0x0000;

	return dst;
}