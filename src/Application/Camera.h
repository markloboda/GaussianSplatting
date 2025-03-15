#pragma once

struct Splat;
struct GLFWwindow;

class Camera {
private:
   vec3 _position;
   vec3 _forward;
   vec3 _up;
   float _fovDegrees;
   float _nearPlane;
   float _farPlane;

   float _speed;
   float _sensitivity;

   vec3 _target;
   float _yaw;
   float _pitch;
   float _distance;
   float _panSpeed;
   float _zoomSpeed;
   float _minDistance;
   float _maxDistance;

   float _aspectRatio;

public:
   Camera() : _position(0.0f, 0.0f, 0.0f),
              _forward(0.0f, 0.0f, 1.0f),
              _up(0.0f, 1.0f, 0.0f),
              _fovDegrees(45.0f),
              _nearPlane(0.1f),
              _farPlane(100.0f),
              _speed(5.0f),
              _sensitivity(0.1f),
              _target(vec3(0.0, 0.0, 0.0)),
              _yaw(0.0f),
              _pitch(0.0f),
              _distance(5.0f),
              _panSpeed(0.1f),
              _zoomSpeed(1.0f),
              _minDistance(1.0f),
              _maxDistance(10.0f),
              _aspectRatio(16.0f / 9.0f)
   {
   }

   void UpdateCameraFree(float dt);

   void UpdateCameraOrbit(float dt);

   void SetPosition(const vec3 &position) {
      _position = position;
   }

   void SetAspectRatio(float aspectRatio) {
      _aspectRatio = aspectRatio;
   }

   mat4x4 GetProjectionMatrix() const {
      float fovRadians = radians(_fovDegrees);
      return perspective(fovRadians, _aspectRatio, _nearPlane, _farPlane);
   }

   mat4x4 GetViewMatrix() const {
      return lookAt(_position, _position + _forward, _up);
   }

   void SetOrbitTarget(const vec3 &target) {
      _target = target;
      CalculateOrbit();
   }

private:
   void CalculateOrbit();
};
