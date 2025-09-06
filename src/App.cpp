#include "App.h"
#include <assert.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <stdio.h>
#include <stdexcept>
#include <vector>
#include <ranges>
#include <algorithm>

#include <SDL3/SDL.h>
#include <glm/gtc/matrix_transform.hpp>

static void SetDarkThemeColors();

static uint32_t ToColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	// RGBA8888 format: R in bits 24-31, G in bits 16-23, B in bits 8-15, A in bits 0-7
	return (r << 24) | (g << 16) | (b << 8) | (a << 0);
}

static float random_float(uint32_t &state)
{
	uint32_t result;

	state = state * 747796405 + 2891336453;
	result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
	result = (result >> 22) ^ result;
	return ((float)result / 4294967295.0f);
}

App *App::s_Instance = nullptr;

App::App()
{
	assert(s_Instance == nullptr && "App already exists!");
	s_Instance = this;
	// Setup SDL
	// [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		printf("Error: SDL_Init(): %s\n", SDL_GetError());
		throw std::runtime_error("Failed to initialize SDL");
	}

	// Create window with SDL_Renderer graphics context
	float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	m_window = SDL_CreateWindow("Dear ImGui SDL3+SDL_Renderer example", (int)(m_width * main_scale), (int)(m_height * main_scale), window_flags);
	if (m_window == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		throw std::runtime_error("Failed to create SDL m_window");
	}
	m_renderer = SDL_CreateRenderer(m_window, nullptr);
	SDL_SetRenderVSync(m_renderer, 1);
	if (m_renderer == nullptr)
	{
		SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
		throw std::runtime_error("Failed to create SDL m_renderer");
	}
	SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(m_window);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;	  // Enable Docking

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	SetDarkThemeColors();
	// ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle &style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
	// io.ConfigDpiScaleFonts = true;        // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
	// io.ConfigDpiScaleViewports = true;    // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

	// Setup Platform/Renderer backends
	ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer);
	ImGui_ImplSDLRenderer3_Init(m_renderer);

	// Initialize with default preset size (512x512)
	int default_size = m_texture_sizes[m_texture_size_preset];
	m_viewport_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, default_size, default_size);

	// Disable texture filtering to see individual pixels
	SDL_SetTextureScaleMode(m_viewport_texture, SDL_SCALEMODE_NEAREST);

	// Initialize texture and accumulation buffers
	int pixel_count = default_size * default_size;
	m_viewport_data.resize(pixel_count);
	m_accumulation_r.resize(pixel_count, 0);
	m_accumulation_g.resize(pixel_count, 0);
	m_accumulation_b.resize(pixel_count, 0);
	m_sample_counts.resize(pixel_count, 0);
	std::fill(m_viewport_data.begin(), m_viewport_data.end(), ToColor(0, 0, 0, 255));
	SDL_UpdateTexture(m_viewport_texture, nullptr, m_viewport_data.data(), default_size * 4);
}

App::~App()
{
	// Cleanup
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}

void App::run()
{

	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	// Main loop
	bool done = false;
	uint32_t rng_state = 0;
	while (!done)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		// [If using SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your SDL_AppEvent() function]
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT)
				done = true;
			if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_window))
				done = true;
		}

		// [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate() function]
		if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED)
		{
			SDL_Delay(10);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

		{
			ImGui::Begin("Viewport");

			ImVec2 content_region = ImGui::GetContentRegionAvail();

			// write to texture - add many points per frame
			if (m_simulation_running)
			{
				int texture_width = m_viewport_texture->w;
				int texture_height = m_viewport_texture->h;
				for (int i = 0; i < m_points_per_frame; ++i)
				{
					glm::vec2 coord{random_float(rng_state), random_float(rng_state)};
					int x = (int)(coord.x * texture_width);
					int y = (int)(coord.y * texture_height);

					// Clamp to prevent out-of-bounds (only happens when coord is exactly 1.0)
					if (x >= texture_width)
						x = texture_width - 1;
					if (y >= texture_height)
						y = texture_height - 1;
					coord = coord * 2.0f - glm::vec2(1.0f, 1.0f);

					int pixel_index = y * texture_width + x;
					m_sample_counts[pixel_index]++;
					total_points++;

					// Use configurable colors
					uint8_t r, g, b;
					if (glm::length(coord) <= 1.0f)
					{
						// Use inside circle color
						r = (uint8_t)(m_inside_color.x * 255.0f);
						g = (uint8_t)(m_inside_color.y * 255.0f);
						b = (uint8_t)(m_inside_color.z * 255.0f);
						inside_circle++;
					}
					else
					{
						// Use outside circle color
						r = (uint8_t)(m_outside_color.x * 255.0f);
						g = (uint8_t)(m_outside_color.y * 255.0f);
						b = (uint8_t)(m_outside_color.z * 255.0f);
					}

					// Accumulate colors
					m_accumulation_r[pixel_index] += r;
					m_accumulation_g[pixel_index] += g;
					m_accumulation_b[pixel_index] += b;
				}

				// Update texture based on accumulated colors divided by sample count
				for (int y = 0; y < texture_height; ++y)
				{
					for (int x = 0; x < texture_width; ++x)
					{
						int pixel_index = y * texture_width + x;
						uint32_t samples = m_sample_counts[pixel_index];

						if (samples == 0)
						{
							m_viewport_data[pixel_index] = ToColor(0, 0, 0, 255); // Black for no samples
						}
						else
						{
							// Average the accumulated colors
							uint8_t final_r = (uint8_t)(m_accumulation_r[pixel_index] / samples);
							uint8_t final_g = (uint8_t)(m_accumulation_g[pixel_index] / samples);
							uint8_t final_b = (uint8_t)(m_accumulation_b[pixel_index] / samples);
							m_viewport_data[pixel_index] = ToColor(final_r, final_g, final_b, 255);
						}
					}
				}

				SDL_UpdateTexture(m_viewport_texture, nullptr, m_viewport_data.data(), texture_width * 4);
			}

			// Scale image to fit viewport while maintaining aspect ratio and center it
			float texture_aspect = (float)m_viewport_texture->w / (float)m_viewport_texture->h;
			float viewport_aspect = content_region.x / content_region.y;

			ImVec2 image_size;
			if (texture_aspect > viewport_aspect)
			{
				// Texture is wider, fit to width
				image_size.x = content_region.x;
				image_size.y = content_region.x / texture_aspect;
			}
			else
			{
				// Texture is taller, fit to height
				image_size.y = content_region.y;
				image_size.x = content_region.y * texture_aspect;
			}

			ImVec2 cursor_pos = ImGui::GetCursorPos();
			ImVec2 center_pos = ImVec2(
				cursor_pos.x + (content_region.x - image_size.x) * 0.5f,
				cursor_pos.y + (content_region.y - image_size.y) * 0.5f);
			ImGui::SetCursorPos(center_pos);

			ImGui::Image(m_viewport_texture, image_size);

			ImGui::End();
		}

		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Properties");

			// Texture size selection
			ImGui::Text("Texture Size:");
			if (ImGui::Combo("##TextureSize", &m_texture_size_preset, m_texture_size_names.data(), 6))
			{
				// Recreate texture with new size
				int new_size = m_texture_sizes[m_texture_size_preset];

				SDL_DestroyTexture(m_viewport_texture);
				m_viewport_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, new_size, new_size);

				// Disable texture filtering to see individual pixels
				SDL_SetTextureScaleMode(m_viewport_texture, SDL_SCALEMODE_NEAREST);

				reset(new_size * new_size);
			}

			// Simulation controls
			if (m_simulation_running)
			{
				if (ImGui::Button("Pause"))
				{
					m_simulation_running = false;
				}
			}
			else
			{
				if (ImGui::Button("Play"))
				{
					m_simulation_running = true;
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Reset"))
			{
				reset((int)m_viewport_texture->w * (int)m_viewport_texture->h);
			}
			ImGui::SameLine();
			ImGui::Text("Points per frame: ");
			ImGui::SliderInt("##PointsPerFrame", &m_points_per_frame, 1, 1000);
			ImGui::SameLine();
			ImGui::InputInt("##PointsPerFrameInput", &m_points_per_frame, 1, 1000);

			ImGui::Separator();

			// Color configuration
			ImGui::Text("Colors:");
			if (ImGui::ColorEdit3("Inside Circle", (float *)&m_inside_color))
			{
				reset((int)m_viewport_texture->w * (int)m_viewport_texture->h);
			}
			if (ImGui::ColorEdit3("Outside Circle", (float *)&m_outside_color))
			{
				reset((int)m_viewport_texture->w * (int)m_viewport_texture->h);
			}

			ImGui::Separator();

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

			ImGui::Text("Texture size: %d x %d", (int)m_viewport_texture->w, (int)m_viewport_texture->h);

			ImGui::Separator();
			ImGui::Text("Monte Carlo Pi Estimation:");
			ImGui::Text("Total points: %llu", total_points);
			ImGui::Text("Points inside circle: %llu", inside_circle);
			ImGui::Text("Points outside circle: %llu", total_points - inside_circle);

			if (total_points > 0)
			{
				double ratio = (double)inside_circle / (double)total_points;
				double estimated_pi = ratio * 4.0;
				double actual_pi = 3.141592653589793;
				double error = abs(estimated_pi - actual_pi);
				double error_percent = (error / actual_pi) * 100.0;

				ImGui::Text("Ratio (inside/total): %.15f", ratio);
				ImGui::Text("Formula: Pi ≈ (inside/total) × 4");
				ImGui::Text("Estimated Pi: %.15f", estimated_pi);
				ImGui::Text("Actual Pi:    %.15f", actual_pi);
				ImGui::Text("Error: %.15f (%.6f%%)", error, error_percent);
			}
			else
			{
				ImGui::Text("Ratio (inside/total): 0.000000000000000");
				ImGui::Text("Formula: Pi ≈ (inside/total) × 4");
				ImGui::Text("Estimated Pi: 0.000000000000000");
				ImGui::Text("Actual Pi:    3.141592653589793");
				ImGui::Text("Error: N/A");
			}
			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		SDL_SetRenderScale(m_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
		SDL_SetRenderDrawColorFloat(m_renderer, m_clear_color.x, m_clear_color.y, m_clear_color.z, m_clear_color.w);
		SDL_RenderClear(m_renderer);
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
		SDL_RenderPresent(m_renderer);
	}
}

void App::reset(uint32_t pixel_count)
{
	total_points = 0;
	inside_circle = 0;
	m_viewport_data.resize(pixel_count);
	m_accumulation_r.resize(pixel_count);
	m_accumulation_g.resize(pixel_count);
	m_accumulation_b.resize(pixel_count);
	m_sample_counts.resize(pixel_count);

	std::ranges::fill(m_accumulation_r, 0);
	std::ranges::fill(m_accumulation_g, 0);
	std::ranges::fill(m_accumulation_b, 0);
	std::ranges::fill(m_sample_counts, 0);
	std::fill(m_viewport_data.begin(), m_viewport_data.end(), ToColor(0, 0, 0, 255));
	SDL_UpdateTexture(m_viewport_texture, nullptr, m_viewport_data.data(), (uint32_t)m_viewport_texture->w * 4);
}

void SetDarkThemeColors()
{
	ImGuiStyle &style = ImGui::GetStyle();
	ImVec4 *colors = style.Colors;

	// Keep the same spacing and rounding
	style.WindowRounding = 6.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowPadding = ImVec2(12, 12);
	style.FramePadding = ImVec2(6, 4);
	style.FrameRounding = 4.0f;
	style.ItemSpacing = ImVec2(8, 6);
	style.ItemInnerSpacing = ImVec2(6, 4);
	style.IndentSpacing = 22.0f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 8.0f;
	style.GrabMinSize = 12.0f;
	style.GrabRounding = 3.0f;
	style.PopupRounding = 4.0f;

	// Base colors
	colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);			// Light grey text (not pure white)
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // Medium grey for disabled
	colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.95f);		// Dark m_window background
	colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.95f);		// Match m_window background
	colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.95f);		// Slightly darker than m_window
	colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);		// Dark grey border
	colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);		// No shadow

	// Frame colors
	colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 0.95f);		  // Dark element backgrounds
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 0.95f); // Slightly lighter on hover
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);  // Even lighter when active

	// Title bar colors
	colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);			// Dark grey inactive title
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);	// Slightly lighter active title
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 0.75f); // Transparent when collapsed
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);		// Slightly darker than title

	// Scrollbar colors
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.95f);			// Scrollbar background
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);		// Scrollbar grab
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f); // Scrollbar grab when hovered
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);	// Scrollbar grab when active

	// Widget colors
	colors[ImGuiCol_CheckMark] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);		// Light grey checkmark
	colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);		// Slider grab
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f); // Slider grab when active
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 0.80f);			// Dark grey buttons
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);	// Slightly lighter on hover
	colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);		// Even lighter when active

	// Header colors (TreeNode, Selectable, etc)
	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 0.76f);		 // Pure dark grey
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.80f); // Slightly lighter on hover
	colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);	 // Even lighter when active

	// Separator
	colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);		// Separator color
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f); // Separator when hovered
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);	// Separator when active

	// Resize grip
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);		 // Resize grip
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.30f, 0.30f, 0.30f, 0.75f); // Resize grip when hovered
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);	 // Resize grip when active

	// Text input cursor
	colors[ImGuiCol_InputTextCursor] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f); // Text input cursor

	// ALL TAB COLORS (both old and new names)
	// Using the newer tab color naming from your enum
	colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 0.86f);						 // Unselected tab
	colors[ImGuiCol_TabHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.80f);				 // Tab when hovered
	colors[ImGuiCol_TabSelected] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);				 // Selected tab
	colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);		 // Selected tab overline
	colors[ImGuiCol_TabDimmed] = ImVec4(0.13f, 0.13f, 0.13f, 0.86f);				 // Dimmed/unfocused tab
	colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);		 // Selected but unfocused tab
	colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // Overline of unfocused selected tab

	// For backward compatibility with older ImGui versions
	// These might be what your version is using
	if (ImGuiCol_TabActive != ImGuiCol_TabSelected)
	{																			  // Only set if they're different (to avoid warnings)
		colors[ImGuiCol_TabActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);		  // Active tab (old name)
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.13f, 0.13f, 0.86f);		  // Unfocused tab (old name)
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // Unfocused active tab (old name)
	}

	// Docking colors
	colors[ImGuiCol_DockingPreview] = ImVec4(0.30f, 0.30f, 0.30f, 0.40f); // Preview when docking
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f); // Empty docking space

	// Plot colors
	colors[ImGuiCol_PlotLines] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);			// Plot lines
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);		// Plot lines when hovered
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);		// Plot histogram
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f); // Plot histogram when hovered

	// Table colors
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);	 // Table header background
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f); // Table outer borders
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);	 // Table inner borders
	colors[ImGuiCol_TableRowBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.90f);		 // Table row background (even)
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.16f, 0.16f, 0.16f, 0.90f);	 // Table row background (odd)

	// Miscellaneous
	colors[ImGuiCol_TextLink] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);				 // Light grey for links (not blue)
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.30f, 0.30f, 0.30f, 0.35f);		 // Light grey selection background
	colors[ImGuiCol_TreeLines] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);			 // Tree node lines
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);		 // Drag and drop target
	colors[ImGuiCol_NavCursor] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);			 // Navigation cursor
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.40f, 0.40f, 0.40f, 0.70f); // Nav windowing highlight
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);	 // Nav windowing dim
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.75f);		 // Modal m_window dim
}
