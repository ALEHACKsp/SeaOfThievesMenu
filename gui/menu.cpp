#include "../framework/pch-sdk.h"
#include "menu.hpp"
#include "../includes/imgui/imgui.h"
#include "../user/state.hpp"

namespace Menu {
	void Init() {
#ifdef _DEBUG
		ImGui::SetNextWindowSize(ImVec2(500, 280), ImGuiCond_Once);
#else
		ImGui::SetNextWindowSize(ImVec2(440, 280), ImGuiCond_Once);
#endif
		ImGui::SetNextWindowBgAlpha(1.F);
	}

	bool init = false;
	bool firstRender = true;
	void Render() {
		if (!init)
			Menu::Init();

		ImGui::Begin("SeaOfThievesMenu", &State.ShowMenu, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
		ImGui::BeginTabBar("SeaOfThieves#TopBar", ImGuiTabBarFlags_NoTabListScrollingButtons);

		if(firstRender)
			firstRender = false;

		ImGui::Text("Its working");

		ImGui::EndTabBar();
		ImGui::End();
	}
}