#include "oe3dmol/bond_adapter.h"

#include "oe3dmol/color_schemes.h"
#include "oe3dmol/data_errors.h"

#include <algorithm>
#include <string>

#include <oechem.h>

namespace oe3dmol {

namespace {

QVector3D atom_pos(const OEChem::OEMolBase* mol, const OEChem::OEAtomBase* atom) {
    double c[3] = {0.0, 0.0, 0.0};
    mol->GetCoords(atom, c);
    return QVector3D(static_cast<float>(c[0]), static_cast<float>(c[1]),
                     static_cast<float>(c[2]));
}

Color atom_color(ColorScheme scheme, const OEChem::OEAtomBase* atom, double opacity) {
    const std::string element = OEChem::OEGetAtomicSymbol(atom->GetAtomicNum());
    Color c = element_color(scheme, element);
    c.setAlphaF(static_cast<float>(std::clamp(opacity, 0.0, 1.0)));
    return c;
}

// Emit one rendered cylinder line from `a` to `b` as two half-bonds split at the
// line's midpoint, each half taking its near-end color. Shared by every bond
// multiplicity so coloring/splitting logic lives in one place.
void add_line(BondData& data, const QVector3D& a, const QVector3D& b, float radius,
              const Color& color_a, const Color& color_b, unsigned bond_index) {
    const QVector3D mid = (a + b) * 0.5f;
    data.add(a, mid, radius, color_a, bond_index);
    data.add(b, mid, radius, color_b, bond_index);
}

}  // namespace

QVector3D bond_offset_dir(const QVector3D& axis, const QVector3D& ref) {
    const QVector3D a = axis.normalized();
    QVector3D perp = ref - a * QVector3D::dotProduct(ref, a);
    constexpr float MIN_PERP_SQ = 1.0e-8f;
    if (perp.lengthSquared() >= MIN_PERP_SQ) {
        return perp.normalized();
    }
    // Collinear / no neighbor: pick any axis-perpendicular. cross with X, then Y;
    // at least one is non-degenerate since `a` cannot be parallel to both.
    QVector3D c = QVector3D::crossProduct(a, QVector3D(1, 0, 0));
    if (c.lengthSquared() < MIN_PERP_SQ) {
        c = QVector3D::crossProduct(a, QVector3D(0, 1, 0));
    }
    return c.normalized();
}

BondData build_bond_data(const OEChem::OEMolBase* mol, const StickStyle& style) {
    if (mol == nullptr) {
        throw DataError("build_bond_data: molecule is null");
    }

    BondData data;
    data.reserve(static_cast<std::size_t>(mol->NumBonds()) * 2);
    const float radius = static_cast<float>(style.radius());

    for (OESystem::OEIter bond = mol->GetBonds(); bond; ++bond) {
        const OEChem::OEBondBase& b = *bond;
        const OEChem::OEAtomBase* abgn = b.GetBgn();
        const OEChem::OEAtomBase* aend = b.GetEnd();
        if (abgn == nullptr || aend == nullptr) {
            continue;
        }

        const QVector3D p_bgn = atom_pos(mol, abgn);
        const QVector3D p_end = atom_pos(mol, aend);
        // Skip degenerate bonds: a near-zero axis would yield a degenerate
        // orientation basis in the cylinder shader. 1e-3 A matches the project's
        // existing degenerate-geometry clamp (see Camera::zoom_to).
        constexpr float MIN_BOND_LENGTH_SQ = 1.0e-6f;  // (1e-3 A)^2
        if ((p_end - p_bgn).lengthSquared() < MIN_BOND_LENGTH_SQ) {
            continue;
        }

        const Color c_bgn = atom_color(style.color(), abgn, style.opacity());
        const Color c_end = atom_color(style.color(), aend, style.opacity());
        const unsigned idx = b.GetIdx();

        add_line(data, p_bgn, p_end, radius, c_bgn, c_end, idx);
    }

    return data;
}

}  // namespace oe3dmol
