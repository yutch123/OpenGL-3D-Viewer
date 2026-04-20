#include "EditorUI.h"

#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

#include <cstring>

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
        if (selected != -1)
        {
            std::string selectedInfo = model->getMeshNote(selected);

            static int lastSelectedMesh = -1;
            static char noteBuffer[2048] = "";
            static bool noteEditorOpen = false;

            if (selected != lastSelectedMesh)
            {
                std::string note = model->getMeshNote(selected);
                strncpy_s(noteBuffer, sizeof(noteBuffer), note.c_str(), _TRUNCATE);

                noteEditorOpen = !note.empty();
                lastSelectedMesh = selected;
            }

            ImGuiIO& io = ImGui::GetIO();
            ImVec2 infoPos(io.DisplaySize.x - 360, 10);
            ImGui::SetNextWindowPos(infoPos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(340, 300));

            ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse;

            ImGui::Begin(u8"Заметки об элементе", nullptr, flags);

            ImGui::Text(u8"Выбранный элемент:");
            ImGui::Separator();

            ImGui::TextWrapped("%s", selectedInfo.c_str());

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text(u8"Моя заметка:");

            if (!noteEditorOpen)
            {
                ImGui::Spacing();
                ImGui::TextDisabled(u8"Для этого элемента заметка еще не создана.");
                ImGui::Spacing();

                if (ImGui::Button(u8"Добавить заметку", ImVec2(180, 32)))
                {
                    noteEditorOpen = true;
                    noteBuffer[0] = '\0';
                }
            }
            else
            {
                if (ImGui::InputTextMultiline(
                    "##MeshNote",
                    noteBuffer,
                    sizeof(noteBuffer),
                    ImVec2(-1.0f, 130.0f)))
                {
                    model->setMeshNote(selected, std::string(noteBuffer));
                }

                ImGui::Spacing();

                if (ImGui::Button(u8"Сохранить", ImVec2(100, 28)))
                {
                    model->setMeshNote(selected, std::string(noteBuffer));
                    model->saveNotes(model->getNotesFilePath());
                    noteEditorOpen = false;
                }

                ImGui::SameLine();

                if (ImGui::Button(u8"Очистить", ImVec2(100, 28)))
                {
                    noteBuffer[0] = '\0';
                    model->setMeshNote(selected, "");
                }
            }

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
        u8"Свет",
        nullptr,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize
    );

    ImGui::Text(u8"Основной свет");
    ImGui::DragFloat3(u8"Позиция 1", &lightPos1[0], 0.1f, -20.0f, 20.0f);
    ImGui::ColorEdit3(u8"Цвет 1", &lightColor1[0]);
    ImGui::DragFloat(u8"Интенсивность 1", &lightIntensity1, 0.05f, 0.0f, 5.0f);

    ImGui::Separator();

    ImGui::Text(u8"Дополнительный свет");
    ImGui::DragFloat3(u8"Позиция 2", &lightPos2[0], 0.1f, -20.0f, 20.0f);
    ImGui::ColorEdit3(u8"Цвет 2", &lightColor2[0]);
    ImGui::DragFloat(u8"Интенсивность 2", &lightIntensity2, 0.05f, 0.0f, 5.0f);

    ImGui::End();

    // Окно дополнительных функций
    ImVec2 windowPos(10.0f, ImGui::GetIO().DisplaySize.y - 10.0f);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(0.0f, 1.0f));

    ImGui::Begin(u8"Дополнительные функции:", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_AlwaysAutoResize);
    
    if (ImGui::Button(u8"Показать все элементы"))
    {
        if (model)
        {
            model->selectMesh(-1);
            model->showAllMeshes();
            isolateSelectedMesh = false;
        }
    }

    if (ImGui::Checkbox(u8"Скрыть элементы", &isolateSelectedMesh))
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
        ImGui::TextDisabled(u8"Поставьте галочку, чтобы включить режим разделения");
    }

    else if (model && model->getSelectedMesh() < 0)
    {
        ImGui::TextDisabled(u8"Нажмите на элемент модели.");
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