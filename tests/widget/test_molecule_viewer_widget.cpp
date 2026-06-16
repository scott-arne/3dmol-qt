#include <gtest/gtest.h>

#include <QApplication>
#include <QKeyEvent>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QPointF>
#include <QWheelEvent>

#include <oechem.h>

#include "oe3dmol/molecule_viewer_widget.h"
#include "oe3dmol/render_errors.h"
#include "oe3dmol/data_errors.h"
#include "oe3dmol/stick_style.h"
#include "oe3dmol/style_spec.h"

namespace oe3dmol {
namespace test {

// A QApplication (QtWidgets) is required to construct/deliver events to a
// widget. Created once per process. Offscreen platform => no display/GPU needed.
class WidgetEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        static int argc = 1;
        static char arg0[] = "oe3dmol_widget_tests";
        static char* argv[] = {arg0, nullptr};
        app_ = new QApplication(argc, argv);
    }
    void TearDown() override { delete app_; }
private:
    QApplication* app_ = nullptr;
};

::testing::Environment* const kEnv =
    ::testing::AddGlobalTestEnvironment(new WidgetEnvironment);

namespace {

// Subclass exposing the protected event handlers so tests can drive interaction
// directly (no event-loop/show required; update() just schedules a repaint).
class TestableWidget : public MoleculeViewerWidget {
public:
    using MoleculeViewerWidget::MoleculeViewerWidget;

    void press(QPoint p, Qt::MouseButton b, Qt::KeyboardModifiers m = Qt::NoModifier) {
        QMouseEvent e(QEvent::MouseButtonPress, QPointF(p), QPointF(p), b, b, m);
        mousePressEvent(&e);
    }
    void drag(QPoint p, Qt::MouseButtons held, Qt::KeyboardModifiers m = Qt::NoModifier) {
        QMouseEvent e(QEvent::MouseMove, QPointF(p), QPointF(p), Qt::NoButton, held, m);
        mouseMoveEvent(&e);
    }
    void pressKey(int key) {
        QKeyEvent e(QEvent::KeyPress, key, Qt::NoModifier);
        keyPressEvent(&e);
    }
    void scroll(int dy) {
        QWheelEvent e(QPointF(0, 0), QPointF(0, 0), QPoint(0, 0), QPoint(0, dy),
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        wheelEvent(&e);
    }
};

// Build a minimal two-atom molecule (C-O single bond) with coordinates so the
// scene has valid bounds and the camera auto-frames on add_model.
OEChem::OEGraphMol make_diatomic() {
    OEChem::OEGraphMol mol;
    OEChem::OEAtomBase* a = mol.NewAtom(6);
    OEChem::OEAtomBase* b = mol.NewAtom(8);
    double pa[3] = {0, 0, 0};
    double pb[3] = {1.5, 0, 0};
    mol.SetCoords(a, pa);
    mol.SetCoords(b, pb);
    mol.NewBond(a, b, 1);
    return mol;
}

// qFuzzyCompare uses a RELATIVE epsilon (unreliable near zero). Sound for these
// tests: the "differ" cases produce large camera deltas (tens of degrees of
// orbit, a 70px pan, a 10% dolly), and the R-key "same" case is bit-identical
// because zoom_to() recomputes the matrix from scratch. A future test asserting
// a SMALL expected delta should not rely on this predicate.
bool matrices_differ(const QMatrix4x4& x, const QMatrix4x4& y) {
    return !qFuzzyCompare(x, y);
}

}  // namespace

TEST(MoleculeViewerWidgetTest, SetStyleOutOfRangeThrows) {
    TestableWidget w;  // no models added yet
    EXPECT_THROW(w.set_style(0, SphereStyle{}), RenderError);
    EXPECT_THROW(w.set_style(-1, SphereStyle{}), RenderError);
}

TEST(MoleculeViewerWidgetTest, AddModelFramesTheCamera) {
    TestableWidget w;
    OEChem::OEGraphMol mol = make_diatomic();
    const OEChem::OEMolBase& base = mol;
    const QMatrix4x4 before = w.view_matrix();  // default, unframed
    w.add_model(&base);
    // Framing moves the camera to fit the scene -> view matrix changes.
    EXPECT_TRUE(matrices_differ(before, w.view_matrix()));
}

TEST(MoleculeViewerWidgetTest, LeftDragOrbitsChangingViewMatrix) {
    TestableWidget w;
    w.resize(200, 200);
    OEChem::OEGraphMol mol = make_diatomic();
    const OEChem::OEMolBase& base = mol;
    w.add_model(&base);

    const QMatrix4x4 framed = w.view_matrix();
    w.press(QPoint(50, 50), Qt::LeftButton);
    w.drag(QPoint(120, 90), Qt::LeftButton);
    EXPECT_TRUE(matrices_differ(framed, w.view_matrix()))
        << "left-drag should orbit the camera";
}

TEST(MoleculeViewerWidgetTest, RightDragPansChangingViewMatrix) {
    TestableWidget w;
    w.resize(200, 200);
    OEChem::OEGraphMol mol = make_diatomic();
    const OEChem::OEMolBase& base = mol;
    w.add_model(&base);

    const QMatrix4x4 framed = w.view_matrix();
    w.press(QPoint(50, 50), Qt::RightButton);
    w.drag(QPoint(120, 50), Qt::RightButton);
    EXPECT_TRUE(matrices_differ(framed, w.view_matrix()))
        << "right-drag should pan the camera";
}

TEST(MoleculeViewerWidgetTest, WheelDolliesChangingDistance) {
    TestableWidget w;
    w.resize(200, 200);
    OEChem::OEGraphMol mol = make_diatomic();
    const OEChem::OEMolBase& base = mol;
    w.add_model(&base);

    const float before = w.camera_distance();
    w.scroll(120);  // wheel up = zoom in => distance shrinks
    EXPECT_LT(w.camera_distance(), before);
}

TEST(MoleculeViewerWidgetTest, RKeyReframesAfterOrbit) {
    TestableWidget w;
    w.resize(200, 200);
    OEChem::OEGraphMol mol = make_diatomic();
    const OEChem::OEMolBase& base = mol;
    w.add_model(&base);

    const QMatrix4x4 framed = w.view_matrix();
    w.press(QPoint(50, 50), Qt::LeftButton);
    w.drag(QPoint(150, 120), Qt::LeftButton);
    ASSERT_TRUE(matrices_differ(framed, w.view_matrix()));  // moved away
    w.pressKey(Qt::Key_R);
    EXPECT_FALSE(matrices_differ(framed, w.view_matrix()))
        << "'R' should restore the framed view";
}

TEST(MoleculeViewerWidgetTest, AddModelRejectsNullWithoutPoisoning) {
    TestableWidget w;
    EXPECT_THROW(w.add_model(nullptr), DataError);
    // Not poisoned: a valid model still works afterward.
    OEChem::OEGraphMol mol = make_diatomic();
    const OEChem::OEMolBase& base = mol;
    EXPECT_NO_THROW(w.add_model(&base));
}
TEST(MoleculeViewerWidgetTest, AddModelRejectsEmptyMolecule) {
    TestableWidget w;
    OEChem::OEGraphMol empty;
    const OEChem::OEMolBase& base = empty;
    EXPECT_THROW(w.add_model(&base), DataError);
}

}  // namespace test
}  // namespace oe3dmol
