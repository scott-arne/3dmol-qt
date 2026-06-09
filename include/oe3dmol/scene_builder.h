#pragma once

/// \file scene_builder.h
/// \brief Shared model -> AtomData/BondData scene assembly for all viewers.

#include <vector>

#include "oe3dmol/atom_data.h"
#include "oe3dmol/bond_data.h"
#include "oe3dmol/bounding_box.h"
#include "oe3dmol/style_spec.h"

namespace OEChem {
class OEMolBase;
}

namespace oe3dmol {

/// \brief A molecule (non-owning) plus the styles applied to it.
struct SceneModel {
    const OEChem::OEMolBase* mol = nullptr;
    std::vector<StyleSpec> styles;
};

/// \brief Assemble combined sphere atoms, stick half-bonds, and scene bounds
/// from a list of styled models.
///
/// For each (model, style) pair: SphereStyle contributes sphere AtomData,
/// StickStyle contributes half-bond BondData (3dmol-style). Geometry is moved
/// into the accumulators when empty and appended otherwise.
///
/// \param models The styled models (each mol must be non-null).
/// \param atoms Output: accumulated sphere atom data (cleared first).
/// \param bonds Output: accumulated stick half-bond data (cleared first).
/// \param bounds Output: accumulated scene bounds (cleared first).
void build_scene(const std::vector<SceneModel>& models, AtomData& atoms,
                 BondData& bonds, BoundingBox& bounds);

}  // namespace oe3dmol
