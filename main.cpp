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

void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))){
		debugLayer->EnableDebugLayer();	// �f�o�b�O���C���[��L����
		debugLayer->Release();			// �L����������C���^�[�t�F�[�X�����
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

#ifdef _DEBUG
	// �f�o�b�O���C���[��ON
	EnableDebugLayer();
#endif

	//-----------------------------------------
	// DirectX12����̏�����

	// �t�B�[�`���[���x���̗�
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&gDxgiFactory));

	// ���p�\�ȃA�_�v�^�[���
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

	// Direct3D�f�o�C�X�̏�����
	D3D_FEATURE_LEVEL featureLevel;
	for(auto level : levels){
		if(D3D12CreateDevice(nullptr, level, IID_PPV_ARGS(&gDev)) == S_OK){
			featureLevel = level;
			break;
		}
	}

	result = gDev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gCmdAllocator));
	result = gDev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gCmdAllocator, nullptr, IID_PPV_ARGS(&gCmdList));

	// �L���[�쐬
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;			 // �^�C���A�E�g�Ȃ�
	cmdQueueDesc.NodeMask = 0;									 // �A�_�v�^�[��1�����g�p���Ȃ��ꍇ0�ł悢
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // �v���C�I���e�B�͓��Ɏw��Ȃ�
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;			 // �R�}���h���X�g�ƍ��킹��
	result = gDev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&gCmdQueue));

	// �X���b�v�`�F�[������
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = WINDOW_WIDTH;
	swapchainDesc.Height = WINDOW_HEIGHT;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;					// �o�b�N�o�b�t�@�[�͐L�яk�݉\
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// �t���b�v��͑��₩�ɔj��
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;			// ���Ɏw��Ȃ�
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// �E�B���h�̃t���X�N���[���ؑ։\
	
	result = gDxgiFactory->CreateSwapChainForHwnd(gCmdQueue,
												  hwnd,
												  &swapchainDesc,
												  nullptr,
												  nullptr,
												  (IDXGISwapChain1**)&gSwapChain);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// �����_�[�^�[�Q�b�g�r���[�Ȃ̂�RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;						// �\����2��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// ���ɂȂ�

	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = gDev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = gSwapChain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> backBuffers(swcDesc.BufferCount);

	// �擪�̃A�h���X�𓾂�
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for(int idx = 0; idx < swcDesc.BufferCount; ++idx){
		result = gSwapChain->GetBuffer(idx, IID_PPV_ARGS(&backBuffers[idx]));
		gDev->CreateRenderTargetView(backBuffers[idx], nullptr, handle);
		handle.ptr += gDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// �t�F���X�̃I�u�W�F�N�g����
	ID3D12Fence* fence = nullptr;
	UINT64 fenceValue = 0;
	result = gDev->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	// �E�B���h�E�\��
	ShowWindow(hwnd, SW_SHOW);

	DirectX::XMFLOAT3 vertices[] = {
		{-1.0f, -1.0f, 0.0f},	// ����
		{-1.0f,  1.0f, 0.0f},	// ����
		{ 1.0f, -1.0f, 0.0f}	// �E��
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
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress(); // �o�b�t�@�[�̉��z�A�h���X
	vbView.SizeInBytes = sizeof(vertices);					  // �S�o�C�g��
	vbView.StrideInBytes = sizeof(vertices[0]);				  // 1���_������̃o�C�g��

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
			::OutputDebugStringA("�t�@�C������������܂���");
		}else{
			std::string errStr;

			// �K�v�ȃT�C�Y���m��
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
			::OutputDebugStringA("�t�@�C������������܂���");
		}else{
			std::string errStr;
			errStr.resize(errBlob->GetBufferSize());
			std::copy_n((char*)errBlob->GetBufferPointer(), errBlob->GetBufferSize(), errStr.begin());
			errStr += "\n";
			OutputDebugStringA(errStr.c_str());
		}
		exit(1);
	}

	// ���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// �O���t�B�b�N�X�p�C�v���C��
	D3D12_GRAPHICS_PIPELINE_STATE_DESC grPipeline = {};
	grPipeline.pRootSignature = nullptr;
	grPipeline.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	grPipeline.VS.BytecodeLength = vsBlob->GetBufferSize();
	grPipeline.PS.pShaderBytecode = psBlob->GetBufferPointer();
	grPipeline.PS.BytecodeLength = psBlob->GetBufferSize();

	// �f�t�H���g�̃T���v���}�X�N��\���萔(0xffffffff)
	grPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	grPipeline.BlendState.AlphaToCoverageEnable = false;
	grPipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

	// �ЂƂ܂����Z���Z�⃿�u�����f�B���O�͎g�p���Ȃ�
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// �ЂƂ܂��_�����Z�͎g�p���Ȃ�
	renderTargetBlendDesc.LogicOpEnable = false;

	grPipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	grPipeline.RasterizerState.MultisampleEnable = false;		 // ��Ufalse
	grPipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  // �J�����O���Ȃ�
	grPipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // ���g��h��Ԃ�
	grPipeline.RasterizerState.DepthClipEnable = true;			 // �[�x�����̃N���b�s���O�͗L��

	grPipeline.InputLayout.pInputElementDescs = inputLayout;	 // ���C�A�E�g�擪�A�h���X
	grPipeline.InputLayout.NumElements = _countof(inputLayout);	 // ���C�A�E�g�z��

	grPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // �X�g���b�v���̃J�b�g�Ȃ�
	grPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // �O�p�`�ō\��0

	grPipeline.NumRenderTargets = 1;						// ����1�̂�
	grPipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	// 0�`1�ɐ��K�����ꂽRGBA

	grPipeline.SampleDesc.Count = 1;	// �T���v�����O��1�s�N�Z���ɂ�1
	grPipeline.SampleDesc.Quality = 0;	// �N�I���e�B�͍Œ�

	// ���[�g�V�O�l�`���̍쐬
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
	viewport.Width = WINDOW_WIDTH;	 // �o�͐�̕�(�s�N�Z����)
	viewport.Height = WINDOW_HEIGHT; // �o�͐�̍���(�s�N�Z����)
	viewport.TopLeftX = 0;			 // �o�͐�̍�����WX
	viewport.TopLeftY = 0;			 // �o�͐�̍�����WY
	viewport.MaxDepth = 1.0f;		 // �[�x�ő�l
	viewport.MinDepth = 0.0f;		 // �[�x�ŏ��l

	D3D12_RECT scissorRect = {};
	scissorRect.top = 0;								  // �؂蔲������W
	scissorRect.left = 0;								  // �؂蔲�������W
	scissorRect.right = scissorRect.left + WINDOW_WIDTH;  // �؂蔲���E���W
	scissorRect.bottom = scissorRect.top + WINDOW_HEIGHT; // �؂蔲�������W

	MSG msg = {};

	while(true)
	{
		if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// �A�v���P�[�V�������I���Ƃ���message��WN_QUIT�ɂȂ�
		if(msg.message == WM_QUIT){ break; }

		// �o�b�N�o�b�t�@�[�̃C���f�b�N�X���擾
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

		// �����_�[�^�[�Q�b�g���w��
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * gDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		gCmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		// ��ʃN���A(���F)
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

		// ���߂̃N���[�Y
		gCmdList->Close();

		// �R�}���h���X�g�̎��s
		ID3D12CommandList* cmdLists[] = { gCmdList };
		gCmdQueue->ExecuteCommandLists(1, cmdLists);

		gCmdQueue->Signal(fence, ++fenceValue);
		if(fence->GetCompletedValue() != fenceValue){
			// �C�x���g�n���h���̎擾
			auto event = CreateEvent(nullptr, false, false, nullptr);
			fence->SetEventOnCompletion(fenceValue, event);

			// �C�x���g����������܂ő҂�������
			WaitForSingleObject(event, INFINITE);

			// �C�x���g�n���h�������
			CloseHandle(event);
		}

		gCmdAllocator->Reset();					 // �L���[���N���A
		gCmdList->Reset(gCmdAllocator, nullptr); // �ĂуR�}���h���X�g�𒙂߂鏀��

		// �t���b�v
		gSwapChain->Present(1, 0);
	}

	// �����N���X�͎g�p���Ȃ��̂œo�^��������
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
}