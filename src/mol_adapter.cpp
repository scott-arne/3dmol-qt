#include "oe3dmol/mol_adapter.h"

#include "oe3dmol/color_schemes.h"
#include "oe3dmol/data_errors.h"

#include <oechem.h>

#include <algorithm>

namespace oe3dmol {

AtomData build_atom_data(const OEChem::OEMolBase* mol, const SphereStyle& style) {
    if (mol == nullptr) {
        throw DataError("build_atom_data: molecule is null");
    }
    if (mol->NumAtoms() == 0) {
        throw DataError("build_atom_data: molecule has no atoms");
    }

    AtomData data;
    data.positions.reserve(mol->NumAtoms());
    data.radii.reserve(mol->NumAtoms());
    data.colors.reserve(mol->NumAtoms());
    data.source_idx.reserve(mol->NumAtoms());

    for (OESystem::OEIter atom = mol->GetAtoms(); atom; ++atom) {
        const OEChem::OEAtomBase& a = *atom;
        double coords[3] = {0.0, 0.0, 0.0};
        mol->GetCoords(&a, coords);

        const unsigned atomic_num = a.GetAtomicNum();
        const std::string element = OEChem::OEGetAtomicSymbol(atomic_num);

        const float radius = static_cast<float>(resolve_radius(style, element));
        Color color = element_color(style.color(), element);
        const double op = std::clamp(style.opacity(), 0.0, 1.0);
        color.setAlphaF(static_cast<float>(op));

        data.add(QVector3D(static_cast<float>(coords[0]),
                           static_cast<float>(coords[1]),
                           static_cast<float>(coords[2])),
                 radius, color, a.GetIdx());
    }

    return data;
}

}  // namespace oe3dmol
