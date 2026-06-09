#include <gtest/gtest.h>

#include "oe3dmol/bond_adapter.h"
#include "oe3dmol/data_errors.h"
#include "oe3dmol/color_schemes.h"

#include <oechem.h>

namespace oe3dmol {
namespace test {

namespace {
const OEChem::OEMolBase* as_base(const OEChem::OEGraphMol& mol) {
    const OEChem::OEMolBase& base = mol;
    return &base;
}
}  // namespace

TEST(BondAdapterTest, NullMoleculeThrows) {
    StickStyle style;
    EXPECT_THROW(build_bond_data(nullptr, style), DataError);
}

TEST(BondAdapterTest, NoBondsYieldsEmpty) {
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* a = mol.NewAtom(6);
    double c[3] = {0, 0, 0};
    mol.SetCoords(a, c);
    StickStyle style;
    BondData b = build_bond_data(as_base(mol), style);
    EXPECT_EQ(b.size(), 0u);
}

TEST(BondAdapterTest, SingleBondYieldsTwoHalfBonds) {
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* a = mol.NewAtom(6);   // carbon
    OEChem::OEAtomBase* o = mol.NewAtom(8);   // oxygen
    double ca[3] = {0, 0, 0};
    double co[3] = {2, 0, 0};
    mol.SetCoords(a, ca);
    mol.SetCoords(o, co);
    mol.NewBond(a, o, 1);

    StickStyle style;  // radius 0.25, Jmol
    BondData b = build_bond_data(as_base(mol), style);
    ASSERT_EQ(b.size(), 2u);

    bool has_c_start = false, has_o_start = false;
    for (std::size_t i = 0; i < b.size(); ++i) {
        EXPECT_FLOAT_EQ(b.ends[i].x(), 1.0f);   // midpoint
        EXPECT_FLOAT_EQ(b.radii[i], 0.25f);
        if (b.starts[i].x() == 0.0f) has_c_start = true;
        if (b.starts[i].x() == 2.0f) has_o_start = true;
    }
    EXPECT_TRUE(has_c_start);
    EXPECT_TRUE(has_o_start);
}

TEST(BondAdapterTest, HalvesUseNearAtomColors) {
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* a = mol.NewAtom(6);
    OEChem::OEAtomBase* o = mol.NewAtom(8);
    double ca[3] = {0, 0, 0};
    double co[3] = {2, 0, 0};
    mol.SetCoords(a, ca);
    mol.SetCoords(o, co);
    mol.NewBond(a, o, 1);

    StickStyle style;
    BondData b = build_bond_data(as_base(mol), style);
    ASSERT_EQ(b.size(), 2u);

    const Color c_color = element_color(ColorScheme::Jmol, "C");  // 0x909090
    const Color o_color = element_color(ColorScheme::Jmol, "O");  // 0xff0d0d
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (b.starts[i].x() == 0.0f) {
            EXPECT_EQ(b.colors[i].rgb(), c_color.rgb());  // carbon half grey
        } else {
            EXPECT_EQ(b.colors[i].rgb(), o_color.rgb());  // oxygen half red
        }
    }
}

}  // namespace test
}  // namespace oe3dmol
