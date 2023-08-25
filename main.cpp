#include<Windows.h>
#include<tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include<vector>
#include<string>

#include <d3dcompiler.h>
#ifdef _DEBUG
#include<iostream>
#endif

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

const unsigned int WINDOW_WIDTH = 1280;
const unsigned int WINDOW_HEIGHT = 720;

ID3D12Device* gDev = nullptr;
IDXGIFactory6* gDxgiFactory = nullptr;
IDXGISwapChain4* gSwapChain = nullptr;
ID3D12CommandAllocator* gCmdAllocator = nullptr;
ID3D12GraphicsCommandList* gCmdList = nullptr;
ID3D12CommandQueue* gCmdQueue = nullptr;

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

void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))){
		debugLayer->EnableDebugLayer();	// デバッグレイヤーを有効化
		debugLayer->Release();			// 有効化したらインターフェースを解放
	}
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

#ifdef _DEBUG
	// デバッグレイヤーをON
	EnableDebugLayer();
#endif

	//-----------------------------------------
	// DirectX12周りの初期化

	// フィーチャーレベルの列挙
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&gDxgiFactory));

	// 利用可能なアダプターを列挙
	std::vector<IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for(int i = 0; gDxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i){
		adapters.emplace_back(tmpAdapter);
	}
	for(auto adpt : adapters){
		DXGI_ADAPTER_DESC adptDesc = {};
		adpt->GetDesc(&adptDesc);
		std::wstring strDesc = adptDesc.Description;
		if(strDesc.find(L"NVIDIA") != std::string::npos){
			tmpAdapter = adpt;
			break;
		}
	}

	// Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for(auto level : levels){
		if(D3D12CreateDevice(nullptr, level, IID_PPV_ARGS(&gDev)) == S_OK){
			featureLevel = level;
			break;
		}
	}

	result = gDev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gCmdAllocator));
	result = gDev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gCmdAllocator, nullptr, IID_PPV_ARGS(&gCmdList));

	// キュー作成
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;			 // タイムアウトなし
	cmdQueueDesc.NodeMask = 0;									 // アダプターを1つしか使用しない場合0でよい
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // プライオリティは特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;			 // コマンドリストと合わせる
	result = gDev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&gCmdQueue));

	// スワップチェーン生成
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = WINDOW_WIDTH;
	swapchainDesc.Height = WINDOW_HEIGHT;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;					// バックバッファーは伸び縮み可能
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// フリップ後は速やかに破棄
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;			// 特に指定なし
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// ウィンド⇔フルスクリーン切替可能
	
	result = gDxgiFactory->CreateSwapChainForHwnd(gCmdQueue,
												  hwnd,
												  &swapchainDesc,
												  nullptr,
												  nullptr,
												  (IDXGISwapChain1**)&gSwapChain);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// レンダーターゲットビューなのでRTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;						// 表裏の2つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// 特になし

	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = gDev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = gSwapChain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> backBuffers(swcDesc.BufferCount);

	// 先頭のアドレスを得る
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for(int idx = 0; idx < swcDesc.BufferCount; ++idx){
		result = gSwapChain->GetBuffer(idx, IID_PPV_ARGS(&backBuffers[idx]));
		gDev->CreateRenderTargetView(backBuffers[idx], nullptr, handle);
		handle.ptr += gDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// フェンスのオブジェクト生成
	ID3D12Fence* fence = nullptr;
	UINT64 fenceValue = 0;
	result = gDev->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	// ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	DirectX::XMFLOAT3 vertices[] = {
		{-1.0f, -1.0f, 0.0f},	// 左下
		{-1.0f,  1.0f, 0.0f},	// 左上
		{ 1.0f, -1.0f, 0.0f}	// 右下
	};

	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeof(vertices);
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* vertBuff = nullptr;
	result = gDev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	DirectX::XMFLOAT3* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);

	std::copy(std::begin(vertices), std::end(vertices), vertMap);

	vertBuff->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress(); // バッファーの仮想アドレス
	vbView.SizeInBytes = sizeof(vertices);					  // 全バイト数
	vbView.StrideInBytes = sizeof(vertices[0]);				  // 1頂点あたりのバイト数

	ID3DBlob* vsBlob = nullptr;
	ID3DBlob* psBlob = nullptr;
	ID3DBlob* errBlob = nullptr;

	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &vsBlob, &errBlob);
	if(FAILED(result)){
		if(result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)){
			::OutputDebugStringA("ファイルが見当たりません");
		}else{
			std::string errStr;

			// 必要なサイズを確保
			errStr.resize(errBlob->GetBufferSize());
			std::copy_n((char*)errBlob->GetBufferPointer(), errBlob->GetBufferSize(), errStr.begin());
			errStr += "\n";
			OutputDebugStringA(errStr.c_str());
		}
		exit(1);
	}

	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &psBlob, &errBlob);
	if(FAILED(result)){
		if(result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)){
			::OutputDebugStringA("ファイルが見当たりません");
		}else{
			std::string errStr;
			errStr.resize(errBlob->GetBufferSize());
			std::copy_n((char*)errBlob->GetBufferPointer(), errBlob->GetBufferSize(), errStr.begin());
			errStr += "\n";
			OutputDebugStringA(errStr.c_str());
		}
		exit(1);
	}

	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// グラフィックスパイプライン
	D3D12_GRAPHICS_PIPELINE_STATE_DESC grPipeline = {};
	grPipeline.pRootSignature = nullptr;
	grPipeline.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	grPipeline.VS.BytecodeLength = vsBlob->GetBufferSize();
	grPipeline.PS.pShaderBytecode = psBlob->GetBufferPointer();
	grPipeline.PS.BytecodeLength = psBlob->GetBufferSize();

	// デフォルトのサンプルマスクを表す定数(0xffffffff)
	grPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	grPipeline.BlendState.AlphaToCoverageEnable = false;
	grPipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

	// ひとまず加算や乗算やαブレンディングは使用しない
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// ひとまず論理演算は使用しない
	renderTargetBlendDesc.LogicOpEnable = false;

	grPipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	grPipeline.RasterizerState.MultisampleEnable = false;		 // 一旦false
	grPipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  // カリングしない
	grPipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // 中身を塗りつぶす
	grPipeline.RasterizerState.DepthClipEnable = true;			 // 深度方向のクリッピングは有効

	grPipeline.InputLayout.pInputElementDescs = inputLayout;	 // レイアウト先頭アドレス
	grPipeline.InputLayout.NumElements = _countof(inputLayout);	 // レイアウト配列数

	grPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // ストリップ時のカットなし
	grPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 三角形で構成0

	grPipeline.NumRenderTargets = 1;						// 今は1つのみ
	grPipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	// 0～1に正規化されたRGBA

	grPipeline.SampleDesc.Count = 1;	// サンプリングは1ピクセルにつき1
	grPipeline.SampleDesc.Quality = 0;	// クオリティは最低

	// ルートシグネチャの作成
	ID3D12RootSignature* rootSignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* rootSigBlob  = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errBlob);
	result = gDev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	rootSigBlob->Release();

	grPipeline.pRootSignature = rootSignature;
	ID3D12PipelineState* piplineState = nullptr;
	result = gDev->CreateGraphicsPipelineState(&grPipeline, IID_PPV_ARGS(&piplineState));

	D3D12_VIEWPORT viewport = {};
	viewport.Width = WINDOW_WIDTH;	 // 出力先の幅(ピクセル数)
	viewport.Height = WINDOW_HEIGHT; // 出力先の高さ(ピクセル数)
	viewport.TopLeftX = 0;			 // 出力先の左上座標X
	viewport.TopLeftY = 0;			 // 出力先の左上座標Y
	viewport.MaxDepth = 1.0f;		 // 深度最大値
	viewport.MinDepth = 0.0f;		 // 深度最小値

	D3D12_RECT scissorRect = {};
	scissorRect.top = 0;								  // 切り抜き上座標
	scissorRect.left = 0;								  // 切り抜き左座標
	scissorRect.right = scissorRect.left + WINDOW_WIDTH;  // 切り抜き右座標
	scissorRect.bottom = scissorRect.top + WINDOW_HEIGHT; // 切り抜き下座標

	MSG msg = {};

	while(true)
	{
		if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// アプリケーションが終わるときにmessageがWN_QUITになる
		if(msg.message == WM_QUIT){ break; }

		// バックバッファーのインデックスを取得
		auto bbIdx = gSwapChain->GetCurrentBackBufferIndex();

		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrierDesc.Transition.pResource = backBuffers[bbIdx];
		barrierDesc.Transition.Subresource = 0;
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		gCmdList->ResourceBarrier(1, &barrierDesc);

		gCmdList->SetPipelineState(piplineState);

		// レンダーターゲットを指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * gDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		gCmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		// 画面クリア(黄色)
		float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
		gCmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		gCmdList->RSSetViewports(1, &viewport);
		gCmdList->RSSetScissorRects(1, &scissorRect);
		gCmdList->SetGraphicsRootSignature(rootSignature);

		gCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gCmdList->IASetVertexBuffers(0, 1, &vbView);

		gCmdList->DrawInstanced(3, 1, 0, 0);

		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		gCmdList->ResourceBarrier(1, &barrierDesc);

		// 命令のクローズ
		gCmdList->Close();

		// コマンドリストの実行
		ID3D12CommandList* cmdLists[] = { gCmdList };
		gCmdQueue->ExecuteCommandLists(1, cmdLists);

		gCmdQueue->Signal(fence, ++fenceValue);
		if(fence->GetCompletedValue() != fenceValue){
			// イベントハンドルの取得
			auto event = CreateEvent(nullptr, false, false, nullptr);
			fence->SetEventOnCompletion(fenceValue, event);

			// イベントが発生するまで待ち続ける
			WaitForSingleObject(event, INFINITE);

			// イベントハンドルを閉じる
			CloseHandle(event);
		}

		gCmdAllocator->Reset();					 // キューをクリア
		gCmdList->Reset(gCmdAllocator, nullptr); // 再びコマンドリストを貯める準備

		// フリップ
		gSwapChain->Present(1, 0);
	}

	// もうクラスは使用しないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
}