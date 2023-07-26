#include<Windows.h>
#include<tchar.h>
//#include <d3d12.h>
//#include <dxgi1_6.h>
//#include<vector>
//#include<string>
#ifdef _DEBUG
#include<iostream>
#endif

using namespace std;

const unsigned int WINDOW_WIDTH = 1280;
const unsigned int WINDOW_HEIGHT = 720;

///@brief �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
///@param format �t�H�[�}�b�g(%d�Ƃ�%f�Ƃ���)
///@param �ϒ�����
///@remarks���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�܂���
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
#endif
}

// �ʓ|�����Ǐ����Ȃ���΂����Ȃ��֐�
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// �E�B���h�E���j�����ꂽ��Ă΂��
	if(msg == WM_DESTROY){
		// OS�ɑ΂��āu�������̃A�v���͏I���v�Ɠ`����
		PostQuitMessage(0);
	}

	// �K��̏������s��
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

#ifdef _DEBUG
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	DebugOutputFormatString("Show window test.");

	// �E�B���h�E�N���X�̐������o�^
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	// �R�[���o�b�N�֐��̎w��
	w.lpszClassName = _T("DX12Sample");			// �A�v���P�[�V�����N���X��
	w.hInstance = GetModuleHandle(nullptr);		// �n���h���̎擾

	// �A�v���P�[�V�����N���X�i�E�B���h�E�N���X�̎w���OS�ɓ`����j
	RegisterClassEx(&w);

	// �E�B���h�E�T�C�Y�����߂�
	// �֐����g���ăE�B���h�E�T�C�Y��␳����
	RECT wrc = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E�I�u�W�F�N�g�̐���
	HWND hwnd = CreateWindow(w.lpszClassName,		// �N���X���w��
							 _T("DX12�e�X�g"),		// �^�C�g���o�[�̕���
							 WS_OVERLAPPEDWINDOW,	// �^�C�g���o�[�Ƌ��E��������E�B���h�E
							 CW_USEDEFAULT,			// �\��x���W��OS�ɂ��C��
							 CW_USEDEFAULT,			// �\��y���W��OS�ɂ��C��
							 wrc.right - wrc.left,	// �E�B���h�E��
							 wrc.bottom - wrc.top,	// �E�B���h�E����
							 nullptr,				// �e�̃E�B���h�E�n���h��
							 nullptr,				// ���j���[�n���h��
							 w.hInstance,			// �Ăяo���A�v���P�[�V�����n���h��
							 nullptr);				// �ǉ��p�����[�^

	// �E�B���h�E�\��
	ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};

	while(true)
	{
		if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// �A�v���P�[�V�������I���Ƃ���message��WN_QUIT�ɂȂ�
		if(msg.message == WM_QUIT){ break; }
	}

	// �����N���X�͎g�p���Ȃ��̂œo�^��������
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
}