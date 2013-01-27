#pragma once

// �萔

#define		MEDIATYPE_TB		0x5442
#define		MEDIATYPE_BS		0x4253
#define		MEDIATYPE_CS		0x4353
#define		MEDIATYPE_UNKNOWN	0xFFFF

#define		PID_PAT				0x0000
#define		PID_SIT				0x001f
#define		PID_EIT				0x0012
#define		PID_SDT				0x0011
#define		PID_INVALID			0xffff

#define		PSIBUFSIZE			65536


// �v���g�^�C�v�錾

BOOL			readTsProgInfo(HANDLE, ProgInfo*, int, int, BOOL);
int				getSitEit(HANDLE, unsigned char*, int, int);
BOOL			getSdt(HANDLE, unsigned char*, int, int, int);
void			mjd_dec(int, int*, int*, int*);
int				comparefornidtable(const void*, const void*);
int				getTbChannelNum(int, int, int);
void			parseSit(unsigned char*, ProgInfo*, BOOL);
int				parseEit(unsigned char*, ProgInfo*, BOOL);
void			parseSdt(unsigned char*, ProgInfo*, int, BOOL);
