// �ȉ��� ifdef �u���b�N�� DLL ����̃G�N�X�|�[�g��e�Ղɂ���}�N�����쐬���邽�߂� 
// ��ʓI�ȕ��@�ł��B���� DLL ���̂��ׂẴt�@�C���́A�R�}���h ���C���Œ�`���ꂽ RPLSINFODLL_EXPORTS
// �V���{�����g�p���ăR���p�C������܂��B���̃V���{���́A���� DLL ���g�p����v���W�F�N�g�ł͒�`�ł��܂���B
// �\�[�X�t�@�C�������̃t�@�C�����܂�ł��鑼�̃v���W�F�N�g�́A 
// RPLSINFODLL_API �֐��� DLL ����C���|�[�g���ꂽ�ƌ��Ȃ��̂ɑ΂��A���� DLL �́A���̃}�N���Œ�`���ꂽ
// �V���{�����G�N�X�|�[�g���ꂽ�ƌ��Ȃ��܂��B
#ifdef RPLSINFODLL_EXPORTS
#define RPLSINFODLL_API __declspec(dllexport)
#else
#define RPLSINFODLL_API __declspec(dllimport)
#endif

//// ���̃N���X�� rplsinfoDLL.dll ����G�N�X�|�[�g����܂����B
//class RPLSINFODLL_API CrplsinfoDLL {
//public:
//	CrplsinfoDLL(void);
//	// TODO: ���\�b�h�������ɒǉ����Ă��������B
//};
//
//extern RPLSINFODLL_API int nrplsinfoDLL;
//
//RPLSINFODLL_API int fnrplsinfoDLL(void);

#ifdef  __cplusplus
extern "C" {
#endif

RPLSINFODLL_API LPCWSTR __stdcall version();
RPLSINFODLL_API int __stdcall rplsRead(LPCWSTR filename);
RPLSINFODLL_API int __stdcall recyear();
RPLSINFODLL_API int __stdcall recmonth();
RPLSINFODLL_API int __stdcall recday();
RPLSINFODLL_API int __stdcall rechour();
RPLSINFODLL_API int __stdcall recmin();
RPLSINFODLL_API int __stdcall recsec();
RPLSINFODLL_API int __stdcall durhour();
RPLSINFODLL_API int __stdcall durmin();
RPLSINFODLL_API int __stdcall dursec();
RPLSINFODLL_API int __stdcall rectimezone();
RPLSINFODLL_API int __stdcall makerid();
RPLSINFODLL_API int __stdcall modelcode();
RPLSINFODLL_API int __stdcall recsrc();
RPLSINFODLL_API int __stdcall chnum();
RPLSINFODLL_API int __stdcall chnamelen();
RPLSINFODLL_API int __stdcall pnamelen();
RPLSINFODLL_API int __stdcall pdetaillen();
RPLSINFODLL_API int __stdcall pextendlen();
RPLSINFODLL_API LPCWSTR __stdcall chname();
RPLSINFODLL_API LPCWSTR __stdcall pname();
RPLSINFODLL_API LPCWSTR __stdcall pdetail();
RPLSINFODLL_API LPCWSTR __stdcall pextend();
RPLSINFODLL_API int __stdcall genre(const int index);
RPLSINFODLL_API int __stdcall genretype();

RPLSINFODLL_API int __stdcall conv_from(const WCHAR *sbuf, const int total_length, const BOOL bCharSize);
RPLSINFODLL_API int __stdcall getconv(const int index);

#ifdef  __cplusplus
}
#endif
