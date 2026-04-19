#include "EditorUI.h"

#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

// Глобальные параметры света из Main.cpp
extern glm::vec3 lightPos1;
extern glm::vec3 lightColor1;
extern glm::vec3 lightPos2;
extern glm::vec3 lightColor2;

extern float lightIntensity1;
extern float lightIntensity2;

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

    // Окно управления светом
    ImVec2 windowlightSize(320.0f, 0.0f); // нормальная ширина
    ImVec2 windowlightPos(
        ImGui::GetIO().DisplaySize.x - windowlightSize.x - 10.0f,
        ImGui::GetIO().DisplaySize.y - 10.0f
    );

    ImGui::SetNextWindowPos(windowlightPos, ImGuiCond_Always, ImVec2(0.0f, 1.0f)); // якорь снизу
    ImGui::SetNextWindowSize(windowlightSize, ImGuiCond_Always);

    ImGui::Begin(
        "Lighting",
        nullptr,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize
    );

    ImGui::Text("Main Light");
    ImGui::DragFloat3("Position 1", &lightPos1[0], 0.1f, -20.0f, 20.0f);
    ImGui::ColorEdit3("Color 1", &lightColor1[0]);
    ImGui::DragFloat("Intensity 1", &lightIntensity1, 0.05f, 0.0f, 5.0f);

    ImGui::Separator();

    ImGui::Text("Fill Light");
    ImGui::DragFloat3("Position 2", &lightPos2[0], 0.1f, -20.0f, 20.0f);
    ImGui::ColorEdit3("Color 2", &lightColor2[0]);
    ImGui::DragFloat("Intensity 2", &lightIntensity2, 0.05f, 0.0f, 5.0f);

    ImGui::End();

    // Окно дополнительных функций
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