// Build file for ImGui
#pragma warning(push)
#pragma warning(disable: 6011 6031 28182 26819 4062 4062)

#include "vktpch.h"

#define IMGUI_API __declspec(dllexport)

#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"

#include "backends/imgui_impl_opengl3.cpp"
#include "backends/imgui_impl_glfw.cpp"

#include "ImGuizmo.cpp"

#pragma warning(pop)