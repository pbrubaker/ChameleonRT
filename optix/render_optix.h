#pragma once

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>
#include "render_backend.h"

struct RenderOptiX : RenderBackend {
	optix::Context context;
	optix::Buffer fb, accum_buffer, view_params;
	int width, height;
	uint32_t frame_id = 0;

	RenderOptiX();

	void initialize(const int fb_width, const int fb_height) override;
	void set_mesh(const std::vector<float> &verts,
			const std::vector<uint32_t> &indices) override;
	double render(const glm::vec3 &pos, const glm::vec3 &dir,
			const glm::vec3 &up, const float fovy, const bool camera_changed) override;
};

