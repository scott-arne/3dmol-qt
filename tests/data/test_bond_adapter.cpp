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

TEST(BondAdapterTest, OpacityAppliedToHalfBondAlpha) {
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* a = mol.NewAtom(6);
    OEChem::OEAtomBase* o = mol.NewAtom(8);
    double ca[3] = {0, 0, 0};
    double co[3] = {2, 0, 0};
    mol.SetCoords(a, ca);
    mol.SetCoords(o, co);
    mol.NewBond(a, o, 1);

    StickStyle style;
    style.opacity(0.5);
    BondData b = build_bond_data(as_base(mol), style);
    ASSERT_EQ(b.size(), 2u);
    for (std::size_t i = 0; i < b.size(); ++i) {
        // QColor stores 8-bit alpha; 0.5 round-trips within 1/255.
        EXPECT_NEAR(b.colors[i].alphaF(), 0.5f, 1.0f / 255.0f);
    }
}

TEST(BondAdapterTest, OpacityClampedAboveOne) {
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* a = mol.NewAtom(6);
    OEChem::OEAtomBase* o = mol.NewAtom(8);
    double ca[3] = {0, 0, 0};
    double co[3] = {2, 0, 0};
    mol.SetCoords(a, ca);
    mol.SetCoords(o, co);
    mol.NewBond(a, o, 1);

    StickStyle style;
    style.opacity(2.0);  // out of range -> clamp to 1.0
    BondData b = build_bond_data(as_base(mol), style);
    ASSERT_EQ(b.size(), 2u);
    EXPECT_NEAR(b.colors[0].alphaF(), 1.0f, 1e-4f);
}

TEST(BondAdapterTest, MultiBondYieldsTwoHalvesPerBond) {
    // Linear C-C-O: 2 bonds -> 4 half-bonds.
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* c1 = mol.NewAtom(6);
    OEChem::OEAtomBase* c2 = mol.NewAtom(6);
    OEChem::OEAtomBase* o = mol.NewAtom(8);
    double p1[3] = {0, 0, 0};
    double p2[3] = {1.5, 0, 0};
    double p3[3] = {3.0, 0, 0};
    mol.SetCoords(c1, p1);
    mol.SetCoords(c2, p2);
    mol.SetCoords(o, p3);
    mol.NewBond(c1, c2, 1);
    mol.NewBond(c2, o, 1);

    StickStyle style;
    BondData b = build_bond_data(as_base(mol), style);
    EXPECT_EQ(b.size(), 4u);  // 2 bonds * 2 halves
}

TEST(BondAdapterTest, CoincidentAtomBondIsSkipped) {
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* a = mol.NewAtom(6);
    OEChem::OEAtomBase* b2 = mol.NewAtom(6);
    double same[3] = {1.0, 1.0, 1.0};
    mol.SetCoords(a, same);
    mol.SetCoords(b2, same);  // coincident
    mol.NewBond(a, b2, 1);

    StickStyle style;
    BondData b = build_bond_data(as_base(mol), style);
    EXPECT_EQ(b.size(), 0u);  // degenerate bond skipped
}

}  // namespace test
}  // namespace oe3dmol
