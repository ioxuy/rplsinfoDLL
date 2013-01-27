#pragma once

#include "stdafx.h"
#include <stdio.h>
#include <windows.h>


// �萔�Ȃ�

#define			READBUFSIZE				65536
#define			READBUFMERGIN			1024


// �f�B�X�N���o�͗p

typedef struct {
	HANDLE			hFile;
	int				psize;
	int				poffset;
	unsigned char	databuf[READBUFMERGIN + READBUFSIZE];
	unsigned char	*buf;
	int				datasize;
	int				pos;
	BOOL			bShowError;
} TsReadProcess;


// �v���g�^�C�v�錾

int				getPid(unsigned char*);
int				getPidValue(unsigned char*);
BOOL			isPsiTop(unsigned char*);
BOOL			isScrambled(unsigned char*);
int				getAdapFieldLength(unsigned char*);
int				getPointerFieldLength(unsigned char*);
int				getSectionLength(unsigned char*);
int				getLength(unsigned char*);
int				getPsiLength(unsigned char*);

int				parsePat(unsigned char*);
void			parsePmt(unsigned char*, int*, int*, int*, int*, BOOL, BOOL);
unsigned int	calc_crc32(unsigned char*, int);
BOOL			isPcrData(unsigned char*);
__int64			getPcrValue(unsigned char*);

void			initTsFileRead(TsReadProcess*, HANDLE, int);
void			setPointerTsFileRead(TsReadProcess*, __int64);
void			setPosTsFileRead(TsReadProcess*, int);
void			showErrorTsFileRead(TsReadProcess*, BOOL);
unsigned char*	getPacketTsFileRead(TsReadProcess*);


