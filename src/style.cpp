#include "oe3dmol/style.h"

#include "oe3dmol/vdw_radii.h"

namespace oe3dmol {

double resolve_radius(const SphereStyle& style, const std::string& element) {
    double r = DEFAULT_SPHERE_RADIUS;
    if (style.radius().has_value()) {
        r = style.radius().value();
    } else {
        r = vdw_radius(element);
    }
    if (style.scale().has_value()) {
        r *= style.scale().value();
    }
    return r;
}

}  // namespace oe3dmol
