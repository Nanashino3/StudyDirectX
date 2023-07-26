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

///@brief コンソール画面にフォーマット付き文字列を表示
///@param format フォーマット(%dとか%fとかの)
///@param 可変長引数
///@remarksこの関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);
	va_end(valist);
#endif
}

// 面倒だけど書かなければいけない関数
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ウィンドウが破棄されたら呼ばれる
	if(msg == WM_DESTROY){
		// OSに対して「もうこのアプリは終わる」と伝える
		PostQuitMessage(0);
	}

	// 規定の処理を行う
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

	// ウィンドウクラスの生成＆登録
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	// コールバック関数の指定
	w.lpszClassName = _T("DX12Sample");			// アプリケーションクラス名
	w.hInstance = GetModuleHandle(nullptr);		// ハンドルの取得

	// アプリケーションクラス（ウィンドウクラスの指定をOSに伝える）
	RegisterClassEx(&w);

	// ウィンドウサイズを決める
	// 関数を使ってウィンドウサイズを補正する
	RECT wrc = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(w.lpszClassName,		// クラス名指定
							 _T("DX12テスト"),		// タイトルバーの文字
							 WS_OVERLAPPEDWINDOW,	// タイトルバーと境界線があるウィンドウ
							 CW_USEDEFAULT,			// 表示x座標はOSにお任せ
							 CW_USEDEFAULT,			// 表示y座標はOSにお任せ
							 wrc.right - wrc.left,	// ウィンドウ幅
							 wrc.bottom - wrc.top,	// ウィンドウ高さ
							 nullptr,				// 親のウィンドウハンドル
							 nullptr,				// メニューハンドル
							 w.hInstance,			// 呼び出しアプリケーションハンドル
							 nullptr);				// 追加パラメータ

	// ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};

	while(true)
	{
		if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// アプリケーションが終わるときにmessageがWN_QUITになる
		if(msg.message == WM_QUIT){ break; }
	}

	// もうクラスは使用しないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
}