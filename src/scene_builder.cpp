#include "oe3dmol/scene_builder.h"

#include <utility>
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
                // Fold into scene bounds here: the move fast-path below skips the
                // per-element add() that would otherwise accumulate bounds.
                for (std::size_t i = 0; i < d.size(); ++i) {
                    bounds.include(d.positions[i], d.radii[i]);
                }
                if (atoms.size() == 0) {
                    atoms = std::move(d);
                } else {
                    atoms.append(d);
                }
            } else if (std::holds_alternative<StickStyle>(spec)) {
                BondData b = build_bond_data(m.mol, std::get<StickStyle>(spec));
                // Fold into scene bounds here (see sphere branch).
                for (std::size_t i = 0; i < b.size(); ++i) {
                    bounds.include(b.starts[i], b.radii[i]);
                    bounds.include(b.ends[i], b.radii[i]);
                }
                if (bonds.size() == 0) {
                    bonds = std::move(b);
                } else {
                    bonds.append(b);
                }
            }
        }
    }
}

}  // namespace oe3dmol
