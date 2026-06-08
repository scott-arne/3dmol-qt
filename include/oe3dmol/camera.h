#pragma once

/// \file camera.h
/// \brief Perspective camera with a zoom-to-fit helper.

#include <QMatrix4x4>

#include "oe3dmol/bounding_box.h"

namespace oe3dmol {

/// \brief Perspective camera producing view and projection matrices.
class Camera {
public:
    /// \brief Position the camera to frame a bounding box.
    ///
    /// Looks down -Z at the box center from a distance that fits the
    /// bounding sphere within the vertical field of view.
    ///
    /// \param box Scene bounds (must be valid()).
    /// \param fov_y_degrees Vertical field of view in degrees.
    void zoom_to(const BoundingBox& box, float fov_y_degrees = 45.0f);

    /// \brief View matrix (world -> camera).
    QMatrix4x4 view_matrix() const { return view_; }

    /// \brief Projection matrix for the given aspect ratio (width/height).
    QMatrix4x4 projection_matrix(float aspect) const;

    /// \brief Camera distance from the target (for fog/near-far tuning).
    float distance() const { return distance_; }

private:
    QVector3D target_;
    QVector3D eye_;
    float fov_y_ = 45.0f;
    float distance_ = 1.0f;
    float near_ = 0.1f;
    float far_ = 10.0f;
    QMatrix4x4 view_;
};

}  // namespace oe3dmol
