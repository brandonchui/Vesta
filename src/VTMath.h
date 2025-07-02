#pragma once

#include "Common/Config.h"

VT_API constexpr float ToRadians(float Degrees) noexcept;
VT_API constexpr float ToDegrees(float Radians) noexcept;

extern VT_API const float Pi;

struct Vector2 {
    float x, y;

    VT_API constexpr Vector2(float InX = 0.0f, float InY = 0.0f) noexcept;
};

struct Matrix4x4;

struct Vector3 {
    float x, y, z;

    VT_API constexpr Vector3(float InX = 0.0f, float InY = 0.0f, float InZ = 0.0f) noexcept;

    VT_API constexpr Vector3 operator-(const Vector3& Other) const noexcept;
    VT_API constexpr Vector3 operator+(const Vector3& Other) const noexcept;
    VT_API constexpr Vector3 operator*(float Scalar) const noexcept;
    VT_API constexpr Vector3& operator+=(const Vector3& Other) noexcept;
    VT_API constexpr Vector3& operator-=(const Vector3& Other) noexcept;
    VT_API constexpr float Dot(const Vector3& Other) const noexcept;
    VT_API Vector3 Cross(const Vector3& Other) const noexcept;
    VT_API constexpr Vector3 operator-() const noexcept;
    VT_API float Length() const noexcept;
    VT_API Vector3 Normalized() const noexcept;
    VT_API void Normalize() noexcept;
    VT_API static Vector3 TransformVector(const Vector3& Vec, const Matrix4x4& Mat) noexcept;
    VT_API static Vector3 Cross(const Vector3& A, const Vector3& B) noexcept;
    VT_API static constexpr float Dot(const Vector3& A, const Vector3& B) noexcept;
    VT_API constexpr float LengthSquared() const noexcept;
};

struct Vector4 {
    float x, y, z, w;

    VT_API constexpr Vector4(float InX = 0.0f,
                             float InY = 0.0f,
                             float InZ = 0.0f,
                             float InW = 1.0f) noexcept;
};

struct Matrix4x4 {
    float M[16];

    VT_API constexpr Matrix4x4() noexcept;
    VT_API constexpr Matrix4x4(float Diagonal) noexcept;

    VT_API constexpr float& operator[](uint32_t Row, uint32_t Col) noexcept;
    VT_API constexpr const float& operator[](uint32_t Row, uint32_t Col) const noexcept;
    VT_API Matrix4x4 operator*(const Matrix4x4& Other) const noexcept;
    VT_API void Transpose() noexcept;
    VT_API Vector3 GetTranslation() const noexcept;

    VT_API static Matrix4x4
    CreateLookAt(const Vector3& Eye, const Vector3& Target, const Vector3& Up) noexcept;
    VT_API static Matrix4x4 CreateFromAxisAngle(const Vector3& Axis, float AngleRadians) noexcept;
    VT_API static Matrix4x4 CreatePerspectiveFieldOfView(float FovAngleY,
                                                         float AspectRatio,
                                                         float NearZ,
                                                         float FarZ) noexcept;
    VT_API static Matrix4x4
    CreatePerspective(float Width, float Height, float NearZ, float FarZ) noexcept;
    VT_API static Matrix4x4 CreateScale(float x, float y, float z) noexcept;
    VT_API static Matrix4x4 CreateScale(float UniformScale) noexcept;
    VT_API static Matrix4x4 CreateScale(const Vector3& Scale) noexcept;
    VT_API static Matrix4x4
    CreateOrtho(float Width, float Height, float NearZ, float FarZ) noexcept;
    VT_API static Matrix4x4 CreateOrthographicOffCenter(float left,
                                                        float right,
                                                        float bottom,
                                                        float top,
                                                        float z_near,
                                                        float z_far) noexcept;
    VT_API static Matrix4x4 CreateRotationX(float AngleRadians) noexcept;
    VT_API static Matrix4x4 CreateRotationY(float AngleRadians) noexcept;
    VT_API static Matrix4x4 CreateRotationZ(float AngleRadians) noexcept;
    VT_API static Matrix4x4 CreateTranslation(float x, float y, float z) noexcept;
    VT_API static Matrix4x4 CreateTranslation(const Vector3& Translation) noexcept;
};

VT_API constexpr Matrix4x4 IdentityMatrix() noexcept;
VT_API bool Invert(Matrix4x4& Mat) noexcept;
