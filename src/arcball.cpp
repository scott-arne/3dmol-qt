#include "oe3dmol/arcball.h"

#include <algorithm>
#include <cmath>

#include <QVector3D>

namespace oe3dmol {

namespace {

// Map a widget pixel point to a vector on the virtual trackball sphere.
// Widget center -> sphere top (0,0,1); edges curve toward the rim. Y is flipped
// so +Y points up (widget pixel y grows downward).
QVector3D to_sphere(const QPointF& p, const QSize& widget) {
    const float w = static_cast<float>(std::max(widget.width(), 1));
    const float h = static_cast<float>(std::max(widget.height(), 1));
    const float x = (2.0f * static_cast<float>(p.x()) - w) / w;
    const float y = (h - 2.0f * static_cast<float>(p.y())) / h;  // flip to +Y up
    const float r2 = x * x + y * y;
    if (r2 <= 1.0f) {
        return QVector3D(x, y, std::sqrt(1.0f - r2));  // on the hemisphere
    }
    const float inv = 1.0f / std::sqrt(r2);
    return QVector3D(x * inv, y * inv, 0.0f);  // projected to the rim
}

}  // namespace

QQuaternion arcball_rotation(const QPointF& p0, const QPointF& p1, const QSize& widget) {
    const QVector3D v0 = to_sphere(p0, widget);
    const QVector3D v1 = to_sphere(p1, widget);
    return QQuaternion::rotationTo(v0, v1);
}

}  // namespace oe3dmol
