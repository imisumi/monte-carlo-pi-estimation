#pragma once

#include <imgui.h>
#include <glm/vec2.hpp>

#include <cstdint>
#include <vector>
#include <array>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class App
{
public:
	App();
	~App();

	void run();

private:
	void reset(uint32_t pixel_count);

private:
	static App *s_Instance;

	SDL_Window *m_window = nullptr;
	SDL_Renderer *m_renderer = nullptr;

	ImVec4 m_clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	uint32_t m_width = 2560;
	uint32_t m_height = 1440;

	std::vector<uint32_t> m_viewport_data;
	std::vector<uint32_t> m_accumulation_r;
	std::vector<uint32_t> m_accumulation_g;
	std::vector<uint32_t> m_accumulation_b;
	std::vector<uint32_t> m_sample_counts;
	SDL_Texture *m_viewport_texture = nullptr;

	int m_texture_size_preset = 4;
	static constexpr std::array<int, 6> m_texture_sizes = {8, 16, 32, 64, 512, 1024};
	static constexpr std::array<const char *, 6> m_texture_size_names = {"8x8", "16x16", "32x32", "64x64", "512x512", "1024x1024"};

	ImVec4 m_inside_color = ImVec4(120.0f / 255.0f, 160.0f / 255.0f, 255.0f / 255.0f, 1.0f);
	ImVec4 m_outside_color = ImVec4(255.0f / 255.0f, 140.0f / 255.0f, 140.0f / 255.0f, 1.0f);

	bool m_simulation_running = true;
	int m_points_per_frame = 10;

	uint64_t total_points = 0;
	uint64_t inside_circle = 0;
};
