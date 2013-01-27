//
#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "tsprocess.h"
#include "rplsinfo.h"
#include "proginfo.h"

#include "convtounicode.h"


//

BOOL readTsProgInfo(HANDLE hFile, ProgInfo *proginfo, int psize, int startpos, BOOL bCharSize)
{
	unsigned char	psibuf[PSIBUFSIZE];
	memset(psibuf, 0, PSIBUFSIZE);

	int	tstype = getSitEit(hFile, psibuf, psize, startpos);

	if(tstype == PID_INVALID) return FALSE;										// �ԑg���(SIT, EIT)�����o�ł��Ȃ������ꍇ

	if(tstype == PID_SIT) {
		parseSit(psibuf, proginfo, bCharSize);
	} else {
		int serviceid = parseEit(psibuf, proginfo, bCharSize);
		if( getSdt(hFile, psibuf, psize, serviceid, startpos) ) parseSdt(psibuf, proginfo, serviceid, bCharSize);
	}

	proginfo->rectimezone	= 18;												// TS�t�@�C���\�[�X�ł͓��e�Ɋ֌W�Ȃ��^�C���]�[�����18�Ƃ݂Ȃ�
	proginfo->makerid		= -1;												// ���[�J�[ID, �@��R�[�h�͖���
	proginfo->modelcode		= -1;
	proginfo->recsrc		= -1;												// ������ʏ�񖳂�
	proginfo->bSonyRpls		= TRUE;												// �ԑg���e�ڍ׏��C�ԑg�W��������񂪂�����̂Ƃ��Ĉ���

	return TRUE;
}


int getSitEit(HANDLE hFile, unsigned char *psibuf, int psize, int startpos)
{
	unsigned char	*buf;
	unsigned char	sitbuf[PSIBUFSIZE];
	unsigned char	eitbuf[PSIBUFSIZE];
	unsigned char	patbuf[PSIBUFSIZE];

	memset(sitbuf, 0, sizeof(sitbuf));
	memset(eitbuf, 0, sizeof(eitbuf));
	memset(patbuf, 0, sizeof(patbuf));

	BOOL		bSitFound	= FALSE;
	BOOL		bEitFound	= FALSE;
	BOOL		bPatFound	= FALSE;

	BOOL		bSitStarted	= FALSE;
	BOOL		bEitStarted	= FALSE;
	BOOL		bPatStarted = FALSE;

	int			sitlen = 0;
	int			eitlen = 0;
	int			patlen = 0;

	int			service_id = 0;

	TsReadProcess	tsin;
	initTsFileRead(&tsin, hFile, psize);
	setPosTsFileRead(&tsin, startpos);

	while(1)
	{
		buf = getPacketTsFileRead(&tsin);
		if(buf == NULL) break;
		buf += (psize - 188);

		int		pid		= getPid(buf);

		if( (pid == PID_SIT) || (pid == PID_EIT) || (pid == PID_PAT) )
		{
			BOOL	bPsiTop = isPsiTop(buf);
			int		adaplen = getAdapFieldLength(buf);
			int		pflen	= getPointerFieldLength(buf);						// !bPsiTop�ȏꍇ�͖��Ӗ��Ȓl

			int		len;
			BOOL	bTop = FALSE;
			BOOL	bFirstSection = TRUE;
			int		i = adaplen + 4;

			while(i < 188)														// 188�o�C�g�p�P�b�g���ł̊e�Z�N�V�����̃��[�v
			{
				if(!bPsiTop) {
					len = 188 - i;
					bTop = FALSE;
				} else if( bFirstSection && (pflen != 0) ) {
					i++;
					len = pflen;
					bTop = FALSE;
					bFirstSection = FALSE;
				} else {
					if( bFirstSection && (pflen == 0) ) {
						i++;
						bFirstSection = FALSE;
					}
					if(buf[i] == 0xFF) break;									// TableID�̂���ׂ��ꏊ��0xFF(stuffing byte)�Ȃ炻�̃p�P�b�g�Ɋւ��鏈���͏I��
					len = getSectionLength(buf + i) + 3;						// �Z�N�V�����w�b�_�̓p�P�b�g�ɂ܂������Ĕz�u����邱�Ƃ͖����͂��Ȃ̂Ńp�P�b�g�͈͊O(188�o�C�g�ȍ~)��ǂ݂ɍs�����Ƃ͖����͂�
					if(i + len > 188) len = 188 - i;
					bTop = TRUE;
				}

				// SIT�Ɋւ��鏈��

				if(pid == PID_SIT) {
					if(bSitStarted && !bTop) {
						memcpy(sitbuf + sitlen, buf + i, len);
						sitlen += len;
					}
					if(bTop) {
						memset(sitbuf, 0, sizeof(sitbuf));
						memcpy(sitbuf, buf + i, len);
						sitlen = len;
						bSitStarted = TRUE;
					}
					if( bSitStarted && (sitlen >= getSectionLength(sitbuf) + 3) ) {
						int		seclen = getSectionLength(sitbuf);
						int		crc = calc_crc32(sitbuf, seclen + 3);
						if( (crc == 0) && (sitbuf[0] == 0x7F) ) {
							memcpy(psibuf, sitbuf, seclen + 3);
							bSitFound = TRUE;
							break;
						}
						bSitStarted = FALSE;
					}
				}

				// EIT�Ɋւ��鏈��

				if(pid == PID_EIT) {
					if(bEitStarted && !bTop) {
						memcpy(eitbuf + eitlen, buf + i, len);
						eitlen += len;
					}
					if(bTop) {
						memset(eitbuf, 0, sizeof(eitbuf));
						memcpy(eitbuf, buf + i, len);
						eitlen = len;
						bEitStarted = TRUE;
					}
					if( bEitStarted && (eitlen >= getSectionLength(eitbuf) + 3) ) {
						int		seclen = getSectionLength(eitbuf);
						int		crc = calc_crc32(eitbuf, seclen + 3);
						int		sid = eitbuf[0x03] * 256 + eitbuf[0x04];
						if( (crc == 0) && (eitbuf[0] == 0x4E) && (eitbuf[0x06] == 0) && bPatFound && (sid == service_id) && (seclen > 15) ) {
							memcpy(psibuf, eitbuf, seclen + 3);
							bEitFound = TRUE;
							break;
						}
						bEitStarted = FALSE;
					}
				}

				// PAT�Ɋւ��鏈��

				if( (pid == PID_PAT) && !bPatFound ) {
					if(bPatStarted && !bTop) {
						memcpy(patbuf + patlen, buf + i, len);
						patlen += len;
					}
					if(bTop) {
						memset(patbuf, 0, sizeof(patbuf));
						memcpy(patbuf, buf + i, len);
						patlen = len;
						bPatStarted = TRUE;
					}
					if( bPatStarted && (patlen >= getSectionLength(patbuf) + 3) ) {
						int		seclen = getSectionLength(patbuf);
						int		crc = calc_crc32(patbuf, seclen + 3);
						if( (crc == 0) && (patbuf[0] == 0x00) ) {
							int		j = 0x08;
							while(j < seclen - 1) {
								service_id = patbuf[j] * 256 + patbuf[j + 1];
								if(service_id != 0) {
									bPatFound = TRUE;
									break;
								}
								j += 4;
							}
						}
						bPatStarted = FALSE;
					}
				}

				if( bSitFound || bEitFound ) break;

				i += len;
			}
		}

		if( bSitFound || bEitFound ) break;
	}

	if(bSitFound) return PID_SIT;
	if(bEitFound) return PID_EIT;

	return PID_INVALID;
}


BOOL getSdt(HANDLE hFile, unsigned char *psibuf, int psize, int serviceid, int startpos)
{
	unsigned char	*buf;
	unsigned char	sdtbuf[PSIBUFSIZE];

	BOOL		bSdtFound	= FALSE;
	BOOL		bSdtStarted	= FALSE;
	int			sdtlen		= 0;

	TsReadProcess	tsin;
	initTsFileRead(&tsin, hFile, psize);
	setPosTsFileRead(&tsin, startpos);

	while(1)
	{
		buf = getPacketTsFileRead(&tsin);
		if(buf == NULL) break;
		buf += (psize - 188);

		int		pid		= getPid(buf);

		if(pid == PID_SDT) 
		{
			BOOL	bPsiTop = isPsiTop(buf);
			int		adaplen = getAdapFieldLength(buf);
			int		pflen	= getPointerFieldLength(buf);						// !bPsiTop�ȏꍇ�͖��Ӗ��Ȓl

			int		len;
			BOOL	bTop = FALSE;
			BOOL	bFirstSection = TRUE;
			int		i = adaplen + 4;

			while(i < 188)														// 188�o�C�g�p�P�b�g���ł̊e�Z�N�V�����̃��[�v
			{
				if(!bPsiTop) {
					len = 188 - i;
					bTop = FALSE;
				} else if( bFirstSection && (pflen != 0) ) {
					i++;
					len = pflen;
					bTop = FALSE;
					bFirstSection = FALSE;
				} else {
					if( bFirstSection && (pflen == 0) ) {
						i++;
						bFirstSection = FALSE;
					}
					if(buf[i] == 0xFF) break;									// TableID�̂���ׂ��ꏊ��0xFF(stuffing byte)�Ȃ炻�̃p�P�b�g�Ɋւ��鏈���͏I��
					len = getSectionLength(buf + i) + 3;						// �Z�N�V�����w�b�_�̓p�P�b�g�ɂ܂������Ĕz�u����邱�Ƃ͖����͂��Ȃ̂Ńp�P�b�g�͈͊O(188�o�C�g�ȍ~)��ǂ݂ɍs�����Ƃ͖����͂�
					if(i + len > 188) len = 188 - i;
					bTop = TRUE;
				}

				if(bSdtStarted && !bTop) {
					memcpy(sdtbuf + sdtlen, buf + i, len);
					sdtlen += len;
				}
				if(bTop) {
					memset(sdtbuf, 0, sizeof(sdtbuf));
					memcpy(sdtbuf, buf + i, len);
					sdtlen = len;
					bSdtStarted = TRUE;
				}
				if( bSdtStarted && (sdtlen >= getSectionLength(sdtbuf) + 3) ) {
					int		seclen = getSectionLength(sdtbuf);
					int		crc = calc_crc32(sdtbuf, seclen + 3);
					if( (crc == 0) && (sdtbuf[0] == 0x42) ) {
						int		j = 0x0B;
						while(j < seclen) {
							int		serviceID = sdtbuf[j] * 256 + sdtbuf[j + 1];
							int		slen = getLength(sdtbuf+ j + 0x03);
							if(serviceID == serviceid) {
								memcpy(psibuf, sdtbuf, seclen + 3);
								bSdtFound = TRUE;
								break;
							}
							j += (slen + 5);
						}
						if(bSdtFound) break;
					}
					bSdtStarted = FALSE;
				}

				if(bSdtFound) break;

				i += len;
			}
		}

		if(bSdtFound) break;
	}

	return bSdtFound;
}


void mjd_dec(int mjd, int *recyear, int *recmonth, int *recday)
{
	int		year, year2, month, month2, day;

	year = (int)(((double)mjd - 15078.2) / 365.25);
	year2 = (int)((double)year  * 365.25);
	month = (int)(((double)mjd - 14956.1 - (double)year2) / 30.6001);
	month2 = (int)((double)month * 30.6001);
	day = mjd - 14956 - year2 - month2;

	if( (month == 14) || (month == 15) ) {
		year += 1901;
		month = month - 13;
	} else {
		year += 1900;
		month = month - 1;
	}

	*recyear = year;
	*recmonth = month;
	*recday = day;

	return;
}


int comparefornidtable(const void *item1, const void *item2)
{
	return *(int*)item1 - *(int*)item2;
}


int getTbChannelNum(int networkID, int serviceID, int remoconkey_id)
{
	static int	nIDTable[] = 
	{
		0x7FE0, 1,	0x7FE1, 2,	0x7FE2, 4,	0x7FE3, 6,	0x7FE4, 8,	0x7FE5, 5,	0x7FE6, 7,	0x7FE8, 12,		// �֓��L��
		0x7FD1, 2,	0x7FD2, 4,	0x7FD3, 6,	0x7FD4, 8,	0x7FD5, 10,											// �ߋE�L��
		0x7FC1, 2,	0x7FC2, 1,	0x7FC3, 5,	0x7FC4, 6,	0x7FC5, 4,											// �����L��			18��

		0x7FB2, 1,	0x7FB3, 5,	0x7FB4, 6,	0x7FB5, 8,	0x7FB6, 7,											// �k�C����
		0x7FA2, 4,	0x7FA3, 5,	0x7FA4, 6,	0x7FA5, 7,	0x7FA6, 8,											// ���R����
		0x7F92, 8,	0x7F93, 6,	0x7F94, 1,																	// ��������
		0x7F50, 3,	0x7F51, 2,	0x7F52, 1,	0x7F53, 5,	0x7F54, 6,	0x7F55, 8,	0x7F56, 7,					// �k�C���i�D�y�j	20��

		0x7F40, 3,	0x7F41, 2,	0x7F42, 1,	0x7F43, 5,	0x7F44, 6,	0x7F45, 8,	0x7F46, 7,					// �k�C���i���فj
		0x7F30, 3,	0x7F31, 2,	0x7F32, 1,	0x7F33, 5,	0x7F34, 6,	0x7F35, 8,	0x7F36, 7,					// �k�C���i����j
		0x7F20, 3,	0x7F21, 2,	0x7F22, 1,	0x7F23, 5,	0x7F24, 6,	0x7F25, 8,	0x7F26, 7,					// �k�C���i�эL�j	21��

		0x7F10, 3,	0x7F11, 2,	0x7F12, 1,	0x7F13, 5,	0x7F14, 6,	0x7F15, 8,	0x7F16, 7,					// �k�C���i���H�j
		0x7F00, 3,	0x7F01, 2,	0x7F02, 1,	0x7F03, 5,	0x7F04, 6,	0x7F05, 8,	0x7F06, 7,					// �k�C���i�k���j
		0x7EF0, 3,	0x7EF1, 2,	0x7EF2, 1,	0x7EF3, 5,	0x7EF4, 6,	0x7EF5, 8,	0x7EF6, 7,					// �k�C���i�����j
		0x7EE0, 3,	0x7EE1, 2,	0x7EE2, 1,	0x7EE3, 8,	0x7EE4, 4,	0x7EE5, 5,								// �{��				27��

		0x7ED0, 1,	0x7ED1, 2,	0x7ED2, 4,	0x7ED3, 8,	0x7ED4, 5,											// �H�c
		0x7EC0, 1,	0x7EC1, 2,	0x7EC2, 4,	0x7EC3, 5,	0x7EC4, 6,	0x7EC5, 8,								// �R�`
		0x7EB0, 1,	0x7EB1, 2,	0x7EB2, 6,	0x7EB3, 4,	0x7EB4, 8,	0x7EB5, 5,								// ���
		0x7EA0, 1,	0x7EA1, 2,	0x7EA2, 8,	0x7EA3, 4,	0x7EA4, 5,	0x7EA5, 6,								// ����				23��

		0x7E90, 3,	0x7E91, 2,	0x7E92, 1,	0x7E93, 6,	0x7E94, 5,											// �X
		0x7E87, 9,																							// ����
		0x7E77, 3,																							// �_�ސ�
		0x7E67, 3,																							// �Q�n
		0x7E50, 1,																							// ���
		0x7E47, 3,																							// ��t
		0x7E37, 3,																							// �Ȗ�
		0x7E27, 3,																							// ���
		0x7E10, 1,	0x7E11, 2,	0x7E12, 4,	0x7E13, 5,	0x7E14, 6,	0x7E15, 8,								// ����
		0x7E00, 1,	0x7E01, 2,	0x7E02, 6,	0x7E03, 8,	0x7E04, 4,	0x7E05, 5,								// �V��
		0x7DF0, 1,	0x7DF1, 2,	0x7DF2, 4,	0x7DF3, 6,														// �R��				28��

		0x7DE0, 3,	0x7DE6, 10,																				// ���m
		0x7DD0, 1,	0x7DD1, 2,	0x7DD2, 4,	0x7DD3, 5,	0x7DD4, 6,	0x7DD5, 8,								// �ΐ�
		0x7DC0, 1,	0x7DC1, 2,	0x7DC2, 6,	0x7DC3, 8,	0x7DC4, 4,	0x7DC5, 5,								// �É�
		0x7DB0, 1,	0x7DB1, 2,	0x7DB2, 7,	0x7DB3, 8,														// ����
		0x7DA0, 3,	0x7DA1, 2,	0x7DA2, 1,	0x7DA3, 8,	0x7DA4, 6,											// �x�R
		0x7D90, 3,	0x7D96, 7,																				// �O�d
		0x7D80, 3,	0x7D86, 8,																				// ��				27��

		0x7D70, 1,	0x7D76, 7,																				// ���
		0x7D60, 1,	0x7D66, 5,																				// ���s
		0x7D50, 1,	0x7D56, 3,																				// ����
		0x7D40, 1,	0x7D46, 5,																				// �a�̎R
		0x7D30, 1,	0x7D36, 9,																				// �ޗ�
		0x7D20, 1,	0x7D26, 3,																				// ����
		0x7D10, 1,	0x7D11, 2,	0x7D12, 3,	0x7D13, 4,	0x7D14, 5,	0x7D15, 8,								// �L��
		0x7D00, 1,	0x7D01, 2,																				// ���R
		0x7CF0, 3,	0x7CF1, 2,																				// ����
		0x7CE0, 3,	0x7CE1, 2,																				// ����				24��

		0x7CD0, 1,	0x7CD1, 2,	0x7CD2, 4,	0x7CD3, 3,	0x7CD4, 5,											// �R��
		0x7CC0, 1,	0x7CC1, 2,	0x7CC2, 4,	0x7CC3, 5,	0x7CC4, 6,	0x7CC5, 8,								// ���Q
		0x7CB0, 1,	0x7CB1, 2,																				// ����
		0x7CA0, 3,	0x7CA1, 2,	0x7CA2, 1,																	// ����
		0x7C90, 1,	0x7C91, 2,	0x7C92, 4,	0x7C93, 6,	0x7C94, 8,											// ���m				21��

		0x7C80, 3,	0x7C81, 2,	0x7C82, 1,	0x7C83, 4,	0x7C84, 5,	0x7C85, 7,	0x7C86, 8,					// ����
		0x7C70, 1,	0x7C71, 2,	0x7C72, 3,	0x7C73, 8,	0x7C74, 4,	0x7C75, 5,								// �F�{
		0x7C60, 1,	0x7C61, 2,	0x7C62, 3,	0x7C63, 8,	0x7C64, 5,	0x7C65, 4,								// ����
		0x7C50, 3,	0x7C51, 2,	0x7C52, 1,	0x7C53, 8,	0x7C54, 5,	0x7C55, 4,								// ������
		0x7C40, 1,	0x7C41, 2,	0x7C42, 6,	0x7C44, 3,														// �{��				29��

		0x7C30, 1,	0x7C31, 2,	0x7C32, 3,	0x7C33, 4,	0x7C34, 5,											// �啪
		0x7C20, 1,	0x7C21, 2,	0x7C22, 3,																	// ����
		0x7C10, 1,	0x7C11, 2,	0x7C12, 3,	0x7C14, 5,	0x7C17, 8,											// ����				13��

		0x7880, 3,	0x7881, 2,																				// �����i����t���O�j	2��			�v253��
	};

	static BOOL		bTableInit = FALSE;

	if(!bTableInit) {
		qsort(nIDTable, sizeof(nIDTable) / sizeof(int) / 2, sizeof(int) * 2, comparefornidtable);
		bTableInit = TRUE;
	}

	if(remoconkey_id == 0) {
		void *result = bsearch(&networkID, nIDTable, sizeof(nIDTable) / sizeof(int) / 2, sizeof(int) * 2, comparefornidtable);
		if(result == NULL) return 0;
		remoconkey_id = *((int*)result + 1);
	}

	return remoconkey_id * 10 + (serviceID & 0x0007) + 1;
}


void parseSit(unsigned char *sitbuf, ProgInfo *proginfo, BOOL bCharSize)
{
	int		mjd;

	int		totallen = getSectionLength(sitbuf);
	int		firstlen = getLength(sitbuf + 0x08);


	// firstloop����

	int			mediaType		= MEDIATYPE_UNKNOWN;
	int			networkID		= 0;
	int			serviceID		= 0;
	int			remoconkey_id	= 0;

	int			i = 0x0A;

	while(i < 0x0A + firstlen)
	{
		switch(sitbuf[i])
		{
			case 0xC2:													// �l�b�g���[�N���ʋL�q�q
				mediaType = sitbuf[i + 5] * 256 + sitbuf[i + 6];
				networkID = sitbuf[i + 7] * 256 + sitbuf[i + 8];
				i += (sitbuf[i + 1] + 2);
				break;
			case 0xCD:													// TS���L�q�q
				remoconkey_id = sitbuf[i + 2];
				i += (sitbuf[i + 1] + 2);
				break;
			default:
				i += (sitbuf[i + 1] + 2);
		}
	}


	// secondloop����

	if( (mediaType == MEDIATYPE_CS) && ( (networkID == 0x0001) || (networkID == 0x0003) ) )		// �X�J�p�[ PerfecTV & SKY �T�[�r�X
	{
		while(i < totallen - 1)
		{
			serviceID = sitbuf[i] * 256 + sitbuf[i + 1];
			int		secondlen = getLength(sitbuf + i + 2);

			i += 4;

			proginfo->chnum = serviceID & 0x03FF;

			int				j = i;
			int				pextendlen;

			unsigned char	tempbuf[CONVBUFSIZE];
			int				templen = 0;

			while(i < secondlen + j)
			{
				switch(sitbuf[i])
				{
					case 0xC3:													// �p�[�V�����g�����X�|�[�g�X�g���[���^�C���L�q�q
						mjd = sitbuf[i + 3] * 256 + sitbuf[i + 4];
						mjd_dec(mjd, &proginfo->recyear, &proginfo->recmonth, &proginfo->recday);
						proginfo->rechour	= (sitbuf[i + 5] >> 4) * 10 + (sitbuf[i + 5] & 0x0f);
						proginfo->recmin	= (sitbuf[i + 6] >> 4) * 10 + (sitbuf[i + 6] & 0x0f);
						proginfo->recsec	= (sitbuf[i + 7] >> 4) * 10 + (sitbuf[i + 7] & 0x0f);
						if( (sitbuf[i + 8] != 0xFF) && (sitbuf[i + 9] != 0xFF) && (sitbuf[i + 10] != 0xFF) ) {
							proginfo->durhour	= (sitbuf[i +  8] >> 4) * 10 + (sitbuf[i +  8] & 0x0f);
							proginfo->durmin	= (sitbuf[i +  9] >> 4) * 10 + (sitbuf[i +  9] & 0x0f);
							proginfo->dursec	= (sitbuf[i + 10] >> 4) * 10 + (sitbuf[i + 10] & 0x0f);
						}
						i += (sitbuf[i + 1] + 2);
						break;
					case 0xB2:													// �ǖ��Ɣԑg���Ɋւ���L�q�q
						if(sitbuf[i + 3] == 0x01) {
							int	chnamelen = sitbuf[i + 2];
							int	pnamelen = sitbuf[i + 3 + chnamelen];
							proginfo->chnamelen	= conv_to_unicode(proginfo->chname, CONVBUFSIZE, sitbuf + i + 4, chnamelen - 1, bCharSize);
							proginfo->pnamelen	= conv_to_unicode(proginfo->pname, CONVBUFSIZE, sitbuf + i + 5 + chnamelen, pnamelen - 1, bCharSize);

						}
						i += (sitbuf[i + 1] + 2);
						break;
					case 0x83:													// �ԑg�ڍ׏��Ɋւ���L�q�q
						pextendlen = sitbuf[i + 1];
						memcpy_s(tempbuf + templen, CONVBUFSIZE - templen, sitbuf + i + 3, pextendlen - 1);
						templen += (pextendlen - 1);
						i += (pextendlen + 2);
						break;
					default:
						i += (sitbuf[i + 1] + 2);
				}
			}

			proginfo->pextendlen = conv_to_unicode(proginfo->pextend, CONVBUFSIZE, tempbuf, templen, bCharSize);
		}
	}
	else // if( (mediaType == MEDIATYPE_BS) || (mediaType == MEDIATYPE_TB) || ( (mediaType == MEDIATYPE_CS) && (networkID == 0x000A) ) )		// �n��f�W�^��, BS�f�W�^��, (�X�J�p�[HD�H)																																	// �X�J�p�[SD�ȊO�͒n��f�W, BS�f�W�^���`���Ƃ��Ĉ����悤�ɂ��܂���
	{																																			// �X�J�p�[SD�ȊO�͒n�f�W, BS�f�W�^���Ɠ��`���̔ԑg���Ƃ��Ĉ����悤�ɂ��܂��� 
		WCHAR			wstr[CONVBUFSIZE];
		unsigned char	tempbuf[CONVBUFSIZE];

		int		pextendlen;
		int		templen;

		while(i < totallen - 1)
		{
			serviceID = sitbuf[i] * 256 + sitbuf[i + 1];
			int		secondlen = getLength(sitbuf + i + 2);

			i += 4;

			if(mediaType == MEDIATYPE_TB) {
				proginfo->chnum = getTbChannelNum(networkID, serviceID, remoconkey_id);
			} else {
				proginfo->chnum = serviceID & 0x03FF;
			}

			pextendlen = 0;
			templen = 0;

			int		j = i;
			int		k;

			int		provnamelen, chnamelen, pnamelen, pdetaillen, itemlooplen, iextlen;
			int		numgenre;

			while(i < secondlen + j)
			{
				switch(sitbuf[i])
				{
					case 0xC3:													// �p�[�V�����g�����X�|�[�g�X�g���[���^�C���L�q�q
						mjd = sitbuf[i + 3] * 256 + sitbuf[i + 4];
						mjd_dec(mjd, &proginfo->recyear, &proginfo->recmonth, &proginfo->recday);
						proginfo->rechour	= (sitbuf[i + 5] >> 4) * 10 + (sitbuf[i + 5] & 0x0f);
						proginfo->recmin	= (sitbuf[i + 6] >> 4) * 10 + (sitbuf[i + 6] & 0x0f);
						proginfo->recsec	= (sitbuf[i + 7] >> 4) * 10 + (sitbuf[i + 7] & 0x0f);
						if( (sitbuf[i + 8] != 0xFF) && (sitbuf[i + 9] != 0xFF) && (sitbuf[i + 10] != 0xFF) ) {
							proginfo->durhour	= (sitbuf[i +  8] >> 4) * 10 + (sitbuf[i +  8] & 0x0f);
							proginfo->durmin	= (sitbuf[i +  9] >> 4) * 10 + (sitbuf[i +  9] & 0x0f);
							proginfo->dursec	= (sitbuf[i + 10] >> 4) * 10 + (sitbuf[i + 10] & 0x0f);
						}
						i += (sitbuf[i + 1] + 2);
						break;
					case 0x54:													// �R���e���g�L�q�q
						numgenre = 0;
						for(unsigned int n = 0; n < sitbuf[i + 1]; n += 2) {
							if( (sitbuf[i + 2 + n] & 0xF0) != 0xE0 ) {
								proginfo->genretype[numgenre] = 0x01;
								proginfo->genre[numgenre] = sitbuf[i + 2 + n];
								numgenre++;
							}
						}
						i += (sitbuf[i + 1] + 2);
						break;
					case 0x48:													// �T�[�r�X�L�q�q
						provnamelen	= sitbuf[i + 3];
						chnamelen	= sitbuf[i + 4 + provnamelen];
						proginfo->chnamelen = conv_to_unicode(proginfo->chname, CONVBUFSIZE, sitbuf + i + 5 + provnamelen, chnamelen, bCharSize);
						i += (sitbuf[i + 1] + 2);
						break;
					case 0x4D:													// �Z�`���C�x���g�L�q�q
						pnamelen   = sitbuf[i + 5];
						pdetaillen = sitbuf[i + 6 + pnamelen];
						proginfo->pnamelen		= conv_to_unicode(proginfo->pname, CONVBUFSIZE, sitbuf + i + 6, pnamelen, bCharSize);
						proginfo->pdetaillen	= conv_to_unicode(proginfo->pdetail, CONVBUFSIZE, sitbuf + i + 7 + pnamelen, pdetaillen, bCharSize);
						i += (sitbuf[i + 1] + 2);
						break;
					case 0x4E:													// �g���`���C�x���g�L�q�q
						itemlooplen = sitbuf[ i + 6];
						i += 7;
						k = i;
						while( i < itemlooplen + k)
						{
							int		idesclen = sitbuf[i];
							int		itemlen = sitbuf[i + 1 + idesclen];
							
							if(idesclen != 0) {
								if(pextendlen != 0) {
									if(templen != 0) {
										pextendlen += conv_to_unicode(wstr + pextendlen, CONVBUFSIZE - pextendlen, tempbuf, templen, bCharSize);
										templen = 0;
									}
									wstr[pextendlen++] = 0x000D;
									wstr[pextendlen++] = 0x000A;
									wstr[pextendlen++] = 0x000D;
									wstr[pextendlen++] = 0x000A;
								}
								pextendlen += conv_to_unicode(wstr + pextendlen, CONVBUFSIZE - pextendlen, sitbuf + i + 1, idesclen, bCharSize);
								wstr[pextendlen++] = 0x000D;
								wstr[pextendlen++] = 0x000A;
							}
							memcpy_s(tempbuf + templen, CONVBUFSIZE - templen, sitbuf + i + 2 + idesclen, itemlen);
							templen += itemlen;
							
							i += (idesclen + itemlen + 2);
						}
						iextlen = sitbuf[i];
						if(iextlen != 0) {
							if(templen != 0) {
								pextendlen += conv_to_unicode(wstr + pextendlen, CONVBUFSIZE - pextendlen, tempbuf, templen, bCharSize);
								templen = 0;
							}
							wstr[pextendlen++] = 0x0D;
							wstr[pextendlen++] = 0x0A;
							wstr[pextendlen++] = 0x0D;
							wstr[pextendlen++] = 0x0A;
							pextendlen += conv_to_unicode(wstr + pextendlen, CONVBUFSIZE - pextendlen, sitbuf + i + 1, iextlen, bCharSize);
						}
						i += (iextlen + 1);
						break;
					default:
						i += (sitbuf[i + 1] + 2);
				}
			}
		}

		if(templen != 0) {
			pextendlen += conv_to_unicode(wstr + pextendlen, CONVBUFSIZE - pextendlen, tempbuf, templen, bCharSize);
		}
		
		wmemcpy_s(proginfo->pextend, CONVBUFSIZE, wstr, pextendlen);
		proginfo->pextendlen = pextendlen;
	}
//	else			// MEDIATYPE_UNKNOWN, �s����MEDIATYPE_CS
//	{
//		serviceID = sitbuf[i] * 256 + sitbuf[i + 1];
//
//		proginfo->chnum = serviceID & 0x03FF;

//	}

	return;
}


int parseEit(unsigned char *eitbuf, ProgInfo *proginfo, BOOL bCharSize)
{
	int 	totallen	= getSectionLength(eitbuf);
	int		serviceID	= eitbuf[0x03] * 256 + eitbuf[0x04];
	int		networkID	= eitbuf[0x0A] * 256 + eitbuf[0x0B];

	if( (networkID >= 0x7880) && (networkID <= 0x7FE8) ) {
		proginfo->chnum = getTbChannelNum(networkID, serviceID, 0);
	} else {
		proginfo->chnum = serviceID & 0x03FF;
	}


	// totalloop����

	int		i = 0x0E;

	int		pnamelen, pdetaillen, pextendlen, templen;
	int		itemlooplen, iextlen;

	WCHAR			wstr[CONVBUFSIZE];
	unsigned char	tempbuf[CONVBUFSIZE];

	while(i < totallen - 1)
	{
		int		mjd = eitbuf[i + 2] * 256 + eitbuf[i + 3];
		mjd_dec(mjd, &proginfo->recyear, &proginfo->recmonth, &proginfo->recday);
		proginfo->rechour	= (eitbuf[i + 4] >> 4) * 10 + (eitbuf[i + 4] & 0x0f);
		proginfo->recmin	= (eitbuf[i + 5] >> 4) * 10 + (eitbuf[i + 5] & 0x0f);
		proginfo->recsec	= (eitbuf[i + 6] >> 4) * 10 + (eitbuf[i + 6] & 0x0f);
		if( (eitbuf[i + 7] != 0xFF) && (eitbuf[i + 8] != 0xFF) && (eitbuf[i + 9] != 0xFF) ) {
			proginfo->durhour	= (eitbuf[i + 7] >> 4) * 10 + (eitbuf[i + 7] & 0x0f);
			proginfo->durmin	= (eitbuf[i + 8] >> 4) * 10 + (eitbuf[i + 8] & 0x0f);
			proginfo->dursec	= (eitbuf[i + 9] >> 4) * 10 + (eitbuf[i + 9] & 0x0f);
		}

		int		seclen = getLength(eitbuf + i + 0x0A);

		i += 0x0C;

	// secondloop����

		pextendlen = 0;
		templen = 0;
		int		j = i;
		int		k;
		int		numgenre;

		while(i < seclen + j)
		{
			switch(eitbuf[i])
			{
				case 0x54:													// �R���e���g�L�q�q
					numgenre = 0;
					for(unsigned int n = 0; n < eitbuf[i + 1]; n += 2) {
						if( (eitbuf[i + 2 + n] & 0xF0) != 0xE0 ) {
							proginfo->genretype[numgenre] = 0x01;
							proginfo->genre[numgenre] = eitbuf[i + 2 + n];
							numgenre++;
						}
					}
					i += (eitbuf[i + 1] + 2);
					break;
				case 0x4D:													// �Z�`���C�x���g�L�q�q
					pnamelen   = eitbuf[i + 5];
					pdetaillen = eitbuf[i + 6 + pnamelen];
					proginfo->pnamelen		= conv_to_unicode(proginfo->pname, CONVBUFSIZE, eitbuf + i + 6, pnamelen, bCharSize);
					proginfo->pdetaillen	= conv_to_unicode(proginfo->pdetail, CONVBUFSIZE, eitbuf + i + 7 + pnamelen, pdetaillen, bCharSize);
					i += (eitbuf[i + 1] + 2);
					break;
				case 0x4E:													// �g���`���C�x���g�L�q�q
					itemlooplen = eitbuf[ i + 6];
					i += 7;
					k = i;
					while( i < itemlooplen + k)
					{
						int		idesclen = eitbuf[i];
						int		itemlen = eitbuf[i + 1 + idesclen];
						
						if(idesclen != 0) {
							if(pextendlen != 0) {
								if(templen != 0) {
									pextendlen += conv_to_unicode(wstr + pextendlen, CONVBUFSIZE - pextendlen, tempbuf, templen, bCharSize);
									templen = 0;
								}
								wstr[pextendlen++] = 0x000D;
								wstr[pextendlen++] = 0x000A;
								wstr[pextendlen++] = 0x000D;
								wstr[pextendlen++] = 0x000A;
							}
							pextendlen += conv_to_unicode(wstr + pextendlen, CONVBUFSIZE - pextendlen, eitbuf + i + 1, idesclen, bCharSize);
							wstr[pextendlen++] = 0x000D;
							wstr[pextendlen++] = 0x000A;
						}
						memcpy_s(tempbuf + templen, CONVBUFSIZE - templen, eitbuf + i + 2 + idesclen, itemlen);
						templen += itemlen;
						
						i += (idesclen + itemlen + 2);
					}
					iextlen = eitbuf[i];
					if(iextlen != 0) {
						if(templen != 0) {
							pextendlen += conv_to_unicode(wstr + pextendlen, CONVBUFSIZE - pextendlen, tempbuf, templen, bCharSize);
							templen = 0;
						}
						wstr[pextendlen++] = 0x0D;
						wstr[pextendlen++] = 0x0A;
						wstr[pextendlen++] = 0x0D;
						wstr[pextendlen++] = 0x0A;
						pextendlen += conv_to_unicode(wstr + pextendlen, CONVBUFSIZE - pextendlen, eitbuf + i + 1, iextlen, bCharSize);
					}
					i += (iextlen + 1);
					break;
				default:
					i += (eitbuf[i + 1] + 2);
			}
		}

		if(templen != 0) {
			pextendlen += conv_to_unicode(wstr + pextendlen, CONVBUFSIZE - pextendlen, tempbuf, templen, bCharSize);
		}
	}

	wmemcpy_s(proginfo->pextend, CONVBUFSIZE, wstr, pextendlen);
	proginfo->pextendlen = pextendlen;

	return serviceID;
}


void parseSdt(unsigned char *sdtbuf, ProgInfo *proginfo, int serviceid, BOOL bCharSize)
{
	int		provnamelen, servnamelen;

	int		totallen = getSectionLength(sdtbuf);

	int		i = 0x0B;

	while(i < totallen)
	{
		int		serviceID = sdtbuf[i] * 256 + sdtbuf[i + 1];
		int		secondlen = getLength(sdtbuf+ i + 3);

		if(serviceID == serviceid) {
			i += 5;
			int		j = i;
			while( i < secondlen + j)
			{
				switch(sdtbuf[i])
				{
					case 0x48:													// �T�[�r�X�L�q�q
						provnamelen = sdtbuf[i + 3];
						servnamelen = sdtbuf[i + 4 + provnamelen];
						proginfo->chnamelen = conv_to_unicode(proginfo->chname, CONVBUFSIZE, sdtbuf + i + 5 + provnamelen, servnamelen, bCharSize);
						i += (sdtbuf[i + 1] + 2);
						break;
					default:
						i += (sdtbuf[i + 1] + 2);
				}
			}
			break;
		} else {
			i += (secondlen + 5);
		}
	}

	return;
}

