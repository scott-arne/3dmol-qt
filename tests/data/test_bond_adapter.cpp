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

TEST(BondOffsetDirTest, ZeroAxisReturnsFiniteUnitVector) {
    QVector3D d = bond_offset_dir(QVector3D(0, 0, 0), QVector3D(0, 0, 0));
    // Contract: always a finite unit vector, even for a zero axis + zero ref.
    EXPECT_TRUE(std::isfinite(d.x()) && std::isfinite(d.y()) && std::isfinite(d.z()));
    EXPECT_NEAR(d.length(), 1.0f, 1e-4f);
    // Header contract: a zero axis returns the canonical +X axis specifically.
    EXPECT_NEAR(d.x(), 1.0f, 1e-4f);
    EXPECT_NEAR(d.y(), 0.0f, 1e-4f);
    EXPECT_NEAR(d.z(), 0.0f, 1e-4f);
}

TEST(BondOffsetDirTest, ZeroAxisWithRefStillFiniteUnit) {
    QVector3D d = bond_offset_dir(QVector3D(0, 0, 0), QVector3D(3, 0, 0));
    EXPECT_TRUE(std::isfinite(d.x()) && std::isfinite(d.y()) && std::isfinite(d.z()));
    EXPECT_NEAR(d.length(), 1.0f, 1e-4f);
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

TEST(BondAdapterTest, DelocalizedBenzeneHasFullBondsPlusThinnerCircle) {
    // Delocalized aromatic style on a benzene ring: the 6 ring BONDS render as
    // full-radius single cylinders (12 half-bonds at radius r), PLUS a dashed
    // circle of THINNER chord segments inside the ring. Verify the circle
    // segments are thinner than the bonds and lie inside the atom ring.
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* c[6];
    const double R = 1.4;
    for (int i = 0; i < 6; ++i) {
        c[i] = mol.NewAtom(6);
        const double a = (M_PI / 3.0) * i;
        double p[3] = {R * std::cos(a), R * std::sin(a), 0.0};
        mol.SetCoords(c[i], p);
    }
    for (int i = 0; i < 6; ++i) {
        OEChem::OEBondBase* b = mol.NewBond(c[i], c[(i + 1) % 6], (i % 2 == 0) ? 2 : 1);
        b->SetAromatic(true);
        c[i]->SetAromatic(true);
    }

    StickStyle style;
    style.aromatic_style(AromaticStyle::Delocalized);
    const float r = 0.25f;
    BondData b = build_bond_data(as_base(mol), style);

    int full = 0, thin = 0;
    float max_thin_dist = 0.0f;  // circle chord distance from ring center (origin)
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (std::abs(b.radii[i] - r) < 1e-4f) {
            ++full;
        } else if (b.radii[i] < r - 1e-4f) {
            ++thin;
            max_thin_dist = std::max(max_thin_dist, b.starts[i].length());
            // circle lies in the ring plane (z ~ 0 for this xy-planar benzene)
            EXPECT_NEAR(b.starts[i].z(), 0.0f, 1e-3f);
        }
    }
    EXPECT_EQ(full, 12) << "6 aromatic ring bonds -> 12 full-radius half-bonds";
    EXPECT_GT(thin, 4) << "dashed circle should add multiple thin chord segments";
    // The circle sits INSIDE the atoms: every chord vertex is closer to center
    // than the atoms (R = 1.4).
    EXPECT_LT(max_thin_dist, static_cast<float>(R))
        << "circle must lie inside the atom ring";
}

TEST(BondAdapterTest, DelocalizedFusedRingsYieldTwoCircles) {
    // Fused bicyclic aromatic (two rings sharing an edge) -> two distinct circles.
    // Build naphthalene-like fused 6+6 sharing the c0-c1 edge, all aromatic.
    OEChem::OEGraphMol mol;
    // Ring A: c0..c5 hexagon; Ring B shares edge c0-c1, adds c6..c9.
    OEChem::OEAtomBase* a[10];
    // Simple planar coords: ring A centered at x=-0.7, ring B at x=+0.7 (shared edge at x=0).
    double coords[10][3] = {
        { 0.0,  0.7, 0}, { 0.0, -0.7, 0},        // shared edge c0(top), c1(bottom)
        {-1.2,  1.4, 0}, {-2.4,  0.7, 0}, {-2.4, -0.7, 0}, {-1.2, -1.4, 0},  // ring A c2..c5
        { 1.2,  1.4, 0}, { 2.4,  0.7, 0}, { 2.4, -0.7, 0}, { 1.2, -1.4, 0},  // ring B c6..c9
    };
    for (int i = 0; i < 10; ++i) { a[i] = mol.NewAtom(6); mol.SetCoords(a[i], coords[i]); a[i]->SetAromatic(true); }
    auto bond = [&](int i, int j) { OEChem::OEBondBase* b = mol.NewBond(a[i], a[j], 1); b->SetAromatic(true); };
    bond(0, 1);                                   // shared edge
    bond(0, 2); bond(2, 3); bond(3, 4); bond(4, 5); bond(5, 1);  // ring A
    bond(0, 6); bond(6, 7); bond(7, 8); bond(8, 9); bond(9, 1);  // ring B

    StickStyle style;
    style.aromatic_style(AromaticStyle::Delocalized);
    const float r = 0.25f;
    BondData b = build_bond_data(as_base(mol), style);

    // Collect thin (circle) chord vertices and cluster by x-sign: ring A is at
    // x<0, ring B at x>0. Both clusters must be non-empty -> two circles drawn.
    int left = 0, right = 0;
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (b.radii[i] < r - 1e-4f) {  // a circle chord
            if (b.starts[i].x() < -0.2f) ++left;
            else if (b.starts[i].x() > 0.2f) ++right;
        }
    }
    EXPECT_GT(left, 0) << "ring A circle missing";
    EXPECT_GT(right, 0) << "ring B circle missing";
}

TEST(BondAdapterTest, KekuleAromaticRingRendersAlternatingSingleDouble) {
    // Default (Kekule) aromatic style: a benzene ring whose bonds already carry
    // alternating orders 2/1/2/1/2/1 (as OEReadMolecule produces) AND aromatic
    // flags must render as 3 single + 3 double bonds -- NO dashed segments, all
    // lines at full radius or the double sub-radius. Verifies we read GetOrder()
    // (after clear+kekulize on the internal copy), not the dashed delocalized path.
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* c[6];
    const double R = 1.4;
    for (int i = 0; i < 6; ++i) {
        c[i] = mol.NewAtom(6);
        const double a = (M_PI / 3.0) * i;
        double p[3] = {R * std::cos(a), R * std::sin(a), 0.0};
        mol.SetCoords(c[i], p);
    }
    for (int i = 0; i < 6; ++i) {
        OEChem::OEBondBase* b = mol.NewBond(c[i], c[(i + 1) % 6], (i % 2 == 0) ? 2 : 1);
        b->SetAromatic(true);
        c[i]->SetAromatic(true);
    }

    StickStyle style;  // default aromatic_style == Kekule
    const float r = 0.25f;
    const float dbl = r * 0.4f;  // double sub-cylinder radius
    BondData b = build_bond_data(as_base(mol), style);

    // Each bond renders by order: single -> 2 half-bonds at full radius r; double
    // -> 4 half-bonds at radius dbl. 3 singles + 3 doubles = 6 + 12 = 18 halves.
    EXPECT_EQ(b.size(), 18u);
    int full_r = 0, double_r = 0, other = 0;
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (std::abs(b.radii[i] - r) < 1e-4f) ++full_r;
        else if (std::abs(b.radii[i] - dbl) < 1e-4f) ++double_r;
        else ++other;
    }
    EXPECT_EQ(full_r, 6) << "3 single bonds -> 6 full-radius half-bonds";
    EXPECT_EQ(double_r, 12) << "3 double bonds -> 12 sub-radius half-bonds";
    EXPECT_EQ(other, 0) << "no dashed/other-radius segments in Kekule mode";
}

}  // namespace test
}  // namespace oe3dmol