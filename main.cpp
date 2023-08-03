#include<Windows.h>
#include<tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include<vector>
//#include<string>
#ifdef _DEBUG
#include<iostream>
#endif

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

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

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};

	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;			 // �^�C���A�E�g�Ȃ�
	cmdQueueDesc.NodeMask = 0;									 // �A�_�v�^�[��1�����g�p���Ȃ��ꍇ0�ł悢
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // �v���C�I���e�B�͓��Ɏw��Ȃ�
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;			 // �R�}���h���X�g�ƍ��킹��
	
	// �L���[�쐬
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

		// �����_�[�^�[�Q�b�g���w��
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * gDev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		gCmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		// ��ʃN���A(���F)
		float clearColor[] = {1.0f, 1.0f, 0.0f, 1.0f};
		gCmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

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