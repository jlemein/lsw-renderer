//
// Created by jlemein on 11-03-21.
//
#include <georm_materialsystem.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <vwr_viewer.h>
//#include <res_resourcemanager.h>
#include <anim_localrotation.h>
#include <core_util.h>
#include <ecs_firstpersoncontrol.h>
#include <ecs_transformutil.h>
#include <ecsg_scenegraph.h>
#include <geo_primitivefactory.h>
#include <georm_materialsystem.h>
#include <georm_resourcemanager.h>
#include <gui_system.h>
#include <gui_lighteditor.h>
#include <georm_fontsystem.h>
#include <ecs_light.h>
#include <cxxopts.hpp>
#include <wsp_workspace.h>

using namespace std;
using namespace lsw;
using namespace geo;
using lsw::core::Util;
using namespace glm;
using lsw::wsp::Workspace;

Workspace parseProgramArguments(int argc, char* argv[]);

int main(int argc, char* argv[]) {
  auto workspace = parseProgramArguments(argc, argv);
  if (workspace.debugMode) {
    spdlog::set_level(spdlog::level::debug);
  }

  try {
    auto resourceManager = make_shared<georm::ResourceManager>();
    auto fontSystem = make_shared<georm::FontSystem>();
    auto sceneGraph = make_shared<ecsg::SceneGraph>();
    auto materialSystem = make_shared<georm::MaterialSystem>(sceneGraph, resourceManager);

    if (workspace.materialsFile.empty()) {
      spdlog::warn("No materials provided. Rendering results may be different than expected.");
    } else {
      materialSystem->loadMaterialsFromFile(workspace.materialsFile);
    }

    resourceManager->setMaterialSystem(materialSystem);

    auto renderSystem = make_shared<ecs::RenderSystem>(sceneGraph, static_pointer_cast<ecs::IMaterialSystem>(materialSystem));
    auto editor = make_shared<gui::GuiLightEditor>(sceneGraph, fontSystem);
    auto guiSystem = make_shared<GuiSystem>(editor);
    auto viewer = make_shared<viewer::Viewer>(sceneGraph, renderSystem, resourceManager->getRawResourceManager(), guiSystem);

    // ==== SCENE SETUP ======================================================
    spdlog::info("Loading scene file: {}", workspace.sceneFile.c_str());

    // ==== LIGHTS SETUP ====================================================
    sceneGraph->makeDirectionalLight("Sun", glm::vec3(0.F, 1.0F, 1.0F));

    // ==== CAMERA SETUP ====================================================
    auto camera = sceneGraph->makeCamera("DummyCamera", 4);
    camera.add<ecs::FirstPersonControl>().onlyRotateOnMousePress = true;
    viewer->setActiveCamera(camera);

    // ==== UI SETUP ========================================================
    viewer->registerExtension(guiSystem);

    viewer->setWindowSize(1024, 768);
    viewer->setTitle("Normal mapping");
    viewer->initialize();
    viewer->run();
  } catch (runtime_error& e) {
    spdlog::error(e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}


namespace {
#ifndef NDEBUG
const char* DEBUG_MODE = "true";
#else
  const char* DEBUG_MODE = "false";
#endif  // NDEBUG
}  // namespace

Workspace parseProgramArguments(int argc, char* argv[]) {
  const std::string PROGRAM_NAME = "normalmap";
  cxxopts::Options _options(PROGRAM_NAME, "Portfolio demo of normal mapping. \n@Copyright reserved to jlemein.nl\n");
  _options.add_options()
  ("d,debug", "Enable debugging", cxxopts::value<bool>()->default_value(DEBUG_MODE))
  ("s,scene", "A scene file for visualization (*.fbx, *.obj)", cxxopts::value<std::string>())
  ("m,materials", "Reference to a materials json file", cxxopts::value<std::string>()->default_value(""))
  ("w,workspace", "Workspace directory", cxxopts::value<std::string>())
  ("v,version", "Version information")
  ("h,help","Print usage");
  auto result = _options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << _options.help() << std::endl;
    exit(EXIT_SUCCESS);
  }

  if (result.count("version")) {
    std::cout << PROGRAM_NAME << " v" << VERSION_STR << std::endl; // VERSION_STR is defined in CmakeLists.txt
    exit(EXIT_SUCCESS);
  }

  if (!result.count("scene")) {
    spdlog::error("Missing required command line argument --scene.");
    std::cout << _options.help() << std::endl;
    exit(EXIT_FAILURE);
  }

  auto& workspace = wsp::WorkspaceManager::GetWorkspace();
  workspace.sceneFile = result["scene"].as<std::string>();
  workspace.materialsFile = result["materials"].as<std::string>();

  const char* weraHomeEnv = getenv("WERA3D_HOME");
  if (weraHomeEnv) {
    workspace.lsw_home_directory = weraHomeEnv;
    workspace.path = argv[0];
  } else {
    workspace.isStrictMode = true;  // no default materials to read from, which implies strict mode.
  }

  workspace.debugMode = result["debug"].as<bool>();

  cout << "Scene file: " << workspace.sceneFile << endl;
  cout << "Materials file: " << (workspace.materialsFile.empty() ? "<not specified>" : workspace.materialsFile) << endl;
  cout << "Strict mode: " << (workspace.isStrictMode ? "enabled" : "disabled") << endl;

  cout << "Testing R: " << wsp::R("myTexture.jpg") << std::endl;

  return workspace;
}