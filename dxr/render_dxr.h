#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include "dx12_utils.h"
#include "dxr_utils.h"
#include "render_backend.h"

struct RenderDXR : RenderBackend {
	Microsoft::WRL::ComPtr<ID3D12Device5> device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> cmd_queue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmd_allocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> cmd_list;
	
	dxr::Buffer view_param_buf, img_readback_buf,
		bottom_level_as, instance_buf, material_param_buf,
		light_buf;

	dxr::Texture2D render_target, accum_buffer;
	std::vector<dxr::Texture2D> textures;

	std::vector<dxr::TriangleMesh> meshes;
	dxr::TopLevelBVH scene_bvh;

	dxr::RTPipeline rt_pipeline;
	dxr::DescriptorHeap raygen_desc_heap, raygen_sampler_heap;

	uint64_t shader_table_entry_size = 0;
	
	uint64_t fence_value = 1;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	HANDLE fence_evt;

	uint32_t frame_id = 0;

	RenderDXR();
	virtual ~RenderDXR();

	std::string name() override;
	void initialize(const int fb_width, const int fb_height) override;
	void set_scene(const Scene &scene) override;
	RenderStats render(const glm::vec3 &pos, const glm::vec3 &dir,
			const glm::vec3 &up, const float fovy, const bool camera_changed) override;

private:
	void build_raytracing_pipeline();
	void build_shader_resource_heap();
	void build_shader_binding_table();
	void update_view_parameters(const glm::vec3 &pos, const glm::vec3 &dir,
		const glm::vec3 &up, const float fovy);
	void build_descriptor_heap();
	void sync_gpu();
};
