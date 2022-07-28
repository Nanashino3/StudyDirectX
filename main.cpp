// Windowsプログラミング

#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <string>

#include <d3dcompiler.h>
#ifdef _DEBUG
#include <iostream>
#endif

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

const unsigned int WINDOW_WIDTH = 1280;
const unsigned int WINDOW_HEIGHT = 720;

ID3D12Device* g_dev = nullptr;
IDXGIFactory6* g_dxgiFactory = nullptr;
IDXGISwapChain4* g_swapChain = nullptr;
ID3D12CommandAllocator* g_cmdAllocator = nullptr;
ID3D12GraphicsCommandList* g_cmdList = nullptr;
ID3D12CommandQueue* g_cmdQueue = nullptr;

// コンソール画面にフォーマット付き文字列を表示
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list vaList;
	va_start(vaList, format);
	vprintf(format, vaList);
	va_end(vaList);
#endif
}

// デバッグレイヤーの有効化
void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))) {
		debugLayer->EnableDebugLayer();	// デバッグレイヤーの有効化
		debugLayer->Release();			// 有効化したらインタフェースを開放
	}
}

// 面倒だけど必須関数
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);	// OSに対してアプリの終了を伝える
		return 0;
	}

	// 規定の処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

#ifdef _DEBUG
int main()
{
#else
int WINMAIN WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	DebugOutputFormatString("Show window test.");
	HINSTANCE hInst = GetModuleHandle(nullptr);

	// ウィンドウクラスの生成と登録
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	// コールバック関数の指定
	w.lpszClassName = _T("DX12Sample");			// アプリケーションクラス名
	w.hInstance = GetModuleHandle(nullptr);		// ハンドルの取得

	// アプリケーションクラス(OSにウィンドウクラスの指定を伝える)
	RegisterClassEx(&w);

	// ウィンドウサイズを決める
	RECT wrc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };

	// 関数を利用しウィンドウサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(w.lpszClassName,					// ウィンドウクラス名指定
							 _T("DX12 単純ポリゴンテスト"),		// タイトルバーの文字
							 WS_OVERLAPPEDWINDOW,				// タイトルバーと境界線があるウィンドウ
							 CW_USEDEFAULT,						// 表示X座標はOSにお任せ
							 CW_USEDEFAULT,						// 表示Y座標はOSにお任せ
							 wrc.right - wrc.left,				// ウィンドウ幅
							 wrc.bottom - wrc.top,				// ウィンドウ高さ
							 nullptr,							// 親ウィンドウハンドル
							 nullptr,							// メニューハンドル
							 w.hInstance,						// 呼び出しアプリケーションハンドル
							 nullptr);							// 追加パラメータ
							 
#ifdef _DEBUG
	// デバイス生成後にやるとデバイスがロストしてしまうので注意
	EnableDebugLayer();
#endif

	/*DirectX12まわり初期化*/
	// フィーチャレベル列挙
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	// アダプターを列挙するためにDXGIFactoryオブジェクトを生成する
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&g_dxgiFactory));
	std::vector<IDXGIAdapter*> adapters;	// アダプターの列挙用

	// ここに特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; g_dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapters.push_back(tmpAdapter);
	}
	for (auto adapter : adapters) {
		DXGI_ADAPTER_DESC adsec = {};
		adapter->GetDesc(&adsec);
		std::wstring strDsec = adsec.Description;
		if (strDsec.find(L"NVIDIA") != std::string::npos) {
			tmpAdapter = adapter;
			break;
		}
	}

	// Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels) {
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&g_dev)) == S_OK) {
			featureLevel = lv;
			break;
		}
	}

	// コマンドアロケータの生成
	result = g_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_cmdAllocator));
	// コマンドリストの生成
	result = g_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_cmdAllocator, nullptr, IID_PPV_ARGS(&g_cmdList));

	//***********************************************
	// コマンドキューの実態生成と設定
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;				// タイムアウトなし
	cmdQueueDesc.NodeMask = 0;										// アダプターを一つしか使用しないときは0
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// 優先度は特に指定なし
	// コマンドリストと合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	// キュー生成
	result = g_dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&g_cmdQueue));
	//***********************************************

	//***********************************************
	// スワップチェーン生成
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = WINDOW_WIDTH;
	swapChainDesc.Height = WINDOW_HEIGHT;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;					// バックバッファーは伸び縮み可能
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// フリップ後は速やかに破棄
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;			// 特に指定なし
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// ウィンドウ⇔フルスクリーン切替可能

	result = g_dxgiFactory->CreateSwapChainForHwnd(g_cmdQueue,
												   hwnd,
												   &swapChainDesc,
												   nullptr,
												   nullptr,
												   (IDXGISwapChain1**)&g_swapChain);
	//***********************************************

	//***********************************************
	// ディスクリプタヒープ生成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// RTV => レンダーターゲットビュー
	heapDesc.NodeMask = 0;								// 複数のGPUを識別するためのビットマスク
	heapDesc.NumDescriptors = 2;						// 表裏の2つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// 特に指定なし

	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = g_dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	// ディスクリプタとバックバッファーの関連付け
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = g_swapChain->GetDesc(&swcDesc);

	std::vector<ID3D12Resource*> backBuffers(swcDesc.BufferCount);
	// 先頭アドレスを取得
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for (size_t index = 0; index < swcDesc.BufferCount; ++index) {
		// スワップチェーンの中のメモリを取得
		result = g_swapChain->GetBuffer(static_cast<UINT>(index), IID_PPV_ARGS(&backBuffers[index]));
		// RTV(レンダーターゲットビュー)を生成
		g_dev->CreateRenderTargetView(backBuffers[index], nullptr, handle);
		// ポインタをずらす
		handle.ptr += g_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	//***********************************************

	// フェンスの作成
	ID3D12Fence* fence = nullptr;
	UINT64 fenceValue = 0;
	result = g_dev->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	// ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	//***********************************************
	// 頂点バッファーの生成
	struct Vertex {
		XMFLOAT3 pos;	// XYZ座標
		XMFLOAT2 uv;	// UV座標
	};

	Vertex vertices[] = {
		{{-0.4f, -0.7f, 0.0f}, {0.0f, 1.0f}},	// 左下
		{{-0.4f,  0.7f, 0.0f}, {0.0f, 0.0f}},	// 左上
		{{ 0.4f, -0.7f, 0.0f}, {1.0f, 1.0f}},	// 右下
		{{ 0.4f,  0.7f, 0.0f}, {1.0f, 0.0f}},	// 右上
	};

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;						// CPUからアクセスできる(マップあできる)
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeof(vertices);	// 頂点情報が入るだけのサイズ
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Layout  = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* vertBuff = nullptr;
	result = g_dev->CreateCommittedResource(&heapProperties, 
											D3D12_HEAP_FLAG_NONE,
											&resourceDesc,
											D3D12_RESOURCE_STATE_GENERIC_READ,
											nullptr,
											IID_PPV_ARGS(&vertBuff));
	//***********************************************

	//***********************************************
	// 頂点情報のコピー(マップ)
	Vertex* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);
	//***********************************************

	//***********************************************
	// 頂点バッファービューの作成
	D3D12_VERTEX_BUFFER_VIEW vbView = {};

	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();	// バッファーの仮想アドレス
	vbView.SizeInBytes = sizeof(vertices);						// 全バイト数
	vbView.StrideInBytes = sizeof(vertices[0]);					// 1頂点あたりのバイト数
	//***********************************************

	//***********************************************
	// インデックスの利用
	unsigned short indices[] = {0, 1, 2, 2, 1, 3};

	ID3D12Resource* idxBuff = nullptr;
	// 設定はバッファーサイズ以外、頂点バッファーの設定を使いまわして良い
	resourceDesc.Width = sizeof(indices);
	result = g_dev->CreateCommittedResource(&heapProperties,
											D3D12_HEAP_FLAG_NONE,
											&resourceDesc,
											D3D12_RESOURCE_STATE_GENERIC_READ,
											nullptr,
											IID_PPV_ARGS(&idxBuff));

	// 作ったバッファーにインデックスデータをコピー
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	// インデックスバッファビューを作成
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);

	//***********************************************

	ID3DBlob* vsBlob = nullptr;		// 頂点シェーダ
	ID3DBlob* psBlob = nullptr;		// ピクセルシェーダ
	ID3DBlob* errBlob = nullptr;	// エラー情報

	// 頂点バッファーの読み込み
	result = D3DCompileFromFile(L"Shader/BasicVertexShader.hlsl",							// シェーダ名
								nullptr,
								D3D_COMPILE_STANDARD_FILE_INCLUDE,					// インクルードはデフォルト
								"BasicVS", "vs_5_0",								// 関数はBasicVS、対象シェーダはvs_5_0
								D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	// デバック用及び最適化なし
								0,
								&vsBlob, &errBlob);

	// エラー処理
	if(FAILED(result)){
		if(result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)){
			::OutputDebugStringA("ファイルが見当たりません");
		}else{
			std::string errStr;
			errStr.resize(errBlob->GetBufferSize());
			std::copy_n((char*)errBlob->GetBufferPointer(), errBlob->GetBufferSize(), errStr.begin());
			errStr += "\n";

			::OutputDebugStringA(errStr.c_str());
		}
		exit(1);
	}

	// ピクセルシェーダの読み込み
	result = D3DCompileFromFile(L"Shader/BasicPixelShader.hlsl",
								nullptr,
								D3D_COMPILE_STANDARD_FILE_INCLUDE,
								"BasicPS", "ps_5_0",
								D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	// 関数はBasicPS、対象シェーダはps_5_0
								0,
								&psBlob, &errBlob);

	if(FAILED(result)){
		if(result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)){
			::OutputDebugStringA("ファイルが見当たりません");
		}else{
			std::string errStr;
			errStr.resize(errBlob->GetBufferSize());
			std::copy_n((char*)errBlob->GetBufferPointer(), errBlob->GetBufferSize(), errStr.begin());
			errStr += "\n";

			::OutputDebugStringA(errStr.c_str());
		}
		exit(1);
	}

	//***********************************************
	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		// 座標情報
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		// UV情報
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	//***********************************************

	//***********************************************
	// グラフィックスパイプライン
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gPipeline = {};
	gPipeline.pRootSignature = nullptr;
	gPipeline.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	gPipeline.VS.BytecodeLength  = vsBlob->GetBufferSize();
	gPipeline.PS.pShaderBytecode = psBlob->GetBufferPointer();
	gPipeline.PS.BytecodeLength	 = psBlob->GetBufferSize();

	// デフォルトのサンプルマスクを表す定数(0xffffffff)
	gPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	
	// ブレンドステートの設定
	gPipeline.BlendState.AlphaToCoverageEnable = false;
	gPipeline.BlendState.IndependentBlendEnable = false;
	
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	
	gPipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	// ラスタライザーステートの設定
	gPipeline.RasterizerState.MultisampleEnable = false;
	gPipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// カリングしない
	gPipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;	// 中身を塗りつぶす
	gPipeline.RasterizerState.DepthClipEnable = true;			// 深度方向にクリッピングは有効に
	//残り
	gPipeline.RasterizerState.FrontCounterClockwise = false;
	gPipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gPipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gPipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gPipeline.RasterizerState.AntialiasedLineEnable = false;
	gPipeline.RasterizerState.ForcedSampleCount = 0;
	gPipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// 入力レイアウトの設定
	gPipeline.InputLayout.pInputElementDescs = inputLayout;						// レイアウト先頭アドレス
	gPipeline.InputLayout.NumElements = _countof(inputLayout);					// レイアウト配列の要素数
	gPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	// カットなし
	gPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// 三角形で構成

	// レンダーターゲットの設定
	gPipeline.NumRenderTargets = 1;							// レンダーターゲット数
	gPipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	// 0～1に正規化されたRGBA

	// アンチエイリアシング(AA)の設定
	gPipeline.SampleDesc.Count = 1;		// サンプリングは1ピクセル
	gPipeline.SampleDesc.Quality = 0;	// クオリティは最低

	// ルートシグネチャの作成(空)
	D3D12_ROOT_SIGNATURE_DESC  rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;	// 頂点情報(入力アセンブラ)がある

	// バイナリコードの作成
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc,			 // ルートシグネチャ設定
										 D3D_ROOT_SIGNATURE_VERSION_1_0, // ルートシグネチャバージョン
										 &rootSigBlob, &errBlob);

	// ルートシグネチャオブジェクトの作成
	ID3D12RootSignature* rootSignature = nullptr;
	result = g_dev->CreateRootSignature(0,
										rootSigBlob->GetBufferPointer(),
										rootSigBlob->GetBufferSize(),
										IID_PPV_ARGS(&rootSignature));
	// 不要になったので開放
	rootSigBlob->Release();
	gPipeline.pRootSignature = rootSignature;

	// グラフィックスパイプラインステートオブジェクトの生成
	ID3D12PipelineState* pipelineState = nullptr;
	result = g_dev->CreateGraphicsPipelineState(&gPipeline, IID_PPV_ARGS(&pipelineState));
	//***********************************************

	//***********************************************
	// ビューポートとシザー矩形

	// ビューポート設定
	D3D12_VIEWPORT viewPort = {};
	viewPort.Width = WINDOW_WIDTH;		// 出力先の幅(ピクセル数)
	viewPort.Height = WINDOW_HEIGHT;	// 出力先の高さ(ピクセル数)
	viewPort.TopLeftX = 0;				// 出力先の左上座標X
	viewPort.TopLeftY = 0;				// 出力先の左上座標Y
	viewPort.MaxDepth = 1.0f;			// 深度最大値
	viewPort.MinDepth = 0.0f;			// 深度最小値

	// シザー矩形設定
	D3D12_RECT scissorRect = {};
	scissorRect.top = 0;									// 切り抜き上座標
	scissorRect.left = 0;									// 切り抜き左座標
	scissorRect.right = scissorRect.left + WINDOW_WIDTH;	// 切り抜き右座標
	scissorRect.bottom = scissorRect.top + WINDOW_HEIGHT;	// 切り抜き下座標

	//***********************************************

	struct TexRGBA{
		unsigned char R, G, B, A;
	};
	std::vector<TexRGBA> textureData(256 * 256);

	for(auto& rgba : textureData){
		rgba.R = rand() % 256;
		rgba.G = rand() % 256;
		rgba.B = rand() % 256;
		rgba.A = 255;			// αは1.0とする
	}

	MSG msg = {};
	unsigned int frame = 0;
	while (1) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// アプリケーションが終了するときにmessageがWM_QUITになる
		if (msg.message == WM_QUIT) { break; }

		//***********************************************
		// DirectXの処理

		// バックバッファのIndexを取得
		auto bbIndex = g_swapChain->GetCurrentBackBufferIndex();

		// リソースバリア
		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrierDesc.Transition.pResource = backBuffers[bbIndex];
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		g_cmdList->ResourceBarrier(1, &barrierDesc);
		g_cmdList->SetPipelineState(pipelineState);		// パイプラインステートの設定

		// レンダーターゲットを指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIndex * g_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		g_cmdList->OMSetRenderTargets(1, &rtvH, false, nullptr);

		// 画面を特定色でクリア(RGBA)
		float r, g, b;
		r = (float)(0xff & frame >> 16) / 255.0f;
		g = (float)(0xff & frame >>  8) / 255.0f;
		b = (float)(0xff & frame >>  0) / 255.0f;
		float clearColor[] = { r, g, b, 1.0f };
		g_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		++frame;

		g_cmdList->RSSetViewports(1, &viewPort);								// ビューポートの設定
		g_cmdList->RSSetScissorRects(1, &scissorRect);							// シザー矩形の設定
		g_cmdList->SetGraphicsRootSignature(rootSignature);						// ルートシグネチャの設定

		g_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	// プリミティブトポロジの設定
		g_cmdList->IASetVertexBuffers(0, 1, &vbView);							// 頂点情報の設定
		g_cmdList->IASetIndexBuffer(&ibView);

		//// 第1引数：頂点数、第2引数：インスタンス数、第3引数：頂点データ、第4引数：インスタンスのオフセット
		//g_cmdList->DrawInstanced(3, 1, 0, 0);									// 描画命令
		g_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// 状態を「Present」に移行
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		g_cmdList->ResourceBarrier(1, &barrierDesc);

		// コマンドリスト実行前に命令をクローズ
		g_cmdList->Close();

		// コマンドリストの実行
		ID3D12CommandList* cmdLists[] = { g_cmdList };
		g_cmdQueue->ExecuteCommandLists(1, cmdLists);
		g_cmdQueue->Signal(fence, ++fenceValue);

		// ビジーループ(待ち状態を表現)
		if (fence->GetCompletedValue() != fenceValue) {
			// イベントハンドルの取得
			auto event = CreateEvent(nullptr, false, false, nullptr);
			fence->SetEventOnCompletion(fenceValue, event);
			// イベントが発生するまで待ち続ける
			WaitForSingleObject(event, INFINITE);
			// イベントハンドルを閉じる
			CloseHandle(event);
		}

		g_cmdAllocator->Reset();							// キューをクリア
		g_cmdList->Reset(g_cmdAllocator, pipelineState);	// 再度コマンドリストを貯める準備(クローズ状態を解除)

		//画面スワップ(フリップ)
		g_swapChain->Present(1, 0);

		//***********************************************
	}

	// もうクラスは使用しないので登録解除
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}
