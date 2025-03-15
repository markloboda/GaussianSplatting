#include <GaussianSplatting.h>
#include <Application/InputManager.h>

#include <GLFW/glfw3.h>

double mouseX;
double mouseY;

void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
   mouseX = xpos;
   mouseY = ypos;
}

void InputManager::Initialize(GLFWwindow* window) {
   _window = window;
   glfwSetCursorPosCallback(window, CursorPositionCallback);
}

void InputManager::Terminate() {
   _window = nullptr;
}

void InputManager::Update() {
   _moveCursor = vec2(mouseX, mouseY) - _prevCursor;
   _prevCursor = vec2(mouseX, mouseY);
}

bool InputManager::IsInputDown(EInput key) {
   GLFWwindow* window = GetInstance()._window;
   // Check if mouse key.
   if (key >= 0 && key <= 7) {
      return glfwGetMouseButton(window, key) == GLFW_PRESS;
   }

   return glfwGetKey(window, key) == GLFW_PRESS;
}

vec2 InputManager::GetCursorMove() {
   return GetInstance()._moveCursor;
}




