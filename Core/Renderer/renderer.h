#pragma once
#include <D3D12.h>
#include <DXGI1_4.h>

#include <wrl.h>
#include <memory>

namespace KLeaf {

class Renderer {
public:

	Renderer() = default;
	virtual ~Renderer() = default;

	virtual bool initialize();
	virtual void update();
	virtual void render();
	virtual void terminate();

private:

	void load_pipeline();
	void load_assets();
	void populate_command_list();
	void wai_for_previous_frame();

	static const uint32_t k_frame_count_ = 2;
	static const uint32_t k_swap_chain_width_ = 720;
	static const uint32_t k_swap_chain_height_ = 480;

	// pipeline
	D3D12_VIEWPORT viewport_;
	D3D12_RECT scissor_rectangle_;
	std::unique_ptr<IDXGISwapChain3> swap_chain_;
	ID3D12Device* native_device_;
	std::unique_ptr<ID3D12Resource> render_targets_[k_frame_count_];
	std::unique_ptr<ID3D12CommandAllocator> command_allocator_;
	std::unique_ptr<ID3D12CommandQueue> command_queue_;
	std::unique_ptr<ID3D12RootSignature> root_signature_; // シェーダがアクセスするリソース検出のため
	std::unique_ptr<ID3D12DescriptorHeap> rtv_descriptor_heap_;
	std::unique_ptr<ID3D12PipelineState> pipeline_state_;
	std::unique_ptr<ID3D12GraphicsCommandList> graphic_command_list_;

	uint32_t rtv_descriptor_size_; // render target view(RTV)

	// App Resources
	std::unique_ptr<ID3D12Resource> vertex_buffer_;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_;

	// Syncronaization Objects
	uint32_t frame_index_;
	HANDLE fence_evemt_; // HANDLE は void* なので他に適切なものがないかあとで調査する
	std::unique_ptr<ID3D12Fence> fence_;
	uint64_t fence_value_;


	// used by load_pipeline()
	bool use_warp_device_;

}; // class 
} // namespace kleaf