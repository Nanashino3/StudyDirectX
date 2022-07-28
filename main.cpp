// Windows�v���O���~���O

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

// �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list vaList;
	va_start(vaList, format);
	vprintf(format, vaList);
	va_end(vaList);
#endif
}

// �f�o�b�O���C���[�̗L����
void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))) {
		debugLayer->EnableDebugLayer();	// �f�o�b�O���C���[�̗L����
		debugLayer->Release();			// �L����������C���^�t�F�[�X���J��
	}
}

// �ʓ|�����ǕK�{�֐�
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// �E�B���h�E���j�����ꂽ��Ă΂��
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);	// OS�ɑ΂��ăA�v���̏I����`����
		return 0;
	}

	// �K��̏������s��
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

	// �E�B���h�E�N���X�̐����Ɠo�^
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	// �R�[���o�b�N�֐��̎w��
	w.lpszClassName = _T("DX12Sample");			// �A�v���P�[�V�����N���X��
	w.hInstance = GetModuleHandle(nullptr);		// �n���h���̎擾

	// �A�v���P�[�V�����N���X(OS�ɃE�B���h�E�N���X�̎w���`����)
	RegisterClassEx(&w);

	// �E�B���h�E�T�C�Y�����߂�
	RECT wrc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };

	// �֐��𗘗p���E�B���h�E�T�C�Y��␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E�I�u�W�F�N�g�̐���
	HWND hwnd = CreateWindow(w.lpszClassName,					// �E�B���h�E�N���X���w��
							 _T("DX12 �P���|���S���e�X�g"),		// �^�C�g���o�[�̕���
							 WS_OVERLAPPEDWINDOW,				// �^�C�g���o�[�Ƌ��E��������E�B���h�E
							 CW_USEDEFAULT,						// �\��X���W��OS�ɂ��C��
							 CW_USEDEFAULT,						// �\��Y���W��OS�ɂ��C��
							 wrc.right - wrc.left,				// �E�B���h�E��
							 wrc.bottom - wrc.top,				// �E�B���h�E����
							 nullptr,							// �e�E�B���h�E�n���h��
							 nullptr,							// ���j���[�n���h��
							 w.hInstance,						// �Ăяo���A�v���P�[�V�����n���h��
							 nullptr);							// �ǉ��p�����[�^
							 
#ifdef _DEBUG
	// �f�o�C�X������ɂ��ƃf�o�C�X�����X�g���Ă��܂��̂Œ���
	EnableDebugLayer();
#endif

	/*DirectX12�܂�菉����*/
	// �t�B�[�`�����x����
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	// �A�_�v�^�[��񋓂��邽�߂�DXGIFactory�I�u�W�F�N�g�𐶐�����
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&g_dxgiFactory));
	std::vector<IDXGIAdapter*> adapters;	// �A�_�v�^�[�̗񋓗p

	// �����ɓ���̖��O�����A�_�v�^�[�I�u�W�F�N�g������
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

	// Direct3D�f�o�C�X�̏�����
	D3D_FEATURE_LEVEL featureLevel;
	for (auto lv : levels) {
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&g_dev)) == S_OK) {
			featureLevel = lv;
			break;
		}
	}

	// �R�}���h�A���P�[�^�̐���
	result = g_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_cmdAllocator));
	// �R�}���h���X�g�̐���
	result = g_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_cmdAllocator, nullptr, IID_PPV_ARGS(&g_cmdList));

	//***********************************************
	// �R�}���h�L���[�̎��Ԑ����Ɛݒ�
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;				// �^�C���A�E�g�Ȃ�
	cmdQueueDesc.NodeMask = 0;										// �A�_�v�^�[��������g�p���Ȃ��Ƃ���0
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// �D��x�͓��Ɏw��Ȃ�
	// �R�}���h���X�g�ƍ��킹��
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	// �L���[����
	result = g_dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&g_cmdQueue));
	//***********************************************

	//***********************************************
	// �X���b�v�`�F�[������
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = WINDOW_WIDTH;
	swapChainDesc.Height = WINDOW_HEIGHT;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;					// �o�b�N�o�b�t�@�[�͐L�яk�݉\
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;		// �t���b�v��͑��₩�ɔj��
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;			// ���Ɏw��Ȃ�
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// �E�B���h�E�̃t���X�N���[���ؑ։\

	result = g_dxgiFactory->CreateSwapChainForHwnd(g_cmdQueue,
												   hwnd,
												   &swapChainDesc,
												   nullptr,
												   nullptr,
												   (IDXGISwapChain1**)&g_swapChain);
	//***********************************************

	//***********************************************
	// �f�B�X�N���v�^�q�[�v����
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// RTV => �����_�[�^�[�Q�b�g�r���[
	heapDesc.NodeMask = 0;								// ������GPU�����ʂ��邽�߂̃r�b�g�}�X�N
	heapDesc.NumDescriptors = 2;						// �\����2��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;	// ���Ɏw��Ȃ�

	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = g_dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	// �f�B�X�N���v�^�ƃo�b�N�o�b�t�@�[�̊֘A�t��
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = g_swapChain->GetDesc(&swcDesc);

	std::vector<ID3D12Resource*> backBuffers(swcDesc.BufferCount);
	// �擪�A�h���X���擾
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	for (size_t index = 0; index < swcDesc.BufferCount; ++index) {
		// �X���b�v�`�F�[���̒��̃��������擾
		result = g_swapChain->GetBuffer(static_cast<UINT>(index), IID_PPV_ARGS(&backBuffers[index]));
		// RTV(�����_�[�^�[�Q�b�g�r���[)�𐶐�
		g_dev->CreateRenderTargetView(backBuffers[index], nullptr, handle);
		// �|�C���^�����炷
		handle.ptr += g_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	//***********************************************

	// �t�F���X�̍쐬
	ID3D12Fence* fence = nullptr;
	UINT64 fenceValue = 0;
	result = g_dev->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	// �E�B���h�E�\��
	ShowWindow(hwnd, SW_SHOW);

	//***********************************************
	// ���_�o�b�t�@�[�̐���
	struct Vertex {
		XMFLOAT3 pos;	// XYZ���W
		XMFLOAT2 uv;	// UV���W
	};

	Vertex vertices[] = {
		{{-0.4f, -0.7f, 0.0f}, {0.0f, 1.0f}},	// ����
		{{-0.4f,  0.7f, 0.0f}, {0.0f, 0.0f}},	// ����
		{{ 0.4f, -0.7f, 0.0f}, {1.0f, 1.0f}},	// �E��
		{{ 0.4f,  0.7f, 0.0f}, {1.0f, 0.0f}},	// �E��
	};

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;						// CPU����A�N�Z�X�ł���(�}�b�v���ł���)
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeof(vertices);	// ���_��񂪓��邾���̃T�C�Y
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
	// ���_���̃R�s�[(�}�b�v)
	Vertex* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);
	//***********************************************

	//***********************************************
	// ���_�o�b�t�@�[�r���[�̍쐬
	D3D12_VERTEX_BUFFER_VIEW vbView = {};

	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();	// �o�b�t�@�[�̉��z�A�h���X
	vbView.SizeInBytes = sizeof(vertices);						// �S�o�C�g��
	vbView.StrideInBytes = sizeof(vertices[0]);					// 1���_������̃o�C�g��
	//***********************************************

	//***********************************************
	// �C���f�b�N�X�̗��p
	unsigned short indices[] = {0, 1, 2, 2, 1, 3};

	ID3D12Resource* idxBuff = nullptr;
	// �ݒ�̓o�b�t�@�[�T�C�Y�ȊO�A���_�o�b�t�@�[�̐ݒ���g���܂킵�ėǂ�
	resourceDesc.Width = sizeof(indices);
	result = g_dev->CreateCommittedResource(&heapProperties,
											D3D12_HEAP_FLAG_NONE,
											&resourceDesc,
											D3D12_RESOURCE_STATE_GENERIC_READ,
											nullptr,
											IID_PPV_ARGS(&idxBuff));

	// ������o�b�t�@�[�ɃC���f�b�N�X�f�[�^���R�s�[
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	// �C���f�b�N�X�o�b�t�@�r���[���쐬
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeof(indices);

	//***********************************************

	ID3DBlob* vsBlob = nullptr;		// ���_�V�F�[�_
	ID3DBlob* psBlob = nullptr;		// �s�N�Z���V�F�[�_
	ID3DBlob* errBlob = nullptr;	// �G���[���

	// ���_�o�b�t�@�[�̓ǂݍ���
	result = D3DCompileFromFile(L"Shader/BasicVertexShader.hlsl",							// �V�F�[�_��
								nullptr,
								D3D_COMPILE_STANDARD_FILE_INCLUDE,					// �C���N���[�h�̓f�t�H���g
								"BasicVS", "vs_5_0",								// �֐���BasicVS�A�ΏۃV�F�[�_��vs_5_0
								D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	// �f�o�b�N�p�y�эœK���Ȃ�
								0,
								&vsBlob, &errBlob);

	// �G���[����
	if(FAILED(result)){
		if(result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)){
			::OutputDebugStringA("�t�@�C������������܂���");
		}else{
			std::string errStr;
			errStr.resize(errBlob->GetBufferSize());
			std::copy_n((char*)errBlob->GetBufferPointer(), errBlob->GetBufferSize(), errStr.begin());
			errStr += "\n";

			::OutputDebugStringA(errStr.c_str());
		}
		exit(1);
	}

	// �s�N�Z���V�F�[�_�̓ǂݍ���
	result = D3DCompileFromFile(L"Shader/BasicPixelShader.hlsl",
								nullptr,
								D3D_COMPILE_STANDARD_FILE_INCLUDE,
								"BasicPS", "ps_5_0",
								D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,	// �֐���BasicPS�A�ΏۃV�F�[�_��ps_5_0
								0,
								&psBlob, &errBlob);

	if(FAILED(result)){
		if(result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)){
			::OutputDebugStringA("�t�@�C������������܂���");
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
	// ���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		// ���W���
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		// UV���
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
	//***********************************************

	//***********************************************
	// �O���t�B�b�N�X�p�C�v���C��
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gPipeline = {};
	gPipeline.pRootSignature = nullptr;
	gPipeline.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	gPipeline.VS.BytecodeLength  = vsBlob->GetBufferSize();
	gPipeline.PS.pShaderBytecode = psBlob->GetBufferPointer();
	gPipeline.PS.BytecodeLength	 = psBlob->GetBufferSize();

	// �f�t�H���g�̃T���v���}�X�N��\���萔(0xffffffff)
	gPipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	
	// �u�����h�X�e�[�g�̐ݒ�
	gPipeline.BlendState.AlphaToCoverageEnable = false;
	gPipeline.BlendState.IndependentBlendEnable = false;
	
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	
	gPipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	// ���X�^���C�U�[�X�e�[�g�̐ݒ�
	gPipeline.RasterizerState.MultisampleEnable = false;
	gPipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// �J�����O���Ȃ�
	gPipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;	// ���g��h��Ԃ�
	gPipeline.RasterizerState.DepthClipEnable = true;			// �[�x�����ɃN���b�s���O�͗L����
	//�c��
	gPipeline.RasterizerState.FrontCounterClockwise = false;
	gPipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gPipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gPipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gPipeline.RasterizerState.AntialiasedLineEnable = false;
	gPipeline.RasterizerState.ForcedSampleCount = 0;
	gPipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// ���̓��C�A�E�g�̐ݒ�
	gPipeline.InputLayout.pInputElementDescs = inputLayout;						// ���C�A�E�g�擪�A�h���X
	gPipeline.InputLayout.NumElements = _countof(inputLayout);					// ���C�A�E�g�z��̗v�f��
	gPipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	// �J�b�g�Ȃ�
	gPipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// �O�p�`�ō\��

	// �����_�[�^�[�Q�b�g�̐ݒ�
	gPipeline.NumRenderTargets = 1;							// �����_�[�^�[�Q�b�g��
	gPipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	// 0�`1�ɐ��K�����ꂽRGBA

	// �A���`�G�C���A�V���O(AA)�̐ݒ�
	gPipeline.SampleDesc.Count = 1;		// �T���v�����O��1�s�N�Z��
	gPipeline.SampleDesc.Quality = 0;	// �N�I���e�B�͍Œ�

	// ���[�g�V�O�l�`���̍쐬(��)
	D3D12_ROOT_SIGNATURE_DESC  rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;	// ���_���(���̓A�Z���u��)������

	// �o�C�i���R�[�h�̍쐬
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc,			 // ���[�g�V�O�l�`���ݒ�
										 D3D_ROOT_SIGNATURE_VERSION_1_0, // ���[�g�V�O�l�`���o�[�W����
										 &rootSigBlob, &errBlob);

	// ���[�g�V�O�l�`���I�u�W�F�N�g�̍쐬
	ID3D12RootSignature* rootSignature = nullptr;
	result = g_dev->CreateRootSignature(0,
										rootSigBlob->GetBufferPointer(),
										rootSigBlob->GetBufferSize(),
										IID_PPV_ARGS(&rootSignature));
	// �s�v�ɂȂ����̂ŊJ��
	rootSigBlob->Release();
	gPipeline.pRootSignature = rootSignature;

	// �O���t�B�b�N�X�p�C�v���C���X�e�[�g�I�u�W�F�N�g�̐���
	ID3D12PipelineState* pipelineState = nullptr;
	result = g_dev->CreateGraphicsPipelineState(&gPipeline, IID_PPV_ARGS(&pipelineState));
	//***********************************************

	//***********************************************
	// �r���[�|�[�g�ƃV�U�[��`

	// �r���[�|�[�g�ݒ�
	D3D12_VIEWPORT viewPort = {};
	viewPort.Width = WINDOW_WIDTH;		// �o�͐�̕�(�s�N�Z����)
	viewPort.Height = WINDOW_HEIGHT;	// �o�͐�̍���(�s�N�Z����)
	viewPort.TopLeftX = 0;				// �o�͐�̍�����WX
	viewPort.TopLeftY = 0;				// �o�͐�̍�����WY
	viewPort.MaxDepth = 1.0f;			// �[�x�ő�l
	viewPort.MinDepth = 0.0f;			// �[�x�ŏ��l

	// �V�U�[��`�ݒ�
	D3D12_RECT scissorRect = {};
	scissorRect.top = 0;									// �؂蔲������W
	scissorRect.left = 0;									// �؂蔲�������W
	scissorRect.right = scissorRect.left + WINDOW_WIDTH;	// �؂蔲���E���W
	scissorRect.bottom = scissorRect.top + WINDOW_HEIGHT;	// �؂蔲�������W

	//***********************************************

	struct TexRGBA{
		unsigned char R, G, B, A;
	};
	std::vector<TexRGBA> textureData(256 * 256);

	for(auto& rgba : textureData){
		rgba.R = rand() % 256;
		rgba.G = rand() % 256;
		rgba.B = rand() % 256;
		rgba.A = 255;			// ����1.0�Ƃ���
	}

	MSG msg = {};
	unsigned int frame = 0;
	while (1) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// �A�v���P�[�V�������I������Ƃ���message��WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT) { break; }

		//***********************************************
		// DirectX�̏���

		// �o�b�N�o�b�t�@��Index���擾
		auto bbIndex = g_swapChain->GetCurrentBackBufferIndex();

		// ���\�[�X�o���A
		D3D12_RESOURCE_BARRIER barrierDesc = {};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrierDesc.Transition.pResource = backBuffers[bbIndex];
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		g_cmdList->ResourceBarrier(1, &barrierDesc);
		g_cmdList->SetPipelineState(pipelineState);		// �p�C�v���C���X�e�[�g�̐ݒ�

		// �����_�[�^�[�Q�b�g���w��
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIndex * g_dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		g_cmdList->OMSetRenderTargets(1, &rtvH, false, nullptr);

		// ��ʂ����F�ŃN���A(RGBA)
		float r, g, b;
		r = (float)(0xff & frame >> 16) / 255.0f;
		g = (float)(0xff & frame >>  8) / 255.0f;
		b = (float)(0xff & frame >>  0) / 255.0f;
		float clearColor[] = { r, g, b, 1.0f };
		g_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		++frame;

		g_cmdList->RSSetViewports(1, &viewPort);								// �r���[�|�[�g�̐ݒ�
		g_cmdList->RSSetScissorRects(1, &scissorRect);							// �V�U�[��`�̐ݒ�
		g_cmdList->SetGraphicsRootSignature(rootSignature);						// ���[�g�V�O�l�`���̐ݒ�

		g_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	// �v���~�e�B�u�g�|���W�̐ݒ�
		g_cmdList->IASetVertexBuffers(0, 1, &vbView);							// ���_���̐ݒ�
		g_cmdList->IASetIndexBuffer(&ibView);

		//// ��1�����F���_���A��2�����F�C���X�^���X���A��3�����F���_�f�[�^�A��4�����F�C���X�^���X�̃I�t�Z�b�g
		//g_cmdList->DrawInstanced(3, 1, 0, 0);									// �`�施��
		g_cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// ��Ԃ��uPresent�v�Ɉڍs
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		g_cmdList->ResourceBarrier(1, &barrierDesc);

		// �R�}���h���X�g���s�O�ɖ��߂��N���[�Y
		g_cmdList->Close();

		// �R�}���h���X�g�̎��s
		ID3D12CommandList* cmdLists[] = { g_cmdList };
		g_cmdQueue->ExecuteCommandLists(1, cmdLists);
		g_cmdQueue->Signal(fence, ++fenceValue);

		// �r�W�[���[�v(�҂���Ԃ�\��)
		if (fence->GetCompletedValue() != fenceValue) {
			// �C�x���g�n���h���̎擾
			auto event = CreateEvent(nullptr, false, false, nullptr);
			fence->SetEventOnCompletion(fenceValue, event);
			// �C�x���g����������܂ő҂�������
			WaitForSingleObject(event, INFINITE);
			// �C�x���g�n���h�������
			CloseHandle(event);
		}

		g_cmdAllocator->Reset();							// �L���[���N���A
		g_cmdList->Reset(g_cmdAllocator, pipelineState);	// �ēx�R�}���h���X�g�𒙂߂鏀��(�N���[�Y��Ԃ�����)

		//��ʃX���b�v(�t���b�v)
		g_swapChain->Present(1, 0);

		//***********************************************
	}

	// �����N���X�͎g�p���Ȃ��̂œo�^����
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}
