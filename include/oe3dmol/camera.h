#pragma once

/// \file camera.h
/// \brief Orbit (trackball) perspective camera with a zoom-to-fit helper.

#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>

#include "oe3dmol/bounding_box.h"

namespace oe3dmol {

/// \brief Perspective orbit camera producing view and projection matrices.
///
/// The camera orbits a target at a fixed distance with a quaternion
/// orientation. At identity rotation it looks down -Z with +Y up, matching the
/// legacy fit-only camera exactly (so offscreen output is unchanged).
class Camera {
public:
    /// \brief Frame a bounding box: set target to its center, fit the distance,
    /// and reset the orientation to identity (look down -Z).
    /// \param box Scene bounds.
    /// \param fov_y_degrees Vertical field of view in degrees.
    void zoom_to(const BoundingBox& box, float fov_y_degrees = 45.0f);

    /// \brief Compose an incremental rotation into the orientation (orbit).
    void orbit(const QQuaternion& delta);

    /// \brief Scale the orbit distance by factor (clamped > 0). <1 zooms in.
    void dolly(float factor);

    /// \brief Shift the target in the camera's right/up plane (screen pan).
    /// \param dx Rightward fraction (scaled by distance).
    /// \param dy Upward fraction (scaled by distance).
    void pan(float dx, float dy);

    /// \brief View matrix (world -> camera).
    QMatrix4x4 view_matrix() const;

    /// \brief Projection matrix for the given aspect ratio (width/height).
    QMatrix4x4 projection_matrix(float aspect) const;

    /// \brief Camera distance from the target.
    float distance() const { return distance_; }

private:
    QVector3D target_;
    QQuaternion rotation_;  // identity = look down -Z, +Y up
    float fov_y_ = 45.0f;
    float distance_ = 1.0f;
    float near_ = 0.1f;
    float far_ = 10.0f;
    float scene_radius_ = 1.0f;
};

}  // namespace oe3dmol
