#pragma once

union SDL_Event;
struct SDL_Window;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace Zn
{
void imgui_initialize();
void imgui_shutdown();
bool imgui_process_event(SDL_Event& event);
bool imgui_begin_frame();
bool imgui_end_frame();
} // namespace Zn
