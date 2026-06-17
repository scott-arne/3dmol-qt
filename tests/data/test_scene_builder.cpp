#include <gtest/gtest.h>

#include "oe3dmol/scene_builder.h"

#include <oechem.h>

namespace oe3dmol {
namespace test {

namespace {
// Build a two-atom C-O molecule with a single bond and real coordinates.
OEChem::OEGraphMol make_co() {
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* c = mol.NewAtom(6);
    OEChem::OEAtomBase* o = mol.NewAtom(8);
    double pc[3] = {0, 0, 0};
    double po[3] = {1.5, 0, 0};
    mol.SetCoords(c, pc);
    mol.SetCoords(o, po);
    mol.NewBond(c, o, 1);
    return mol;
}
}  // namespace

TEST(SceneBuilderTest, SphereStyleProducesAtomsAndBounds) {
    OEChem::OEGraphMol mol = make_co();
    const OEChem::OEMolBase& base = mol;
    std::vector<SceneModel> models{SceneModel{&base, {SphereStyle{}}, nullptr}};

    AtomData atoms;
    BondData bonds;
    BoundingBox bounds;
    build_scene(models, atoms, bonds, bounds);

    EXPECT_EQ(atoms.size(), 2u);
    EXPECT_EQ(bonds.size(), 0u);
    EXPECT_TRUE(bounds.valid());
}

TEST(SceneBuilderTest, StickStyleProducesHalfBonds) {
    OEChem::OEGraphMol mol = make_co();
    const OEChem::OEMolBase& base = mol;
    std::vector<SceneModel> models{SceneModel{&base, {StickStyle{}}, nullptr}};

    AtomData atoms;
    BondData bonds;
    BoundingBox bounds;
    build_scene(models, atoms, bonds, bounds);

    EXPECT_EQ(bonds.size(), 2u);  // one bond -> two half-bonds
    EXPECT_TRUE(bounds.valid());
}

TEST(SceneBuilderTest, MultipleModelsAccumulateIntoOneBuffer) {
    OEChem::OEGraphMol mol = make_co();
    const OEChem::OEMolBase& base = mol;
    // Two models, each sphere-styled -> atoms accumulate (move first, append second).
    std::vector<SceneModel> models{
        SceneModel{&base, {SphereStyle{}}, nullptr},
        SceneModel{&base, {SphereStyle{}}, nullptr},
    };

    AtomData atoms;
    BondData bonds;
    BoundingBox bounds;
    build_scene(models, atoms, bonds, bounds);

    EXPECT_EQ(atoms.size(), 4u);  // 2 atoms x 2 models, appended into one buffer
    EXPECT_TRUE(bounds.valid());  // scene bounds survive move + append
}

TEST(SceneBuilderTest, MultipleStickModelsAccumulateHalfBonds) {
    OEChem::OEGraphMol mol = make_co();
    const OEChem::OEMolBase& base = mol;
    // Two stick-styled models -> bonds accumulate via the append (non-move) path.
    std::vector<SceneModel> models{
        SceneModel{&base, {StickStyle{}}, nullptr},
        SceneModel{&base, {StickStyle{}}, nullptr},
    };

    AtomData atoms;
    BondData bonds;
    BoundingBox bounds;
    build_scene(models, atoms, bonds, bounds);

    EXPECT_EQ(bonds.size(), 4u);  // 2 half-bonds x 2 models, appended into one buffer
    EXPECT_TRUE(bounds.valid());
}

}  // namespace test
}  // namespace oe3dmol
