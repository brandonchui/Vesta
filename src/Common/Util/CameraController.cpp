#include <cmath>
#include "CameraController.h"

/////////////////////////////////////////////
// Orbit
OrbitCameraController::OrbitCameraController(const Vector3& target, float distance)
    : mTarget(target), mDistance(distance), mYaw(0.0f), mPitch(0.0f), mIsOrbiting(false) {
    mPosition = Vector3(0.0f, 0.0f, -mDistance);
    UpdateViewMatrix();
}

void OrbitCameraController::Update(IInput* pInput, float deltaTime) {
    if (pInput->WasPressed(MouseButton::Left)) {
        mIsOrbiting = true;
        mLastMousePos = pInput->GetMousePosition();
    } else if (pInput->WasReleased(MouseButton::Left)) {
        mIsOrbiting = false;
    }

    if (mIsOrbiting) {
        Vec2 currentMousePos = pInput->GetMousePosition();
        Vec2 delta = { currentMousePos.x - mLastMousePos.x, currentMousePos.y - mLastMousePos.y };

        mYaw -= delta.x * 0.005f;
        mPitch += delta.y * 0.005f;

        if (mPitch > ToRadians(89.0f))
            mPitch = ToRadians(89.0f);
        if (mPitch < ToRadians(-89.0f))
            mPitch = ToRadians(-89.0f);

        mLastMousePos = currentMousePos;
    }
    UpdateViewMatrix();
}

Matrix4x4 OrbitCameraController::GetViewMatrix() const { return mViewMatrix; }
Vector3 OrbitCameraController::GetPosition() const { return mPosition; }
Vector3 OrbitCameraController::GetDirection() const { return (mTarget - mPosition).Normalized(); }

void OrbitCameraController::SetState(const Vector3& position, const Vector3& direction) {
    mPosition = position;
    mTarget = position + direction * mDistance;

    Vector3 toTargetDir = (mTarget - mPosition).Normalized();
    mPitch = asinf(toTargetDir.y);
    mYaw = atan2f(toTargetDir.x, toTargetDir.z);

    UpdateViewMatrix();
}

void OrbitCameraController::UpdateViewMatrix() {
    mPosition.x = mTarget.x + mDistance * cosf(mPitch) * sinf(mYaw);
    mPosition.y = mTarget.y + mDistance * sinf(mPitch);
    mPosition.z = mTarget.z + mDistance * cosf(mPitch) * cosf(mYaw);
    mViewMatrix = Matrix4x4::CreateLookAt(mPosition, mTarget, Vector3(0.0f, 1.0f, 0.0f));
}

/////////////////////////////////////////////
// FPS TODO wip
FpsCameraController::FpsCameraController(const Vector3& position)
    : mPosition(position), mYaw(0.0f), mPitch(0.0f) {
    UpdateViewMatrix();
}

void FpsCameraController::Update(IInput* pInput, float deltaTime) {
    Vec2 mouseDelta = pInput->GetMouseDelta();
    mYaw -= mouseDelta.x * mRotationSpeed;
    mPitch -= mouseDelta.y * mRotationSpeed;

    if (mPitch > ToRadians(89.0f))
        mPitch = ToRadians(89.0f);
    if (mPitch < ToRadians(-89.0f))
        mPitch = ToRadians(-89.0f);

    Vector3 forward = GetDirection();
    Vector3 right = Vector3::Cross(forward, Vector3(0, 1, 0)).Normalized();

    Vector3 moveDir(0, 0, 0);
    if (pInput->IsDown(Key::W))
        moveDir += forward;
    if (pInput->IsDown(Key::S))
        moveDir -= forward;
    if (pInput->IsDown(Key::D))
        moveDir -= right;
    if (pInput->IsDown(Key::A))
        moveDir += right;

    if (moveDir.LengthSquared() > 0.0f) {
        mPosition += moveDir.Normalized() * mMovementSpeed * deltaTime;
    }

    UpdateViewMatrix();
}

Matrix4x4 FpsCameraController::GetViewMatrix() const { return mViewMatrix; }
Vector3 FpsCameraController::GetPosition() const { return mPosition; }

Vector3 FpsCameraController::GetDirection() const {
    Vector3 direction;
    direction.x = cosf(mPitch) * sinf(mYaw);
    direction.y = sinf(mPitch);
    direction.z = cosf(mPitch) * cosf(mYaw);
    return direction.Normalized();
}

void FpsCameraController::SetState(const Vector3& position, const Vector3& direction) {
    mPosition = position;
    mPitch = asinf(direction.y);
    mYaw = atan2f(direction.x, direction.z);
    UpdateViewMatrix();
}

void FpsCameraController::UpdateViewMatrix() {
    Vector3 target = mPosition + GetDirection();
    mViewMatrix = Matrix4x4::CreateLookAt(mPosition, target, Vector3(0.0f, 1.0f, 0.0f));
}
