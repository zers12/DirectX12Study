// TODO インクルードパスを整理
#include "renderer.h"
#include <Window/window.h>

#include <External/d3dx12.h>
#include <D3DCompiler.h> // for macro D3DCOMPILE_XXX ...

using namespace Microsoft::WRL;

// anonymouse namespace
namespace {

void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter)
{
	*ppAdapter = nullptr;
	for (UINT adapterIndex = 0; ; ++adapterIndex)
	{
		IDXGIAdapter1* pAdapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter))
		{
			// No more adapters to enumerate.
			break;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			*ppAdapter = pAdapter;
			return;
		}
		pAdapter->Release();
	}
}
}

namespace KLeaf {

	// public

	bool Renderer::initialize() {

		load_pipeline();
		load_assets();

		return true;
	}


	void Renderer::update() {



	}

	void Renderer::render()
	{
	}

	void Renderer::terminate()
	{
	}


	/// private

	/// 1) Enable debug layer
	/// 2) Create Device
	/// 3) Describe command queue
	/// 4) Describe swap chain
	void Renderer::load_pipeline()
	{
#if defined(_DEBUG)

		// Enable the D3D12 debug layer.
		ID3D12Debug* debug_controller;
		if (D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)) == S_OK) {
			debug_controller->EnableDebugLayer();
		}
		else {
			printf("Failed to creation debug layer.\n");
		}
#endif

		// Create Device

		IDXGIFactory4* factory;

		if (S_OK != CreateDXGIFactory1(IID_PPV_ARGS(&factory))){
			printf("Failed to CreatedDXGIFactory1()\n");
		}

		if (use_warp_device_) {
			IDXGIAdapter* warp_adapter;
			if (S_OK != factory->EnumWarpAdapter(IID_PPV_ARGS(&warp_adapter))) {
				printf("Failed to call IDXGIFactory4::EnumWarpAdapter()\n");
			}

			if (S_OK != D3D12CreateDevice(warp_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&native_device_))) {
				printf("Failed to D3D12CreateDevice.\n");
			}

		}
		else {

			IDXGIAdapter1* hardware_adapter;
			GetHardwareAdapter(factory, &hardware_adapter);

			if (S_OK != D3D12CreateDevice(hardware_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&native_device_))) {
				printf("Failed to D3D12CreateDevice.\n");
			}
		}

		// Describe and create the command queue.
		D3D12_COMMAND_QUEUE_DESC queue_desc = {};
		queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ID3D12CommandQueue* queue = command_queue_.get();
		HRESULT result_create_cq = native_device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue));
		if (S_OK != result_create_cq) {
			printf("Failed to create command queue.\n");
		}

		DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
		swap_chain_desc.BufferCount = k_frame_count_;
		swap_chain_desc.BufferDesc.Width = k_swap_chain_width_;
		swap_chain_desc.BufferDesc.Height = k_swap_chain_height_;
		swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		// TODO :初期化済みかこの前にチェック
		swap_chain_desc.OutputWindow = KLeaf::Window::get_window_handle(); 

		IDXGISwapChain* swap_chain;
		HRESULT result_of_swap_chain = factory->CreateSwapChain(
			command_queue_.get(),
			&swap_chain_desc,
			&swap_chain
		);

		auto* swap_chain3 = swap_chain_.get();
		swap_chain->QueryInterface(IID_PPV_ARGS(&swap_chain3)); // TODO : HRESULT

		// This sample does not support fullscreen transitions.
		factory->MakeWindowAssociation(
			KLeaf::Window::get_window_handle(),
			DXGI_MWA_NO_ALT_ENTER);

		frame_index_ = swap_chain_->GetCurrentBackBufferIndex();

		// Create Descriptor Heap
		D3D12_DESCRIPTOR_HEAP_DESC rtv_descriptor_heap = {};
		rtv_descriptor_heap.NumDescriptors = k_frame_count_;
		rtv_descriptor_heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtv_descriptor_heap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		auto* descriptor_heap = rtv_descriptor_heap_.get();
		native_device_->CreateDescriptorHeap(&rtv_descriptor_heap, IID_PPV_ARGS(&descriptor_heap));

		rtv_descriptor_size_ = native_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Create Frame Resources
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());

		for (uint32_t i = 0; i < k_frame_count_; ++i) {

			auto* render_target = render_targets_[i].get();
			swap_chain_->GetBuffer(i, IID_PPV_ARGS(&render_target));
			native_device_->CreateRenderTargetView(render_target, nullptr, rtv_handle);
			rtv_handle.Offset(1, rtv_descriptor_size_);
		}

		auto* command_allocator = command_allocator_.get();
		native_device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator));

	}

	// To create PSO
	void Renderer::load_assets()
	{
		// Create an empty root signature.
		{
			CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
			root_signature_desc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;

			D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

			auto* root_signature = root_signature_.get();
			native_device_->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature));
		}

		// Create the pipeline state, which includes compiling and loading shaders.
		{
			ComPtr<ID3DBlob> vertex_shader;
			ComPtr<ID3DBlob> pixel_shader;

#if defined(_DEBUG)
			// Enable better shader debugging with the graphics debugging tools.
			uint32_t compile_flgas = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			uint32_t compile_flags = 0;
#endif
			// TODO : シェーダの用意
			D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compile_flgas, 0, &vertex_shader, nullptr);
			D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compile_flgas, 0, &pixel_shader, nullptr);

			// Define the vertex input layout.

			// ここから


		}


	}

	void Renderer::populate_command_list()
	{
	}

	void Renderer::wai_for_previous_frame()
	{
	}


}