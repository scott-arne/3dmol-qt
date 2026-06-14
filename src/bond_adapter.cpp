#include "oe3dmol/bond_adapter.h"

#include "oe3dmol/color_schemes.h"
#include "oe3dmol/data_errors.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include <oechem.h>

namespace oe3dmol {

namespace {

QVector3D atom_pos(const OEChem::OEMolBase* mol, const OEChem::OEAtomBase* atom) {
    double c[3] = {0.0, 0.0, 0.0};
    mol->GetCoords(atom, c);
    return QVector3D(static_cast<float>(c[0]), static_cast<float>(c[1]),
                     static_cast<float>(c[2]));
}

Color atom_color(ColorScheme scheme, const OEChem::OEAtomBase* atom, double opacity) {
    const std::string element = OEChem::OEGetAtomicSymbol(atom->GetAtomicNum());
    Color c = element_color(scheme, element);
    c.setAlphaF(static_cast<float>(std::clamp(opacity, 0.0, 1.0)));
    return c;
}

// Emit one rendered cylinder line from `a` to `b` as two half-bonds split at the
// line's midpoint, each half taking its near-end color. Shared by every bond
// multiplicity so coloring/splitting logic lives in one place.
void add_line(BondData& data, const QVector3D& a, const QVector3D& b, float radius,
              const Color& color_a, const Color& color_b, unsigned bond_index) {
    const QVector3D mid = (a + b) * 0.5f;
    data.add(a, mid, radius, color_a, bond_index);
    data.add(b, mid, radius, color_b, bond_index);
}

// Multi-bond geometry, ported from 3dmol.js (drawBondSticks defaults). A
// multi-bond's parallel lines fit within a single bond's footprint: each line is
// a thin sub-cylinder and the pair straddles the axis so the whole reads as one
// split bond. Values are 3dmol's doubleBondScaling=0.4 / tripleBondScaling=0.25.
constexpr float DOUBLE_BOND_SCALE = 0.4f;    // double sub-cylinder radius / R
constexpr float TRIPLE_BOND_SCALE = 0.25f;   // triple sub-cylinder radius / R
constexpr float DOUBLE_OFFSET = 0.6f;        // double line offset / R   (3dmol: r*1.5)
constexpr float TRIPLE_OFFSET = 0.75f;       // triple outer offset / R  (3dmol: r*3)
// Aromatic = a double-like straddling pair: a solid outer line + a thinner DASHED
// inner line toward the ring interior (3dmol's default "dashed" aromaticStyle).
constexpr float AROMATIC_DASH_RADIUS_FRAC = 0.5f;  // dashed line radius / solid line radius
constexpr float DASH_LENGTH = 0.10f;  // 3dmol dashedBondConfig.dashLength
constexpr float DASH_GAP = 0.25f;     // 3dmol dashedBondConfig.gapLength
// Aromatic delocalized circle (Delocalized style): a thin dashed ring inside
// each aromatic ring. Tuned visually on benzene.
constexpr float RING_CIRCLE_FRACTION = 0.6f;   // circle radius / ring apothem (sits inside bonds)
constexpr float RING_CIRCLE_TUBE_FRAC = 0.55f; // circle tube radius / bond radius (thinner than bonds)
constexpr int   CIRCLE_SEGMENTS = 48;          // chords around the circle

// Accumulate the neighbor of `atom` (other than `exclude`) whose bond is MOST
// perpendicular to `axis` -- the largest cross-product magnitude. This is 3dmol's
// getGoodCross criterion: the most-divergent neighbor gives a well-conditioned,
// in-molecular-plane offset direction for multi-bond lines, avoiding the
// near-collinear (degenerate) neighbors that make offsets jitter around fused
// rings. Updates `best`/`out` in place; call for both bond atoms.
void accumulate_divergent_neighbor(const OEChem::OEMolBase* mol,
                                   const OEChem::OEAtomBase* atom,
                                   const OEChem::OEAtomBase* exclude,
                                   const QVector3D& axis, float& best, QVector3D& out) {
    const QVector3D p = atom_pos(mol, atom);
    for (OESystem::OEIter nbr = atom->GetAtoms(); nbr; ++nbr) {
        const OEChem::OEAtomBase* n = nbr;
        if (n == exclude) continue;
        const QVector3D np = atom_pos(mol, n);
        const float l = QVector3D::crossProduct(np - p, axis).lengthSquared();
        if (l > best) {
            best = l;
            out = np;
        }
    }
}

// Emit a dashed line from `a` to `b` as a series of short cylinder segments,
// ported from 3dmol's calculateDashes (dash 0.1 A, gap 0.25 A + 2*radius,
// end-padded and centered). Each segment is colored by the near atom, split at
// `color_mid` (the line's own midpoint). Used for the aromatic inner line.
void add_dashed_line(BondData& data, const QVector3D& a, const QVector3D& b, float radius,
                     const Color& color_a, const Color& color_b, const QVector3D& color_mid,
                     unsigned bond_index) {
    const QVector3D axis = b - a;
    const float len = axis.length();
    if (len < 1.0e-3f) {
        return;
    }
    const QVector3D dir = axis / len;
    auto seg_color = [&](const QVector3D& s, const QVector3D& e) {
        const QVector3D m = (s + e) * 0.5f;
        return QVector3D::dotProduct(m - color_mid, dir) < 0.0f ? color_a : color_b;
    };
    const float eff_gap = DASH_GAP + 2.0f * radius;
    const float end_pad = eff_gap * 0.5f;
    const float drawable = len - 2.0f * end_pad;
    if (drawable < DASH_LENGTH) {  // too short for dashes: one solid segment
        data.add(a, b, radius, seg_color(a, b), bond_index);
        return;
    }
    const int num = static_cast<int>((drawable - DASH_LENGTH) / (DASH_LENGTH + eff_gap)) + 1;
    const int gaps = num > 1 ? num - 1 : 0;
    const float remaining = drawable - num * DASH_LENGTH;
    const float internal_gap = gaps > 0 ? remaining / (gaps + 1) : remaining;
    const float start = end_pad + internal_gap * 0.5f;
    for (int i = 0; i < num; ++i) {
        const float s = start + i * (DASH_LENGTH + internal_gap);
        const QVector3D ps = a + dir * s;
        const QVector3D pe = a + dir * (s + DASH_LENGTH);
        data.add(ps, pe, radius, seg_color(ps, pe), bond_index);
    }
}

// Choose which side of the bond (+off or -off) the aromatic dashed inner line
// goes: toward the average direction of the bond atoms' OTHER neighbors, which
// points into the ring interior. Port of 3dmol's chooseDashedPlusV. Returns +1
// for the +off side, -1 for the -off side.
float aromatic_dash_sign(const OEChem::OEMolBase* mol, const OEChem::OEAtomBase* abgn,
                         const OEChem::OEAtomBase* aend, const QVector3D& mid,
                         const QVector3D& off) {
    QVector3D center(0, 0, 0);
    int count = 0;
    for (const OEChem::OEAtomBase* a : {abgn, aend}) {
        const OEChem::OEAtomBase* other = (a == abgn) ? aend : abgn;
        for (OESystem::OEIter nbr = a->GetAtoms(); nbr; ++nbr) {
            const OEChem::OEAtomBase* n = nbr;
            if (n == other) continue;
            center += atom_pos(mol, n);
            ++count;
        }
    }
    if (count == 0) {
        return -1.0f;  // no neighbors: arbitrary (interior is undefined)
    }
    center /= static_cast<float>(count);
    return QVector3D::dotProduct(off, center - mid) > 0.0f ? 1.0f : -1.0f;
}

// Smallest aromatic rings as ordered atom cycles. Ported from 3dmol's
// findSmallestRing: for each aromatic bond, BFS from one endpoint back to the
// other through ONLY aromatic bonds (bounded to <=8-membered rings), recovering
// the shortest cycle; dedup rings by their sorted atom-index set so a fused
// system (e.g. indazole) yields one cycle PER smallest ring. OE exposes no
// smallest-ring atom-list enumerator, so we walk the bond graph ourselves.
std::vector<std::vector<const OEChem::OEAtomBase*>> aromatic_rings(
    const OEChem::OEMolBase* mol) {
    constexpr unsigned MAX_RING = 8;
    std::vector<std::vector<const OEChem::OEAtomBase*>> rings;
    std::set<std::vector<unsigned>> seen;  // dedup keys (sorted atom idx)

    for (OESystem::OEIter bond = mol->GetBonds(); bond; ++bond) {
        const OEChem::OEBondBase& b = *bond;
        if (!b.IsAromatic()) continue;
        const OEChem::OEAtomBase* start = b.GetBgn();
        const OEChem::OEAtomBase* goal = b.GetEnd();
        if (start == nullptr || goal == nullptr) continue;

        // BFS for the shortest path start->goal that does NOT use bond b directly,
        // through aromatic bonds only. parent[] reconstructs the path.
        std::queue<const OEChem::OEAtomBase*> q;
        std::map<unsigned, const OEChem::OEAtomBase*> parent;  // atomIdx -> predecessor
        std::map<unsigned, unsigned> depth;
        q.push(start);
        depth[start->GetIdx()] = 0;
        parent[start->GetIdx()] = nullptr;
        const OEChem::OEAtomBase* found = nullptr;
        while (!q.empty() && found == nullptr) {
            const OEChem::OEAtomBase* cur = q.front();
            q.pop();
            const unsigned cd = depth[cur->GetIdx()];
            if (cd >= MAX_RING) continue;
            for (OESystem::OEIter nb = cur->GetBonds(); nb; ++nb) {
                const OEChem::OEBondBase& e = *nb;
                if (!e.IsAromatic()) continue;
                if (&e == &b) continue;  // don't traverse the closing bond itself
                const OEChem::OEAtomBase* nxt =
                    (e.GetBgn() == cur) ? e.GetEnd() : e.GetBgn();
                if (nxt == nullptr) continue;
                if (depth.count(nxt->GetIdx())) continue;  // visited
                depth[nxt->GetIdx()] = cd + 1;
                parent[nxt->GetIdx()] = cur;
                if (nxt == goal) { found = nxt; break; }
                q.push(nxt);
            }
        }
        if (found == nullptr) continue;  // no ring through this bond within bound

        // Reconstruct goal->start path, then the ring is path + closing bond.
        std::vector<const OEChem::OEAtomBase*> cycle;
        for (const OEChem::OEAtomBase* a = goal; a != nullptr;
             a = parent[a->GetIdx()]) {
            cycle.push_back(a);
        }
        // cycle currently goal..start; that IS the ring (start and goal are the
        // closing bond's two atoms, adjacent via b).
        std::vector<unsigned> key;
        key.reserve(cycle.size());
        for (const OEChem::OEAtomBase* a : cycle) key.push_back(a->GetIdx());
        std::sort(key.begin(), key.end());
        if (seen.insert(key).second) {
            rings.push_back(std::move(cycle));
        }
    }
    return rings;
}

// Emit a dashed circle inside one aromatic ring as short cylinder chords. The
// circle lies in the ring's mean plane (Newell normal), centered on the ring
// centroid, at RING_CIRCLE_FRACTION of the apothem so it sits clearly inside the
// bonds, with a tube radius thinner than the bonds. Dashed by drawing every
// other chord. No-op for degenerate (near-collinear) rings.
void add_aromatic_circle(BondData& data, const std::vector<QVector3D>& ring_pos,
                         float bond_radius, const Color& color, unsigned bond_index) {
    const std::size_t n = ring_pos.size();
    if (n < 3) return;
    QVector3D centroid(0, 0, 0);
    for (const QVector3D& p : ring_pos) centroid += p;
    centroid /= static_cast<float>(n);

    // Newell's method for a robust plane normal over the ordered cycle.
    QVector3D normal(0, 0, 0);
    for (std::size_t i = 0; i < n; ++i) {
        const QVector3D& cur = ring_pos[i];
        const QVector3D& nxt = ring_pos[(i + 1) % n];
        normal += QVector3D(
            (cur.y() - nxt.y()) * (cur.z() + nxt.z()),
            (cur.z() - nxt.z()) * (cur.x() + nxt.x()),
            (cur.x() - nxt.x()) * (cur.y() + nxt.y()));
    }
    if (normal.lengthSquared() < 1.0e-8f) return;  // degenerate ring
    normal.normalize();

    float avg_dist = 0.0f;
    for (const QVector3D& p : ring_pos) avg_dist += (p - centroid).length();
    avg_dist /= static_cast<float>(n);
    const float apothem = avg_dist * std::cos(static_cast<float>(M_PI) / static_cast<float>(n));
    const float circle_r = RING_CIRCLE_FRACTION * apothem;
    const float tube_r = RING_CIRCLE_TUBE_FRAC * bond_radius;

    // In-plane orthonormal basis (u, v).
    QVector3D u = QVector3D::crossProduct(normal, QVector3D(0, 0, 1));
    if (u.lengthSquared() < 1.0e-6f) u = QVector3D::crossProduct(normal, QVector3D(0, 1, 0));
    u.normalize();
    const QVector3D v = QVector3D::crossProduct(normal, u);

    auto pt = [&](int i) {
        const float t = (2.0f * static_cast<float>(M_PI) * static_cast<float>(i)) /
                        static_cast<float>(CIRCLE_SEGMENTS);
        return centroid + circle_r * (std::cos(t) * u + std::sin(t) * v);
    };
    // Dashed: draw every other chord segment.
    for (int i = 0; i < CIRCLE_SEGMENTS; i += 2) {
        data.add(pt(i), pt(i + 1), tube_r, color, bond_index);
    }
}

}  // namespace

QVector3D bond_offset_dir(const QVector3D& axis, const QVector3D& ref) {
    const QVector3D a = axis.normalized();
    QVector3D perp = ref - a * QVector3D::dotProduct(ref, a);
    constexpr float MIN_PERP_SQ = 1.0e-8f;
    if (perp.lengthSquared() >= MIN_PERP_SQ) {
        return perp.normalized();
    }
    // Collinear / no neighbor: pick any axis-perpendicular. cross with X, then Y;
    // at least one is non-degenerate since `a` cannot be parallel to both.
    QVector3D c = QVector3D::crossProduct(a, QVector3D(1, 0, 0));
    if (c.lengthSquared() < MIN_PERP_SQ) {
        c = QVector3D::crossProduct(a, QVector3D(0, 1, 0));
    }
    return c.normalized();
}

BondData build_bond_data(const OEChem::OEMolBase* mol, const StickStyle& style) {
    if (mol == nullptr) {
        throw DataError("build_bond_data: molecule is null");
    }

    BondData data;
    data.reserve(static_cast<std::size_t>(mol->NumBonds()) * 6);
    const float radius = static_cast<float>(style.radius());

    // In Kekule aromatic mode, render the molecule's alternating single/double
    // bond orders instead of a delocalized marker. Realistic inputs (SDF, SMILES,
    // the design molecules here) already carry valid Kekule orders after
    // OEReadMolecule, but to be robust we (re)derive them on a MUTABLE COPY via
    // OEClearAromaticFlags + OEKekulize -- never touching the caller's molecule.
    // OEKekulize may report failure on already-valid / degenerate inputs; in that
    // case the orders it leaves in place are still the on-read Kekule orders, so
    // we use the copy regardless and read GetOrder() from it (the cleared
    // aromatic flags then route every bond through the order-based paths below).
    OEChem::OEGraphMol kekulized;
    const OEChem::OEMolBase* src = mol;
    if (style.aromatic_style() == AromaticStyle::Kekule && !style.single_bonds()) {
        bool any_aromatic = false;
        for (OESystem::OEIter bond = mol->GetBonds(); bond; ++bond) {
            if ((*bond).IsAromatic()) { any_aromatic = true; break; }
        }
        if (any_aromatic) {
            kekulized = *mol;  // OEGraphMol(const OEMolBase&) deep copy
            OEChem::OEClearAromaticFlags(kekulized);
            OEChem::OEKekulize(kekulized);  // best-effort; orders valid either way
            src = &static_cast<const OEChem::OEMolBase&>(kekulized);
        }
    }

    for (OESystem::OEIter bond = src->GetBonds(); bond; ++bond) {
        const OEChem::OEBondBase& b = *bond;
        const OEChem::OEAtomBase* abgn = b.GetBgn();
        const OEChem::OEAtomBase* aend = b.GetEnd();
        if (abgn == nullptr || aend == nullptr) {
            continue;
        }

        const QVector3D p_bgn = atom_pos(src, abgn);
        const QVector3D p_end = atom_pos(src, aend);
        // Skip degenerate bonds: a near-zero axis would yield a degenerate
        // orientation basis in the cylinder shader. 1e-3 A matches the project's
        // existing degenerate-geometry clamp (see Camera::zoom_to).
        constexpr float MIN_BOND_LENGTH_SQ = 1.0e-6f;  // (1e-3 A)^2
        if ((p_end - p_bgn).lengthSquared() < MIN_BOND_LENGTH_SQ) {
            continue;
        }

        const Color c_bgn = atom_color(style.color(), abgn, style.opacity());
        const Color c_end = atom_color(style.color(), aend, style.opacity());
        const unsigned idx = b.GetIdx();

        const unsigned order = b.GetOrder();
        const bool aromatic = b.IsAromatic();

        if (style.single_bonds() || (order <= 1 && !aromatic)) {
            add_line(data, p_bgn, p_end, radius, c_bgn, c_end, idx);
            continue;
        }

        // Offset direction: perpendicular to the bond, toward the MOST-divergent
        // neighbor across both atoms (3dmol's getGoodCross). This lies in the
        // molecular plane / ring interior and is well-conditioned even in fused
        // rings, where the first neighbor is often near-collinear.
        const QVector3D axis = p_end - p_bgn;
        float best = -1.0f;
        QVector3D ref = p_bgn + QVector3D(0, 1, 0);  // fallback; bond_offset_dir handles it
        accumulate_divergent_neighbor(src, abgn, aend, axis, best, ref);
        accumulate_divergent_neighbor(src, aend, abgn, axis, best, ref);
        const QVector3D off = bond_offset_dir(axis, ref - p_bgn);
        const QVector3D mid = (p_bgn + p_end) * 0.5f;

        if (aromatic) {
            // Symmetric straddling pair (like a double bond): a SOLID outer line
            // and a thinner DASHED inner line toward the ring interior -- 3dmol's
            // default aromatic look.
            const float sub_radius = radius * DOUBLE_BOND_SCALE;
            const QVector3D d = off * (DOUBLE_OFFSET * radius);
            const float dash_sign = aromatic_dash_sign(src, abgn, aend, mid, off);
            const QVector3D solid_d = d * -dash_sign;  // solid on the outer side
            const QVector3D dash_d = d * dash_sign;    // dashed toward ring interior
            add_line(data, p_bgn + solid_d, p_end + solid_d, sub_radius, c_bgn, c_end, idx);
            add_dashed_line(data, p_bgn + dash_d, p_end + dash_d,
                            sub_radius * AROMATIC_DASH_RADIUS_FRAC, c_bgn, c_end, mid + dash_d, idx);
            continue;
        }

        if (order == 2) {
            // Two thin cylinders straddling the axis; together ~one bond wide.
            const float sub_radius = radius * DOUBLE_BOND_SCALE;
            const QVector3D d = off * (DOUBLE_OFFSET * radius);
            add_line(data, p_bgn + d, p_end + d, sub_radius, c_bgn, c_end, idx);
            add_line(data, p_bgn - d, p_end - d, sub_radius, c_bgn, c_end, idx);
        } else {  // order >= 3: triple (defensive: treat >3 as triple)
            const float sub_radius = radius * TRIPLE_BOND_SCALE;
            const QVector3D d = off * (TRIPLE_OFFSET * radius);
            add_line(data, p_bgn, p_end, sub_radius, c_bgn, c_end, idx);
            add_line(data, p_bgn + d, p_end + d, sub_radius, c_bgn, c_end, idx);
            add_line(data, p_bgn - d, p_end - d, sub_radius, c_bgn, c_end, idx);
        }
    }

    return data;
}

}  // namespace oe3dmol
