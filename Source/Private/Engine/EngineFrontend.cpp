#include <Znpch.h>
#include <Engine/EngineFrontend.h>
#include <Automation/AutomationTestManager.h>
#include <imgui.h> //#todo handle editor UI separately
#include <Rendering/Renderer.h>

void Zn::EngineFrontend::DrawMainMenu()
{
    ImGui::BeginMainMenuBar();

    bool openExitPopup        = false;
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
            bAutomationWindow = true;
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
            bIsRequestingExit = true;
        }

        if (ImGui::Button("No"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    bool out;
    ImGui::Begin("Input", &out);

    float l[3] {light.x, light.y, light.z};

    ImGui::InputFloat3("Light Position", l);

    light.x = l[0];
    light.y = l[1];
    light.z = l[2];

    ImGui::InputFloat("Light Intensity", &intensity);
    ImGui::InputFloat("Light Distance", &distance);

    Renderer::Get().set_light(light, distance, intensity);

    ImGui::End();
}

void Zn::EngineFrontend::DrawAutomationWindow()
{
    if (bAutomationWindow)
    {
        ImGui::Begin("Automation", &bAutomationWindow);

        bool showButton = false;

        auto& automationTestManager = Automation::AutomationTestManager::Get();

        if (ImGui::TreeNode("Startup Tests"))
        {
            Vector<Name> testNames = automationTestManager.GetStartupTestsNames();

            for (const Name& name : testNames)
            {
                auto emplaceResult = SelectedTests.try_emplace(name, false);

                auto& kvpReference = *emplaceResult.first;

                ImGui::Selectable(name.CString(), &kvpReference.second);

                showButton |= kvpReference.second;
            }

            ImGui::TreePop();
        }

        if (showButton && ImGui::Button("Clear Selection"))
        {
            for (auto& kvp : SelectedTests)
            {
                kvp.second = false;
            }
        }

        if (showButton && !automationTestManager.IsRunningTests() && ImGui::Button("Start Tests"))
        {
            Vector<Name> testsToRun;
            for (const auto& kvp : SelectedTests)
            {
                if (kvp.second)
                {
                    testsToRun.emplace_back(kvp.first);
                }
            }

            automationTestManager.EnqueueTests(testsToRun);
        }

        ImGui::End();
    }
}
