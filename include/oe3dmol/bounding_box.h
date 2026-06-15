#pragma once

/// \file bounding_box.h
/// \brief Axis-aligned bounding box over atom positions (radius-aware).

#include <QVector3D>

namespace oe3dmol {

/// \brief Axis-aligned bounding box accumulated from points (optionally with radii).
class BoundingBox {
public:
    /// \brief Expand the box to include a sphere of given center and radius.
    ///
    /// \param center Sphere center.
    /// \param radius Sphere radius (defaults to 0 for a point).
    void include(const QVector3D& center, float radius = 0.0f);

    /// \brief Whether any point has been included.
    bool valid() const { return valid_; }

    /// \brief Minimum corner. Undefined if !valid().
    QVector3D min() const { return min_; }
    /// \brief Maximum corner. Undefined if !valid().
    QVector3D max() const { return max_; }

    /// \brief Geometric center of the box. Asserts (debug) if !valid().
    QVector3D center() const;

    /// \brief Radius of the bounding sphere enclosing the box. Asserts (debug)
    /// if !valid().
    float bounding_radius() const;

private:
    bool valid_ = false;
    QVector3D min_;
    QVector3D max_;
};

}  // namespace oe3dmol
