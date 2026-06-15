#include "oe3dmol/bounding_box.h"

#include <algorithm>
#include <cassert>

namespace oe3dmol {

void BoundingBox::include(const QVector3D& center, float radius) {
    const QVector3D lo(center.x() - radius, center.y() - radius, center.z() - radius);
    const QVector3D hi(center.x() + radius, center.y() + radius, center.z() + radius);
    if (!valid_) {
        min_ = lo;
        max_ = hi;
        valid_ = true;
        return;
    }
    min_.setX(std::min(min_.x(), lo.x()));
    min_.setY(std::min(min_.y(), lo.y()));
    min_.setZ(std::min(min_.z(), lo.z()));
    max_.setX(std::max(max_.x(), hi.x()));
    max_.setY(std::max(max_.y(), hi.y()));
    max_.setZ(std::max(max_.z(), hi.z()));
}

QVector3D BoundingBox::center() const {
    assert(valid_ && "BoundingBox::center() called on an empty box");
    return (min_ + max_) * 0.5f;
}

float BoundingBox::bounding_radius() const {
    assert(valid_ && "BoundingBox::bounding_radius() called on an empty box");
    return ((max_ - min_) * 0.5f).length();
}

}  // namespace oe3dmol
