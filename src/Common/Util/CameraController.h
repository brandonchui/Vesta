#pragma once
#include "../Config.h"
#include "../IInput.h"
#include "VTMath.h"

class ICameraController {
  public:
    virtual ~ICameraController() = default;

    virtual void Update(IInput* pInput, float deltaTime) = 0;
    virtual Matrix4x4 GetViewMatrix() const = 0;

    virtual Vector3 GetPosition() const = 0;
    virtual Vector3 GetDirection() const = 0;
    virtual void SetState(const Vector3& position, const Vector3& direction) = 0;
};

class VT_API OrbitCameraController : public ICameraController {
  public:
    OrbitCameraController(const Vector3& target = Vector3(0.0f, 0.0f, 0.0f), float distance = 5.0f);

    void Update(IInput* pInput, float deltaTime) override;
    Matrix4x4 GetViewMatrix() const override;

    Vector3 GetPosition() const override;
    Vector3 GetDirection() const override;
    void SetState(const Vector3& position, const Vector3& direction) override;

  private:
    void UpdateViewMatrix();

    Matrix4x4 mViewMatrix;
    Vector3 mTarget;
    Vector3 mPosition;
    float mDistance;
    float mYaw;
    float mPitch;
    bool mIsOrbiting;
    Vec2 mLastMousePos;
};

class VT_API FpsCameraController : public ICameraController {
  public:
    FpsCameraController(const Vector3& position = Vector3(0.0f, 0.0f, 0.0f));

    void Update(IInput* pInput, float deltaTime) override;
    Matrix4x4 GetViewMatrix() const override;

    Vector3 GetPosition() const override;
    Vector3 GetDirection() const override;
    void SetState(const Vector3& position, const Vector3& direction) override;

  private:
    void UpdateViewMatrix();

    Matrix4x4 mViewMatrix;
    Vector3 mPosition;
    float mYaw;
    float mPitch;
    float mMovementSpeed = 5.0f;
    float mRotationSpeed = 0.005f;
};
