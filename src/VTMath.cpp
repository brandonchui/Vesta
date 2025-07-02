#include "VTMath.h"

#include <cmath>

// TODO move this or delete might clash
const float Pi = 3.14159265358979323846f;

constexpr float ToRadians(float Degrees) noexcept { return Degrees * 0.017453292519943295f; }
constexpr float ToDegrees(float Radians) noexcept { return Radians * 57.295779513082320876f; }

/////////////////////////////////////
/// Vector2
constexpr Vector2::Vector2(float InX, float InY) noexcept : x(InX), y(InY) {}

/////////////////////////////////////
/// Vector3 implementation
constexpr Vector3::Vector3(float InX, float InY, float InZ) noexcept : x(InX), y(InY), z(InZ) {}

constexpr Vector3 Vector3::operator-(const Vector3& Other) const noexcept {
    return Vector3(x - Other.x, y - Other.y, z - Other.z);
}

constexpr Vector3 Vector3::operator+(const Vector3& Other) const noexcept {
    return Vector3(x + Other.x, y + Other.y, z + Other.z);
}

constexpr Vector3 Vector3::operator*(float Scalar) const noexcept {
    return Vector3(x * Scalar, y * Scalar, z * Scalar);
}

constexpr Vector3& Vector3::operator+=(const Vector3& Other) noexcept {
    x += Other.x;
    y += Other.y;
    z += Other.z;
    return *this;
}

constexpr Vector3& Vector3::operator-=(const Vector3& Other) noexcept {
    x -= Other.x;
    y -= Other.y;
    z -= Other.z;
    return *this;
}

constexpr float Vector3::Dot(const Vector3& Other) const noexcept {
    return x * Other.x + y * Other.y + z * Other.z;
}

Vector3 Vector3::Cross(const Vector3& Other) const noexcept {
    return Vector3(y * Other.z - z * Other.y, z * Other.x - x * Other.z, x * Other.y - y * Other.x);
}

constexpr Vector3 Vector3::operator-() const noexcept { return Vector3(-x, -y, -z); }

float Vector3::Length() const noexcept { return std::sqrt(x * x + y * y + z * z); }

Vector3 Vector3::Normalized() const noexcept {
    float Len = Length();
    if (Len > 0.0f) {
        float InvLen = 1.0f / Len;
        return Vector3(x * InvLen, y * InvLen, z * InvLen);
    }
    return *this;
}

void Vector3::Normalize() noexcept {
    float Len = Length();
    if (Len > 0.0f) {
        float InvLen = 1.0f / Len;
        x *= InvLen;
        y *= InvLen;
        z *= InvLen;
    }
}

Vector3 Vector3::TransformVector(const Vector3& Vec, const Matrix4x4& Mat) noexcept {
    return Vector3(Vec.x * Mat[0, 0] + Vec.y * Mat[1, 0] + Vec.z * Mat[2, 0],
                   Vec.x * Mat[0, 1] + Vec.y * Mat[1, 1] + Vec.z * Mat[2, 1],
                   Vec.x * Mat[0, 2] + Vec.y * Mat[1, 2] + Vec.z * Mat[2, 2]);
}

Vector3 Vector3::Cross(const Vector3& A, const Vector3& B) noexcept {
    return Vector3(A.y * B.z - A.z * B.y, A.z * B.x - A.x * B.z, A.x * B.y - A.y * B.x);
}

constexpr float Vector3::Dot(const Vector3& A, const Vector3& B) noexcept {
    return A.x * B.x + A.y * B.y + A.z * B.z;
}

constexpr float Vector3::LengthSquared() const noexcept { return x * x + y * y + z * z; }

/////////////////////////////////////
/// Vector4 implementation
constexpr Vector4::Vector4(float InX, float InY, float InZ, float InW) noexcept
    : x(InX), y(InY), z(InZ), w(InW) {}

/////////////////////////////////////
/// Matrix4x4 implementation
constexpr Matrix4x4::Matrix4x4() noexcept {
    for (uint32_t I = 0; I < 16; ++I) {
        M[I] = 0.0f;
    }
}

constexpr Matrix4x4::Matrix4x4(float Diagonal) noexcept {
    for (uint32_t I = 0; I < 16; ++I) {
        M[I] = 0.0f;
    }
    M[0] = M[5] = M[10] = M[15] = Diagonal;
}

constexpr float& Matrix4x4::operator[](uint32_t Row, uint32_t Col) noexcept {
    return M[Row * 4 + Col];
}

constexpr const float& Matrix4x4::operator[](uint32_t Row, uint32_t Col) const noexcept {
    return M[Row * 4 + Col];
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& Other) const noexcept {
    Matrix4x4 Result;
    for (uint32_t Row = 0; Row < 4; ++Row) {
        for (uint32_t Col = 0; Col < 4; ++Col) {
            float Sum = 0.0f;
            for (uint32_t K = 0; K < 4; ++K) {
                Sum += (*this)[Row, K] * Other[K, Col];
            }
            Result[Row, Col] = Sum;
        }
    }
    return Result;
}

void Matrix4x4::Transpose() noexcept {
    for (uint32_t Row = 0; Row < 4; ++Row) {
        for (uint32_t Col = Row + 1; Col < 4; ++Col) {
            float Temp = (*this)[Row, Col];
            (*this)[Row, Col] = (*this)[Col, Row];
            (*this)[Col, Row] = Temp;
        }
    }
}

Vector3 Matrix4x4::GetTranslation() const noexcept {
    return Vector3((*this)[3, 0], (*this)[3, 1], (*this)[3, 2]);
}

Matrix4x4
Matrix4x4::CreateLookAt(const Vector3& Eye, const Vector3& Target, const Vector3& Up) noexcept {
    Vector3 Forward = (Target - Eye).Normalized();
    Vector3 Right = Up.Cross(Forward).Normalized();
    Vector3 RealUp = Forward.Cross(Right);

    Matrix4x4 Result;

    Result[0, 0] = Right.x;
    Result[0, 1] = Right.y;
    Result[0, 2] = Right.z;

    Result[1, 0] = RealUp.x;
    Result[1, 1] = RealUp.y;
    Result[1, 2] = RealUp.z;

    Result[2, 0] = Forward.x;
    Result[2, 1] = Forward.y;
    Result[2, 2] = Forward.z;

    Result[3, 0] = -Right.Dot(Eye);
    Result[3, 1] = -RealUp.Dot(Eye);
    Result[3, 2] = -Forward.Dot(Eye);

    Result[0, 3] = 0.0f;
    Result[1, 3] = 0.0f;
    Result[2, 3] = 0.0f;
    Result[3, 3] = 1.0f;

    return Result;
}

Matrix4x4 Matrix4x4::CreateFromAxisAngle(const Vector3& Axis, float AngleRadians) noexcept {
    Vector3 NormalizedAxis = Axis.Normalized();
    float CosAngle = std::cos(AngleRadians);
    float SinAngle = std::sin(AngleRadians);
    float OneMinusCos = 1.0f - CosAngle;

    float XX = NormalizedAxis.x * NormalizedAxis.x;
    float YY = NormalizedAxis.y * NormalizedAxis.y;
    float ZZ = NormalizedAxis.z * NormalizedAxis.z;
    float XY = NormalizedAxis.x * NormalizedAxis.y;
    float XZ = NormalizedAxis.x * NormalizedAxis.z;
    float YZ = NormalizedAxis.y * NormalizedAxis.z;

    Matrix4x4 Result;

    Result[0, 0] = CosAngle + XX * OneMinusCos;
    Result[0, 1] = XY * OneMinusCos + NormalizedAxis.z * SinAngle;
    Result[0, 2] = XZ * OneMinusCos - NormalizedAxis.y * SinAngle;
    Result[0, 3] = 0.0f;

    Result[1, 0] = XY * OneMinusCos - NormalizedAxis.z * SinAngle;
    Result[1, 1] = CosAngle + YY * OneMinusCos;
    Result[1, 2] = YZ * OneMinusCos + NormalizedAxis.x * SinAngle;
    Result[1, 3] = 0.0f;

    Result[2, 0] = XZ * OneMinusCos + NormalizedAxis.y * SinAngle;
    Result[2, 1] = YZ * OneMinusCos - NormalizedAxis.x * SinAngle;
    Result[2, 2] = CosAngle + ZZ * OneMinusCos;
    Result[2, 3] = 0.0f;

    Result[3, 0] = 0.0f;
    Result[3, 1] = 0.0f;
    Result[3, 2] = 0.0f;
    Result[3, 3] = 1.0f;

    return Result;
}

Matrix4x4 Matrix4x4::CreatePerspectiveFieldOfView(float FovAngleY,
                                                  float AspectRatio,
                                                  float NearZ,
                                                  float FarZ) noexcept {
    float YScale = 1.0f / tanf(FovAngleY * 0.5f);
    float XScale = YScale / AspectRatio;

    Matrix4x4 Result;

    Result[0, 0] = XScale;
    Result[1, 1] = YScale;
    Result[2, 2] = FarZ / (FarZ - NearZ);
    Result[2, 3] = 1.0f;
    Result[3, 2] = -NearZ * FarZ / (FarZ - NearZ);
    Result[3, 3] = 0.0f;

    return Result;
}

Matrix4x4
Matrix4x4::CreatePerspective(float Width, float Height, float NearZ, float FarZ) noexcept {
    float ZRange = FarZ - NearZ;

    Matrix4x4 Result;

    Result[0, 0] = 2.0f * NearZ / Width;
    Result[1, 1] = 2.0f * NearZ / Height;
    Result[2, 2] = FarZ / ZRange;
    Result[2, 3] = 1.0f;
    Result[3, 2] = -NearZ * FarZ / ZRange;
    Result[3, 3] = 0.0f;

    return Result;
}

Matrix4x4 Matrix4x4::CreateScale(float x, float y, float z) noexcept {
    Matrix4x4 Result;
    Result[0, 0] = x;
    Result[1, 1] = y;
    Result[2, 2] = z;
    Result[3, 3] = 1.0f;
    return Result;
}

Matrix4x4 Matrix4x4::CreateScale(float UniformScale) noexcept {
    return CreateScale(UniformScale, UniformScale, UniformScale);
}

Matrix4x4 Matrix4x4::CreateScale(const Vector3& Scale) noexcept {
    return CreateScale(Scale.x, Scale.y, Scale.z);
}

Matrix4x4 Matrix4x4::CreateOrtho(float Width, float Height, float NearZ, float FarZ) noexcept {
    Matrix4x4 Result;
    Result[0, 0] = 2.0f / Width;
    Result[1, 1] = 2.0f / Height;
    Result[2, 2] = 1.0f / (FarZ - NearZ);
    Result[3, 2] = -NearZ / (FarZ - NearZ);
    Result[3, 3] = 1.0f;
    return Result;
}

Matrix4x4 Matrix4x4::CreateOrthographicOffCenter(float left,
                                                 float right,
                                                 float bottom,
                                                 float top,
                                                 float z_near,
                                                 float z_far) noexcept {
    Matrix4x4 Result;

    float inv_width = 1.0f / (right - left);
    float inv_height = 1.0f / (top - bottom);
    float inv_depth = 1.0f / (z_far - z_near);

    Result[0, 0] = 2.0f * inv_width;
    Result[1, 1] = 2.0f * inv_height;
    Result[2, 2] = inv_depth;

    Result[3, 0] = -(right + left) * inv_width;
    Result[3, 1] = -(top + bottom) * inv_height;
    Result[3, 2] = -z_near * inv_depth;
    Result[3, 3] = 1.0f;

    return Result;
}

Matrix4x4 Matrix4x4::CreateRotationX(float AngleRadians) noexcept {
    Matrix4x4 Result = IdentityMatrix();
    float CosA = std::cos(AngleRadians);
    float SinA = std::sin(AngleRadians);

    Result[1, 1] = CosA;
    Result[1, 2] = SinA;
    Result[2, 1] = -SinA;
    Result[2, 2] = CosA;

    return Result;
}

Matrix4x4 Matrix4x4::CreateRotationY(float AngleRadians) noexcept {
    Matrix4x4 Result = IdentityMatrix();
    float CosA = std::cos(AngleRadians);
    float SinA = std::sin(AngleRadians);

    Result[0, 0] = CosA;
    Result[0, 2] = -SinA;
    Result[2, 0] = SinA;
    Result[2, 2] = CosA;

    return Result;
}

Matrix4x4 Matrix4x4::CreateRotationZ(float AngleRadians) noexcept {
    Matrix4x4 Result = IdentityMatrix();
    float CosA = std::cos(AngleRadians);
    float SinA = std::sin(AngleRadians);

    Result[0, 0] = CosA;
    Result[0, 1] = SinA;
    Result[1, 0] = -SinA;
    Result[1, 1] = CosA;

    return Result;
}

Matrix4x4 Matrix4x4::CreateTranslation(float x, float y, float z) noexcept {
    Matrix4x4 Result = IdentityMatrix();
    Result[3, 0] = x;
    Result[3, 1] = y;
    Result[3, 2] = z;
    return Result;
}

Matrix4x4 Matrix4x4::CreateTranslation(const Vector3& Translation) noexcept {
    return CreateTranslation(Translation.x, Translation.y, Translation.z);
}

constexpr Matrix4x4 IdentityMatrix() noexcept { return Matrix4x4(1.0f); }

bool Invert(Matrix4x4& Mat) noexcept {
    float Inv[16];
    float* M = Mat.M;

    Inv[0] = M[5] * M[10] * M[15] - M[5] * M[11] * M[14] - M[9] * M[6] * M[15] +
             M[9] * M[7] * M[14] + M[13] * M[6] * M[11] - M[13] * M[7] * M[10];
    Inv[4] = -M[4] * M[10] * M[15] + M[4] * M[11] * M[14] + M[8] * M[6] * M[15] -
             M[8] * M[7] * M[14] - M[12] * M[6] * M[11] + M[12] * M[7] * M[10];
    Inv[8] = M[4] * M[9] * M[15] - M[4] * M[11] * M[13] - M[8] * M[5] * M[15] +
             M[8] * M[7] * M[13] + M[12] * M[5] * M[11] - M[12] * M[7] * M[9];
    Inv[12] = -M[4] * M[9] * M[14] + M[4] * M[10] * M[13] + M[8] * M[5] * M[14] -
              M[8] * M[6] * M[13] - M[12] * M[5] * M[10] + M[12] * M[6] * M[9];
    Inv[1] = -M[1] * M[10] * M[15] + M[1] * M[11] * M[14] + M[9] * M[2] * M[15] -
             M[9] * M[3] * M[14] - M[13] * M[2] * M[11] + M[13] * M[3] * M[10];
    Inv[5] = M[0] * M[10] * M[15] - M[0] * M[11] * M[14] - M[8] * M[2] * M[15] +
             M[8] * M[3] * M[14] + M[12] * M[2] * M[11] - M[12] * M[3] * M[10];
    Inv[9] = -M[0] * M[9] * M[15] + M[0] * M[11] * M[13] + M[8] * M[1] * M[15] -
             M[8] * M[3] * M[13] - M[12] * M[1] * M[11] + M[12] * M[3] * M[9];
    Inv[13] = M[0] * M[9] * M[14] - M[0] * M[10] * M[13] - M[8] * M[1] * M[14] +
              M[8] * M[2] * M[13] + M[12] * M[1] * M[10] - M[12] * M[2] * M[9];
    Inv[2] = M[1] * M[6] * M[15] - M[1] * M[7] * M[14] - M[5] * M[2] * M[15] + M[5] * M[3] * M[14] +
             M[13] * M[2] * M[7] - M[13] * M[3] * M[6];
    Inv[6] = -M[0] * M[6] * M[15] + M[0] * M[7] * M[14] + M[4] * M[2] * M[15] -
             M[4] * M[3] * M[14] - M[12] * M[2] * M[7] + M[12] * M[3] * M[6];
    Inv[10] = M[0] * M[5] * M[15] - M[0] * M[7] * M[13] - M[4] * M[1] * M[15] +
              M[4] * M[3] * M[13] + M[12] * M[1] * M[7] - M[12] * M[3] * M[5];
    Inv[14] = -M[0] * M[5] * M[14] + M[0] * M[6] * M[13] + M[4] * M[1] * M[14] -
              M[4] * M[2] * M[13] - M[12] * M[1] * M[6] + M[12] * M[2] * M[5];
    Inv[3] = -M[1] * M[6] * M[11] + M[1] * M[7] * M[10] + M[5] * M[2] * M[11] -
             M[5] * M[3] * M[10] - M[9] * M[2] * M[7] + M[9] * M[3] * M[6];
    Inv[7] = M[0] * M[6] * M[11] - M[0] * M[7] * M[10] - M[4] * M[2] * M[11] + M[4] * M[3] * M[10] +
             M[8] * M[2] * M[7] - M[8] * M[3] * M[6];
    Inv[11] = -M[0] * M[5] * M[11] + M[0] * M[7] * M[9] + M[4] * M[1] * M[11] - M[4] * M[3] * M[9] -
              M[8] * M[1] * M[7] + M[8] * M[3] * M[5];
    Inv[15] = M[0] * M[5] * M[10] - M[0] * M[6] * M[9] - M[4] * M[1] * M[10] + M[4] * M[2] * M[9] +
              M[8] * M[1] * M[6] - M[8] * M[2] * M[5];

    float Det = M[0] * Inv[0] + M[1] * Inv[4] + M[2] * Inv[8] + M[3] * Inv[12];

    if (Det == 0.0f)
        return false;

    Det = 1.0f / Det;

    for (uint32_t I = 0; I < 16; I++)
        M[I] = Inv[I] * Det;

    return true;
}
