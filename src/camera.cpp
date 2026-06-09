#include "oe3dmol/camera.h"

#include <algorithm>
#include <cmath>

namespace oe3dmol {

namespace {
// Derive near/far clip planes from the orbit distance and scene radius. The far
// extent is measured as one scene radius past the target; valid while the target
// stays near the scene center (true for zoom_to and bounded pan). near is kept
// positive via the 0.01*distance floor for very-close dolly.
void update_clip(float distance, float radius, float& near_out, float& far_out) {
    near_out = std::max(distance - radius, 0.01f * distance);
    far_out = distance + radius;
}
}  // namespace

void Camera::zoom_to(const BoundingBox& box, float fov_y_degrees) {
    fov_y_ = fov_y_degrees;
    target_ = box.valid() ? box.center() : QVector3D(0.0f, 0.0f, 0.0f);
    scene_radius_ = box.valid() ? std::max(box.bounding_radius(), 1e-3f) : 1.0f;
    const float half_fov_rad = (fov_y_ * 0.5f) * static_cast<float>(M_PI) / 180.0f;
    distance_ = scene_radius_ / std::sin(half_fov_rad);
    rotation_ = QQuaternion();  // identity
    update_clip(distance_, scene_radius_, near_, far_);
}

void Camera::orbit(const QQuaternion& delta) {
    rotation_ = (delta * rotation_).normalized();
}

void Camera::dolly(float factor) {
    const float min_d = 0.05f * scene_radius_;
    const float max_d = 1000.0f * scene_radius_;
    distance_ = std::clamp(distance_ * factor, min_d, max_d);
    update_clip(distance_, scene_radius_, near_, far_);
}

void Camera::pan(float dx, float dy) {
    const QQuaternion inv = rotation_.conjugated();
    const QVector3D right = inv.rotatedVector(QVector3D(1.0f, 0.0f, 0.0f));
    const QVector3D up = inv.rotatedVector(QVector3D(0.0f, 1.0f, 0.0f));
    target_ += (right * (-dx) + up * dy) * distance_;
}

QMatrix4x4 Camera::view_matrix() const {
    QMatrix4x4 v;
    v.translate(0.0f, 0.0f, -distance_);
    v.rotate(rotation_);
    v.translate(-target_);
    return v;
}

QMatrix4x4 Camera::projection_matrix(float aspect) const {
    QMatrix4x4 p;
    p.perspective(fov_y_, aspect, near_, far_);
    return p;
}

}  // namespace oe3dmol
