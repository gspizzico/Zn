#include <Znpch.h>
#include <Editor/Editor.h>
#include <Automation/AutomationTestManager.h>
#include <Core/Containers/Map.h>

#include <ImGui/imgui.h> //#todo handle editor UI separately

namespace Zn::UI
{
	using namespace Zn;

	bool GAutomationWindow = false;
	bool GIsRequestingExit = false;

	Map<Name, bool> GSelectedTests;

	void DrawMainMenu()
	{
		ImGui::BeginMainMenuBar();

		bool openExitPopup = false;
		bool openAutomationWindow = false;

		if (ImGui::BeginMenu("Zn"))
		{
			if (ImGui::MenuItem("Exit", NULL))
			{
				openExitPopup = true;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools"))
		{
			if (ImGui::MenuItem("Automation", NULL))
			{
				GAutomationWindow = true;
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();

		if (openExitPopup)
		{
			ImGui::OpenPopup("editor-exit");
		}

		if (ImGui::BeginPopup("editor-exit"))
		{
			ImGui::Text("Are you sure you want to exit?");

			if (ImGui::Button("Yes"))
			{
				GIsRequestingExit = true;
			}

			if (ImGui::Button("No"))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void DrawAutomationWindow()
	{
		if (GAutomationWindow)
		{
			ImGui::Begin("Automation", &GAutomationWindow);

			bool ShowButton = false;

			auto& AutomationTestManager = Automation::AutomationTestManager::Get();

			if (ImGui::TreeNode("Startup Tests"))
			{
				Vector<Name> TestNames = AutomationTestManager.GetStartupTestsNames();

				for (const Name& name : TestNames)
				{
					auto EmplaceResult = GSelectedTests.try_emplace(name, false);

					auto& KvpReference = *EmplaceResult.first;

					ImGui::Selectable(name.CString(), &KvpReference.second);

					ShowButton |= KvpReference.second;
				}

				ImGui::TreePop();
			}

			if (ShowButton && ImGui::Button("Clear Selection"))
			{
				for (auto& Kvp : GSelectedTests)
				{
					Kvp.second = false;
				}
			}

			if (ShowButton && !AutomationTestManager.IsRunningTests() && ImGui::Button("Start Tests"))
			{
				Vector<Name> TestsToRun;
				for (const auto& Kvp : GSelectedTests)
				{
					if (Kvp.second)
					{
						TestsToRun.emplace_back(Kvp.first);
					}
				}

				AutomationTestManager.EnqueueTests(TestsToRun);
			}

			ImGui::End();
		}
	}
}

using namespace Zn;

Editor& Editor::Get()
{
	static Editor Instance{};
	return Instance;
}

void Editor::PreUpdate(float deltaTime)
{
	// Draw editor UI

	ZN_TRACE_QUICKSCOPE();

	//Zn::UI::DrawMainMenu();
	//
	//Zn::UI::DrawAutomationWindow();

}

void Editor::Update(float deltaTime)
{

}

void Editor::PostUpdate(float deltaTime)
{

}

bool Editor::IsRequestingExit() const
{
	return Zn::UI::GIsRequestingExit;
}
