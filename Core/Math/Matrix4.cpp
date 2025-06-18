#include "Matrix4.h"
#include "Quaternion.h"

namespace BGE {

Matrix4 Matrix4::Rotation(const Quaternion& q) {
    return q.ToMatrix();
}

void Matrix4::Decompose(Vector3& translation, Quaternion& rotation, Vector3& scale) const {
    // Extract translation
    translation = GetTranslation();
    
    // Extract scale
    scale = GetScale();
    
    // Extract rotation by removing scale from the matrix
    Matrix4 rotMat = *this;
    if (scale.x != 0) {
        rotMat.m[0] /= scale.x;
        rotMat.m[1] /= scale.x;
        rotMat.m[2] /= scale.x;
    }
    if (scale.y != 0) {
        rotMat.m[4] /= scale.y;
        rotMat.m[5] /= scale.y;
        rotMat.m[6] /= scale.y;
    }
    if (scale.z != 0) {
        rotMat.m[8] /= scale.z;
        rotMat.m[9] /= scale.z;
        rotMat.m[10] /= scale.z;
    }
    
    rotation = Quaternion::FromRotationMatrix(rotMat);
}

Quaternion Quaternion::FromRotationMatrix(const Matrix4& m) {
    float trace = m.m[0] + m.m[5] + m.m[10];
    
    if (trace > 0) {
        float s = 0.5f / sqrtf(trace + 1.0f);
        return Quaternion(
            (m.m[6] - m.m[9]) * s,
            (m.m[8] - m.m[2]) * s,
            (m.m[1] - m.m[4]) * s,
            0.25f / s
        );
    } else {
        if (m.m[0] > m.m[5] && m.m[0] > m.m[10]) {
            float s = 2.0f * sqrtf(1.0f + m.m[0] - m.m[5] - m.m[10]);
            return Quaternion(
                0.25f * s,
                (m.m[4] + m.m[1]) / s,
                (m.m[8] + m.m[2]) / s,
                (m.m[6] - m.m[9]) / s
            );
        } else if (m.m[5] > m.m[10]) {
            float s = 2.0f * sqrtf(1.0f + m.m[5] - m.m[0] - m.m[10]);
            return Quaternion(
                (m.m[4] + m.m[1]) / s,
                0.25f * s,
                (m.m[9] + m.m[6]) / s,
                (m.m[8] - m.m[2]) / s
            );
        } else {
            float s = 2.0f * sqrtf(1.0f + m.m[10] - m.m[0] - m.m[5]);
            return Quaternion(
                (m.m[8] + m.m[2]) / s,
                (m.m[9] + m.m[6]) / s,
                0.25f * s,
                (m.m[1] - m.m[4]) / s
            );
        }
    }
}

} // namespace BGE