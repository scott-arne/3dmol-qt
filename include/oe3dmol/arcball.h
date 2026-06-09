#pragma once

/// \file arcball.h
/// \brief Arcball (virtual trackball) rotation from screen drags.

#include <QPointF>
#include <QQuaternion>
#include <QSize>

namespace oe3dmol {

/// \brief Compute the rotation produced by dragging from p0 to p1 on a virtual
/// trackball covering the widget.
///
/// Each widget-space point is mapped to a point on a unit sphere centered on
/// the widget (inside the sphere radius -> hemisphere; outside -> projected to
/// the rim). The result is the quaternion rotating the p0 sphere-vector to the
/// p1 sphere-vector. A zero-length drag returns the identity quaternion.
///
/// \param p0 Drag start, widget pixel coordinates (origin top-left).
/// \param p1 Drag end, widget pixel coordinates.
/// \param widget Widget size in pixels.
/// \returns Rotation quaternion (world-space, +Y up, +X right, -Z into screen).
QQuaternion arcball_rotation(const QPointF& p0, const QPointF& p1, const QSize& widget);

}  // namespace oe3dmol
