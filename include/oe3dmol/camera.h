#pragma once

/// \file camera.h
/// \brief Orbit (trackball) perspective camera with a zoom-to-fit helper.

#include <QMatrix4x4>
#include <QQuaternion>
#include <QSize>
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
    ///
    /// Re-derives the near/far clip planes from the new distance, so the scene
    /// stays fully framed (wheel-zoom behavior). Use move_z to drive the camera
    /// through the near plane for clipping inspection.
    void dolly(float factor);

    /// \brief Shift the target in the camera's right/up plane to track a screen
    /// drag 1:1 (screen pan).
    ///
    /// The pixel deltas are converted to world units using the true
    /// world-units-per-pixel at the target plane
    /// (2*distance*tan(fov/2)/viewport_height), the same scale on both axes so
    /// the content follows the cursor without aspect-ratio skew.
    ///
    /// \param dx_pixels Rightward drag in pixels.
    /// \param dy_pixels Upward drag in pixels (screen-up positive).
    /// \param viewport Viewport size in pixels (for the world-per-pixel scale).
    void pan(float dx_pixels, float dy_pixels, const QSize& viewport);

    /// \brief Move the camera along its view axis by a screen drag, WITHOUT
    /// re-deriving the clip planes (unlike dolly).
    ///
    /// Holding the near/far planes fixed lets the camera advance through the
    /// near plane so geometry clips, which is the point of this control
    /// (interactive clip-plane inspection). Distance is kept positive.
    ///
    /// \param dy_pixels Drag in pixels; negative (drag up) moves toward the
    ///        scene, positive (drag down) moves away.
    /// \param viewport Viewport size in pixels (for the world-per-pixel scale).
    void move_z(float dy_pixels, const QSize& viewport);

    /// \brief View matrix (world -> camera).
    QMatrix4x4 view_matrix() const;

    /// \brief Projection matrix for the given aspect ratio (width/height).
    QMatrix4x4 projection_matrix(float aspect) const;

    /// \brief Camera distance from the target.
    float distance() const { return distance_; }

    /// \brief Near clip plane distance (view space).
    float near_plane() const { return near_; }

    /// \brief Far clip plane distance (view space).
    float far_plane() const { return far_; }

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
