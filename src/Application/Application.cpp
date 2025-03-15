#include <GaussianSplatting.h>
#include <Application/Application.h>

#include <GLFW/glfw3.h>

#include <Application/Renderer.h>
#include <Application/Camera.h>
#include <Application/InputManager.h>
#include <Utils/FileReader.h>

#include "imgui.h"

bool Application::Initialize() {
   _filename = "../../../assets/splats/nike.splat";

   // Open window.
   if (!glfwInit()) {
      std::cerr << "Could not initialize GLFW!" << std::endl;
      return false;
   }
   glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Disable the graphics API, as we handle it ourselves.
   glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
   _window = glfwCreateWindow(_windowWidth, _windowHeight, "Gaussian Splatting", nullptr, nullptr);
   if (!_window) {
      std::cerr << "Could not open window!" << std::endl;
      glfwTerminate();
      return false;
   }

   // Initialize input manager.
   InputManager::GetInstance().Initialize(_window);

   if (!InitializeRenderer())
   {
      return false;
   }

   // Create camera.
   _camera = new Camera();
   _camera->SetAspectRatio(static_cast<float>(_windowWidth) / static_cast<float>(_windowHeight));

   vec4 modelPosition = _renderer->GetModelPosition();
   _camera->SetOrbitTarget(modelPosition);

   return true;
}

void Application::Terminate() const {
   InputManager::GetInstance().Terminate();
   _renderer->Terminate();
   delete _renderer;
   delete _camera;
   glfwDestroyWindow(_window);
}

void Application::Run() {
   double lastTime = glfwGetTime();
   while (IsRunning()) {
      const double currentTime = glfwGetTime();
      const float dt = static_cast<float>(currentTime - lastTime);
      lastTime = currentTime;

      glfwPollEvents();

      InputManager::GetInstance().Update();

      bool imguiMouseCaptured = ImGui::GetIO().WantCaptureMouse;
      if (!imguiMouseCaptured)
      {
         if (_renderer->FreeCamera)
         {
            _camera->UpdateCameraFree(dt);
         }
         else
         {
            _camera->UpdateCameraOrbit(dt);
         }
      }

      _renderer->Render(*_camera);

      // Check if file change.
      if (_renderer->ChangeSplatsFlag)
      {
         _renderer->ChangeSplatsFlag = false;
         std::string filename = std::string("../../../assets/splats/") + std::string(_renderer->SelectedFile);
         _filename = filename.c_str();
         _selectedFileIndex = _renderer->SelectedFileIndex;
         _renderer->Terminate();
         delete _renderer;
         if (!InitializeRenderer())
         {
            return;
         }

         _renderer->SelectedFileIndex = _selectedFileIndex;
      }
   }
}

bool Application::IsRunning() const {
   return !glfwWindowShouldClose(_window);
}

bool Application::InitializeRenderer()
{
   // Initialize renderer.
   _renderer = new Renderer();
   const bool success = _renderer->Initialize(_window, _windowWidth, _windowHeight, _filename);
   if (!success) {
      std::cerr << "Could not initialize renderer!" << std::endl;
      return false;
   }

   return true;
}
