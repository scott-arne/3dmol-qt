#include <gtest/gtest.h>

#include "oe3dmol/bond_adapter.h"
#include "oe3dmol/data_errors.h"
#include "oe3dmol/color_schemes.h"

#include <oechem.h>
#include <vector>
#include <cmath>

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

TEST(BondOffsetDirTest, PerpendicularToAxisTowardNeighbor) {
    // axis along +x; neighbor sits up and to the side. Offset must be perpendicular
    // to the axis and point toward the neighbor's perpendicular component (+y).
    QVector3D axis(1, 0, 0);
    QVector3D ref(0.5f, 1.0f, 0.0f);  // neighbor relative to bond start
    QVector3D d = bond_offset_dir(axis, ref);
    EXPECT_NEAR(d.length(), 1.0f, 1e-4f);          // unit
    EXPECT_NEAR(QVector3D::dotProduct(d, axis), 0.0f, 1e-4f);  // perpendicular
    EXPECT_GT(d.y(), 0.9f);                         // points toward +y neighbor
}

TEST(BondOffsetDirTest, CollinearNeighborFallsBackToValidPerpendicular) {
    // neighbor exactly along the axis -> perpendicular component ~0 -> fallback.
    QVector3D axis(0, 0, 1);
    QVector3D ref(0, 0, 5);  // collinear
    QVector3D d = bond_offset_dir(axis, ref);
    EXPECT_NEAR(d.length(), 1.0f, 1e-4f);          // still unit (fallback used)
    EXPECT_NEAR(QVector3D::dotProduct(d, axis), 0.0f, 1e-4f);  // still perpendicular
}

TEST(BondAdapterTest, DoubleBondYieldsTwoParallelLines) {
    // C=O double bond, with a neighbor on carbon so the offset plane is defined.
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* c = mol.NewAtom(6);
    OEChem::OEAtomBase* o = mol.NewAtom(8);
    OEChem::OEAtomBase* h = mol.NewAtom(1);  // neighbor on carbon
    double pc[3] = {0, 0, 0};
    double po[3] = {1.2, 0, 0};
    double ph[3] = {-0.5, 1.0, 0};  // off-axis -> defines the plane
    mol.SetCoords(c, pc); mol.SetCoords(o, po); mol.SetCoords(h, ph);
    mol.NewBond(c, o, 2);  // double
    mol.NewBond(c, h, 1);

    StickStyle style;  // single_bonds defaults false
    BondData b = build_bond_data(as_base(mol), style);
    // double C=O -> 2 lines * 2 halves = 4; single C-H -> 2. Total 6.
    EXPECT_EQ(b.size(), 6u);

    // The two C=O lines must be offset in OPPOSITE perpendicular directions:
    // collect the C-side half-bond starts for bond index 0 and check they differ
    // and are symmetric about the original carbon position (0,0,0).
    std::vector<QVector3D> co_starts;
    for (std::size_t i = 0; i < b.size(); ++i)
        if (b.source_bond_idx[i] == 0u &&
            (b.starts[i] - QVector3D(0, 0, 0)).length() < 0.7f)  // carbon-side halves
            co_starts.push_back(b.starts[i]);
    ASSERT_EQ(co_starts.size(), 2u);
    // Symmetric about the carbon center: midpoint of the two offset starts ~ (0,0,0).
    QVector3D mid = (co_starts[0] + co_starts[1]) * 0.5f;
    EXPECT_NEAR(mid.x(), 0.0f, 1e-3f);
    EXPECT_NEAR(mid.y(), 0.0f, 1e-3f);
    EXPECT_NEAR(mid.z(), 0.0f, 1e-3f);
    // Offset is perpendicular to the bond axis (x): the two starts share x, differ in y.
    EXPECT_NEAR(co_starts[0].x(), co_starts[1].x(), 1e-3f);
    EXPECT_GT((co_starts[0] - co_starts[1]).length(), 1e-2f);
}

TEST(BondAdapterTest, TripleBondYieldsThreeLines) {
    // C#C triple with neighbors so the plane is defined.
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* c1 = mol.NewAtom(6);
    OEChem::OEAtomBase* c2 = mol.NewAtom(6);
    OEChem::OEAtomBase* h = mol.NewAtom(1);
    double p1[3] = {0, 0, 0};
    double p2[3] = {1.2, 0, 0};
    double ph[3] = {-0.5, 1.0, 0};
    mol.SetCoords(c1, p1); mol.SetCoords(c2, p2); mol.SetCoords(h, ph);
    mol.NewBond(c1, c2, 3);  // triple
    mol.NewBond(c1, h, 1);

    StickStyle style;
    BondData b = build_bond_data(as_base(mol), style);
    // triple -> 3 lines * 2 = 6; single C-H -> 2. Total 8.
    EXPECT_EQ(b.size(), 8u);
}

TEST(BondAdapterTest, SingleBondsTrueCollapsesMultiplicity) {
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* c = mol.NewAtom(6);
    OEChem::OEAtomBase* o = mol.NewAtom(8);
    double pc[3] = {0, 0, 0};
    double po[3] = {1.2, 0, 0};
    mol.SetCoords(c, pc); mol.SetCoords(o, po);
    mol.NewBond(c, o, 2);  // double

    StickStyle style;
    style.single_bonds(true);  // force plain sticks
    BondData b = build_bond_data(as_base(mol), style);
    EXPECT_EQ(b.size(), 2u);  // collapsed to a single line
}

TEST(BondAdapterTest, DoubleBondWithNoNeighborStillRenders) {
    // Isolated diatomic double bond: no neighbor -> fallback perpendicular.
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* c = mol.NewAtom(6);
    OEChem::OEAtomBase* o = mol.NewAtom(8);
    double pc[3] = {0, 0, 0};
    double po[3] = {1.2, 0, 0};
    mol.SetCoords(c, pc); mol.SetCoords(o, po);
    mol.NewBond(c, o, 2);

    StickStyle style;
    BondData b = build_bond_data(as_base(mol), style);
    EXPECT_EQ(b.size(), 4u);  // still two parallel lines, no crash
}

TEST(BondAdapterTest, AromaticBondYieldsMainPlusShorterThinnerInnerLine) {
    // Build a 3-atom fragment, mark the central bond aromatic, give a neighbor
    // so the inner line offsets toward the ring interior.
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* c1 = mol.NewAtom(6);
    OEChem::OEAtomBase* c2 = mol.NewAtom(6);
    OEChem::OEAtomBase* c3 = mol.NewAtom(6);  // neighbor on c1
    double p1[3] = {0, 0, 0};
    double p2[3] = {1.4, 0, 0};
    double p3[3] = {-0.7, 1.2, 0};
    mol.SetCoords(c1, p1); mol.SetCoords(c2, p2); mol.SetCoords(c3, p3);
    OEChem::OEBondBase* bond = mol.NewBond(c1, c2, 1);
    bond->SetAromatic(true);
    c1->SetAromatic(true);
    c2->SetAromatic(true);
    mol.NewBond(c1, c3, 1);

    StickStyle style;  // radius 0.25
    const float r = 0.25f;
    BondData b = build_bond_data(as_base(mol), style);

    // Aromatic bond (idx 0) -> main pair (2) + inner pair (2) = 4 half-bonds.
    // Plus the single c1-c3 bond (idx 1) -> 2. Total 6.
    EXPECT_EQ(b.size(), 6u);

    // Among bond idx 0 half-bonds, the MAIN line uses radius r; the INNER line
    // uses a smaller radius. Collect distinct radii for bond 0.
    bool has_main_radius = false, has_inner_radius = false;
    float main_len_sq = 0.0f, inner_len_sq = 1e9f;
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (b.source_bond_idx[i] != 0u) continue;
        if (std::abs(b.radii[i] - r) < 1e-4f) has_main_radius = true;
        if (b.radii[i] < r - 1e-4f) has_inner_radius = true;
        const float seg = (b.ends[i] - b.starts[i]).lengthSquared();  // half-line length^2
        if (std::abs(b.radii[i] - r) < 1e-4f) main_len_sq = std::max(main_len_sq, seg);
        else inner_len_sq = std::min(inner_len_sq, seg);
    }
    EXPECT_TRUE(has_main_radius);   // main line at full radius
    EXPECT_TRUE(has_inner_radius);  // inner line is thinner
    EXPECT_LT(inner_len_sq, main_len_sq);  // inner line is shorter
}

}  // namespace test
}  // namespace oe3dmol