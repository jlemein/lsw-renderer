//
// Created by jeffrey on 12-12-21.
//
#include <imgui.h>
#include <izzgl_entityfactory.h>
#include <izzgl_material.h>
#include <izzgl_materialsystem.h>
#include <izzgui_materialeditor.h>
#include <spdlog/spdlog.h>
using namespace izz::gui;

static int activeTab = 0;
static std::array<std::string, 2> tabs{"Materials", "Shaders"};

MaterialEditor::MaterialEditor(std::shared_ptr<gl::MaterialSystem> materialSystem, std::shared_ptr<izz::EntityFactory> sceneGraph)
  : m_materialSystem{materialSystem}
  , m_sceneGraph{sceneGraph}
  , m_shaderEditor{sceneGraph} {}

void MaterialEditor::init() {
  m_shaderEditor.init();
}

void MaterialEditor::render(float time, float dt) {
  if (MaterialEditor::Show) {
    if (ImGui::Begin(MaterialEditor::ID, &MaterialEditor::Show)) {
      // -- tabs ---
      for (int i=0; i<tabs.size(); ++i) {
        if (i==activeTab) {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2F, 0.389F, 0.619F, 1));
        } else {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.137F, 0.267F, 0.424F, 1));
        }

        if (ImGui::Button(tabs[i].c_str(), ImVec2(150, 25))) {
          activeTab = i;
        }

        ImGui::SameLine(0, 2);
        ImGui::PopStyleColor(1);
      }
      ImGui::NewLine();

      if (activeTab == 0) {
        drawMaterialTable();
      }
      if (activeTab == 1) {
        drawShaderProgramTable();
      }
    }

    ImGui::End();
  }

  m_shaderEditor.render(dt, time);
}

void MaterialEditor::drawMaterialTable() {
  constexpr int NUM_COLUMNS = 3;
  constexpr const char* columns[] = {
      "Material ID", "Material",  "Actions",
  };
  auto flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders;
  if (ImGui::BeginTable("Table1", NUM_COLUMNS, flags, ImVec2(0, 0), 0)) {
    // columns of the table
    for (auto name : columns) {
      ImGui::TableSetupColumn(name);
    }
    ImGui::TableHeadersRow();

    auto materials = m_materialSystem->getCreatedMaterials();
    if (materials.empty()) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("No materials to display");
    }

    for (const auto& [id, material] : materials) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(std::to_string(id).c_str());

      ImGui::TableNextColumn();
      ImGui::PushID(static_cast<int>(id));
      ImGui::TextUnformatted(material.name.c_str());

      ImGui::TableNextColumn();
      if (ImGui::Button("Edit")) {
        // m_shaderEditor.openDialog(e);
        spdlog::info("Open material editor for program id -- material id: {}", material.id);
      }
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
}


void MaterialEditor::drawShaderProgramTable() {
  constexpr int NUM_COLUMNS = 7;
  constexpr const char* columns[] = {
      "Program ID", "Name", "Vertex Shader", "Geometry Shader", "Fragment Shader", "Instance count", "Actions",
  };
  auto flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders;
  if (ImGui::BeginTable("ShaderProgramTable", NUM_COLUMNS, flags, ImVec2(0, 0), 0)) {
    // columns of the table
    for (auto name : columns) {
      ImGui::TableSetupColumn(name);
    }
    ImGui::TableHeadersRow();

    auto shaderPrograms = m_materialSystem->getShaderPrograms();
    if (shaderPrograms.empty()) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted("No programs to display");
    }

    for (const auto& [id, shader] : shaderPrograms) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(std::to_string(id).c_str());

      ImGui::TableNextColumn();
      ImGui::TextUnformatted(shader.materialTemplateName.c_str());

      auto materialTemplate = m_materialSystem->getMaterialTemplate(shader.materialTemplateName);
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(materialTemplate.vertexShader.c_str());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(materialTemplate.geometryShader.c_str());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(materialTemplate.fragmentShader.c_str());


      ImGui::TableNextColumn();
      ImGui::PushID(static_cast<int>(id));
      ImGui::TextUnformatted(std::to_string(shader.numberOfInstances).c_str());

      ImGui::TableNextColumn();
      if (ImGui::Button("Edit")) {
        // m_shaderEditor.openDialog(e);
        spdlog::info("Open shader editor for program id: {}", id);
      }
      ImGui::PopID();
    }
    ImGui::EndTable();
  }
}

void MaterialEditor::Open() {
  MaterialEditor::Show = true;
}

void MaterialEditor::Toggle() {
  MaterialEditor::Show = !MaterialEditor::Show;
}