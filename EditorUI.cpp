#include "EditorUI.h"

#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

// Начало нового кадра ImGui — нужно вызывать каждый рендер-цикл
void EditorUI::beginFrame()
{
	ImGui_ImplOpenGL3_NewFrame(); // подготавливаем OpenGL3 бэкенд
	ImGui_ImplGlfw_NewFrame(); // подготавливаем GLFW бэкенд
	ImGui::NewFrame(); // создаем новый кадр ImGui
}

// Отрисовка UI и вывод на экран
void EditorUI::render(Model* model, bool& isolateSelectedMesh)
{
    drawModelWindow(); // новое окно загрузки модели

    if (model)
    {
        int selected = model->getSelectedMesh();
        if (selected != -1) // если есть выбранный меш
        {
            std::string selectedInfo = model->getMeshInfo(selected);

            ImGuiIO& io = ImGui::GetIO();
            ImVec2 infoPos(io.DisplaySize.x - 300, 10); // справа, 300 пикселей от края
            ImGui::SetNextWindowPos(infoPos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(280, 150)); // ширина окна

            ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                     ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoScrollbar |
                                     ImGuiWindowFlags_NoCollapse;

            ImGui::Begin("Mesh Info", nullptr, flags);
            ImGui::TextWrapped("%s", selectedInfo.c_str());
            ImGui::End();
        }
    }

    ImVec2 windowPos(10.0f, ImGui::GetIO().DisplaySize.y - 150.0f);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);

    ImGui::Begin("Additional functions:", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_AlwaysAutoResize);
    
    if (ImGui::Button("Show All Meshes"))
    {
        if (model)
        {
            model->selectMesh(-1);
            model->showAllMeshes();
            isolateSelectedMesh = false;
        }
    }

    if (ImGui::Checkbox("Isolate selected mesh", &isolateSelectedMesh))
    {
        if (model)
        {
            if (isolateSelectedMesh)
                model->isolateSelectedMesh();
            else
                model->showAllMeshes();
        }
    }

    if (!isolateSelectedMesh)
    {
        ImGui::TextDisabled("Click selects a mesh without hiding the others.");
    }

    else if (model && model->getSelectedMesh() < 0)
    {
        ImGui::TextDisabled("Select a mesh by clicking on the model.");
    }

    ImGui::End();

    ImGui::Render(); // подготавливаем отрисовку
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // отрисовываем через OpenGL
}

void EditorUI::drawModelWindow()
{
    // Применяем позицию для второго окна
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);

    ImGui::Begin(
        "Model Loader",
        nullptr,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize
    );

    if (ImGui::Button("3D_Model", ImVec2(120, 40)))
        loadModelRequested = true;

    ImGui::End();
}