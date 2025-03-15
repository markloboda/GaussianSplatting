#pragma once

struct GLFWwindow;
class Renderer;
class Camera;

class Application {
private:
   int _windowWidth = 1280;
   int _windowHeight = 720;

   GLFWwindow *_window = nullptr;
   Renderer *_renderer = nullptr;
   Camera *_camera = nullptr;

   const char* _filename = nullptr;
   int _selectedFileIndex = 0;

public:
   bool Initialize();

   void Terminate() const;

   // Runs the main loop.
   void Run();

   [[nodiscard]] bool IsRunning() const;

private:
   bool InitializeRenderer();
};
