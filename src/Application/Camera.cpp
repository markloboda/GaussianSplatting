#include <Core/Core.h>
#include <Application/Camera.h>

#include <Application/InputManager.h>
#include <Utils/FileReader.h>

void Camera::UpdateCameraFree(float dt) {
   if (InputManager::IsInputDown(KEY_LEFT_ALT) || InputManager::IsInputDown(MOUSE_BUTTON_RIGHT)) {
      vec3 worldUp = vec3(0.0f, 1.0f, 0.0f);
      vec3 right = normalize(cross(worldUp, _forward));

      // Movement
      if (InputManager::IsInputDown(KEY_W)) {
         _position += _speed * _forward * dt;
      }
      if (InputManager::IsInputDown(KEY_S)) {
         _position -= _speed * _forward * dt;
      }
      if (InputManager::IsInputDown(KEY_A)) {
         _position -= _speed * right * dt;
      }
      if (InputManager::IsInputDown(KEY_D)) {
         _position += _speed * right * dt;
      }
      if (InputManager::IsInputDown(KEY_E)) {
         _position += _speed * _up * dt;
      }
      if (InputManager::IsInputDown(KEY_Q)) {
         _position -= _speed * _up * dt;
      }

      // Rotation
      // Get pitch and yaw angle
      float pitch = asin(_forward.y);
      float yaw = atan2(_forward.z, _forward.x);

      // Get mouse movement
      vec2 mouseMove = InputManager::GetCursorMove();
      float dYaw = radians(-mouseMove.x * _sensitivity);
      float dPitch = radians(mouseMove.y * _sensitivity);

      // Update yaw and pitch
      pitch = clamp(pitch + dPitch, -PI / 2.0f + 0.01f, PI / 2.0f - 0.01f);
      yaw += dYaw;

      // Update forward and up vectors
      _forward = normalize(vec3(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch)));
      right = normalize(cross(worldUp, _forward));
      _up = normalize(cross(right, _forward));
   }
}

void Camera::UpdateCameraOrbit(float dt) {
   vec3 worldUp = vec3(0.0f, 1.0f, 0.0f);

   if (InputManager::IsInputDown(MOUSE_BUTTON_LEFT)) {
      CalculateOrbit();
   }

   if (InputManager::IsInputDown(MOUSE_BUTTON_MIDDLE)) {
      vec2 mouseMove = InputManager::GetCursorMove();
      vec3 right = normalize(cross(worldUp, _forward));
      _target += (-right * mouseMove.x + _up * mouseMove.y) * dt * _panSpeed;
      _position = _target - _forward * _distance;
   }

   float zoomInput = static_cast<float>(InputManager::IsInputDown(KEY_R)) - static_cast<float>(InputManager::IsInputDown(KEY_F));
   if (zoomInput != 0.0f) {
      _distance = clamp(_distance - zoomInput * dt * _zoomSpeed, _minDistance, _maxDistance);
      _position = _target - _forward * _distance;
   }
}

void Camera::CalculateOrbit() {
   vec3 worldUp = vec3(0.0f, 1.0f, 0.0f);
   vec2 mouseMove = InputManager::GetCursorMove();
   float dYaw = radians(-mouseMove.x * _sensitivity);
   float dPitch = radians(mouseMove.y * _sensitivity);

   _yaw += dYaw;
   _pitch = clamp(_pitch + dPitch, -PI / 2.0f + 0.01f, PI / 2.0f - 0.01f);

   _forward = normalize(vec3(cos(_yaw) * cos(_pitch),
                             sin(_pitch),
                             sin(_yaw) * cos(_pitch)));

   vec3 right = normalize(cross(worldUp, _forward));
   _up = normalize(cross(right, _forward));

   _position = _target - _forward * _distance;
}
