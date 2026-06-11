#include "oe3dmol/scene_builder.h"

#include <variant>

#include "oe3dmol/bond_adapter.h"
#include "oe3dmol/mol_adapter.h"

namespace oe3dmol {

void build_scene(const std::vector<SceneModel>& models, AtomData& atoms,
                 BondData& bonds, BoundingBox& bounds) {
    atoms = AtomData{};
    bonds = BondData{};
    bounds = BoundingBox{};

    for (const SceneModel& m : models) {
        for (const StyleSpec& spec : m.styles) {
            if (std::holds_alternative<SphereStyle>(spec)) {
                AtomData d = build_atom_data(m.mol, std::get<SphereStyle>(spec));
                for (std::size_t i = 0; i < d.size(); ++i) {
                    bounds.include(d.positions[i], d.radii[i]);
                }
                if (atoms.size() == 0) {
                    atoms = std::move(d);
                } else {
                    for (std::size_t i = 0; i < d.size(); ++i) {
                        atoms.add(d.positions[i], d.radii[i], d.colors[i],
                                  d.source_idx[i]);
                    }
                }
            } else if (std::holds_alternative<StickStyle>(spec)) {
                BondData b = build_bond_data(m.mol, std::get<StickStyle>(spec));
                for (std::size_t i = 0; i < b.size(); ++i) {
                    bounds.include(b.starts[i], b.radii[i]);
                    bounds.include(b.ends[i], b.radii[i]);
                }
                if (bonds.size() == 0) {
                    bonds = std::move(b);
                } else {
                    for (std::size_t i = 0; i < b.size(); ++i) {
                        bonds.add(b.starts[i], b.ends[i], b.radii[i], b.colors[i],
                                  b.source_bond_idx[i]);
                    }
                }
            }
        }
    }
}

}  // namespace oe3dmol
