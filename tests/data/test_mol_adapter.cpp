#include <gtest/gtest.h>

#include "oe3dmol/mol_adapter.h"
#include "oe3dmol/data_errors.h"

#include <oechem.h>

namespace oe3dmol {
namespace test {

namespace {
// OEGraphMol does not derive from OEMolBase; it converts implicitly to
// OEMolBase&. Real callers hold an OEMolBase*, so the tests acquire one the
// same way before calling build_atom_data.
const OEChem::OEMolBase* as_base(const OEChem::OEGraphMol& mol) {
    const OEChem::OEMolBase& base = mol;
    return &base;
}
}  // namespace

TEST(MolAdapterTest, NullMoleculeThrows) {
    SphereStyle style;
    EXPECT_THROW(build_atom_data(nullptr, style), DataError);
}

TEST(MolAdapterTest, EmptyMoleculeThrows) {
    OEChem::OEGraphMol mol;
    SphereStyle style;
    EXPECT_THROW(build_atom_data(as_base(mol), style), DataError);
}

TEST(MolAdapterTest, SingleCarbonHasOneAtomVdwRadius) {
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* a = mol.NewAtom(6);  // carbon
    double coords[3] = {1.0, 2.0, 3.0};
    mol.SetCoords(a, coords);

    SphereStyle style;  // Jmol, no explicit radius/scale
    AtomData data = build_atom_data(as_base(mol), style);

    ASSERT_EQ(data.size(), 1u);
    EXPECT_FLOAT_EQ(data.positions[0].x(), 1.0f);
    EXPECT_FLOAT_EQ(data.positions[0].y(), 2.0f);
    EXPECT_FLOAT_EQ(data.positions[0].z(), 3.0f);
    EXPECT_FLOAT_EQ(data.radii[0], 1.7f);  // carbon VDW
}

TEST(MolAdapterTest, CarbonGetsJmolGreyColor) {
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* a = mol.NewAtom(6);
    double coords[3] = {0.0, 0.0, 0.0};
    mol.SetCoords(a, coords);

    SphereStyle style;
    AtomData data = build_atom_data(as_base(mol), style);

    ASSERT_EQ(data.size(), 1u);
    EXPECT_EQ(data.colors[0].red(), 0x90);
    EXPECT_EQ(data.colors[0].green(), 0x90);
    EXPECT_EQ(data.colors[0].blue(), 0x90);
}

TEST(MolAdapterTest, BoundsCoverAllAtoms) {
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* a = mol.NewAtom(6);
    OEChem::OEAtomBase* b = mol.NewAtom(8);  // oxygen
    double ca[3] = {0.0, 0.0, 0.0};
    double cb[3] = {2.0, 0.0, 0.0};
    mol.SetCoords(a, ca);
    mol.SetCoords(b, cb);

    SphereStyle style;
    AtomData data = build_atom_data(as_base(mol), style);

    ASSERT_EQ(data.size(), 2u);
    EXPECT_TRUE(data.bounds.valid());
    EXPECT_GT(data.bounds.center().x(), 0.0f);
    EXPECT_LT(data.bounds.center().x(), 2.0f);
}

}  // namespace test
}  // namespace oe3dmol
