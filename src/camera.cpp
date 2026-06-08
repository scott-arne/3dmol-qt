#include "oe3dmol/camera.h"

#include <algorithm>
#include <cmath>

namespace oe3dmol {

void Camera::zoom_to(const BoundingBox& box, float fov_y_degrees) {
    fov_y_ = fov_y_degrees;
    target_ = box.valid() ? box.center() : QVector3D(0.0f, 0.0f, 0.0f);

    const float radius = box.valid() ? std::max(box.bounding_radius(), 1e-3f) : 1.0f;
    const float half_fov_rad = (fov_y_ * 0.5f) * static_cast<float>(M_PI) / 180.0f;
    distance_ = radius / std::sin(half_fov_rad);

    eye_ = target_ + QVector3D(0.0f, 0.0f, distance_);

    near_ = std::max(distance_ - radius, 0.01f * distance_);
    far_ = distance_ + radius;

    view_.setToIdentity();
    view_.lookAt(eye_, target_, QVector3D(0.0f, 1.0f, 0.0f));
}

QMatrix4x4 Camera::projection_matrix(float aspect) const {
    QMatrix4x4 p;
    p.perspective(fov_y_, aspect, near_, far_);
    return p;
}

}  // namespace oe3dmol
