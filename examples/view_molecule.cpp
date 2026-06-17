/// \file view_molecule.cpp
/// \brief Interactive molecule viewer (orbit/zoom/pan) with a live representation
///        dropdown, plus a --grab PNG mode for headless capture.

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <oechem.h>

#include "oe3dmol/molecule_viewer_widget.h"
#include "oe3dmol/offscreen_session.h"
#include "oe3dmol/stick_style.h"
#include "oe3dmol/style_spec.h"
#include "oe3dmol/viewer.h"

namespace {

// Representation choices, in dropdown order. Index 0 is the default.
enum Representation { kSphere = 0, kBallAndStick = 1, kSticks = 2 };

// Map a representation index to the StyleSpec list the widget should apply.
std::vector<oe3dmol::StyleSpec> styles_for(int index, oe3dmol::AromaticStyle arom) {
    using namespace oe3dmol;
    const auto stick = [&] { return StickStyle{}.color(ColorScheme::Jmol).aromatic_style(arom); };
    switch (index) {
        case kBallAndStick:
            return {SphereStyle{}.scale(0.3).color(ColorScheme::Jmol), stick()};
        case kSticks:
            return {stick()};
        case kSphere:
        default:
            return {SphereStyle{}.color(ColorScheme::Jmol)};
    }
}

// Parse a positional representation name to its index (defaults to Sphere).
int representation_index(const std::string& name) {
    if (name == "ballstick") return kBallAndStick;
    if (name == "sticks") return kSticks;
    return kSphere;
}

// Report a startup error. In interactive mode (no --grab) show a modal dialog so
// a GUI launch with no terminal still tells the user what went wrong; always
// echo to stderr for command-line / --grab use. Returns 1 for use as exit code.
int report_startup_error(const QString& message, bool interactive) {
    std::cerr << message.toStdString() << "\n";
    if (interactive) {
        QMessageBox::critical(nullptr, QStringLiteral("oe3dmol viewer"), message);
    }
    return 1;
}

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    // Parse args: <file> [sphere|ballstick|sticks] [--grab <out.png>]
    std::string input;
    int initial_rep = kSphere;
    std::string grab_path;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--grab") {
            if (i + 1 >= argc) {
                std::cerr << "--grab requires an output PNG path\n";
                return 1;
            }
            grab_path = argv[++i];
        } else if (arg == "sphere" || arg == "ballstick" || arg == "sticks") {
            initial_rep = representation_index(arg);
        } else if (input.empty()) {
            input = arg;
        }
    }

    if (input.empty()) {
        std::cerr << "usage: view_molecule <input.sdf|mol2|pdb> "
                     "[sphere|ballstick|sticks] [--grab <out.png>]\n";
        return 1;
    }

    const bool interactive = grab_path.empty();

    if (!OEChem::OEChemIsLicensed()) {
        return report_startup_error(
            QStringLiteral("OpenEye OEChem license not found or expired. Set "
                           "OE_LICENSE to a valid license file."),
            interactive);
    }

    OEChem::oemolistream ims;
    if (!ims.open(input.c_str())) {
        return report_startup_error(
            QStringLiteral("Cannot open file: %1").arg(QString::fromStdString(input)),
            interactive);
    }
    // The widget holds non-owning pointers, so the molecules must outlive it.
    // This main-local vector lives for the whole interactive session (until
    // app.exec() returns), so the widget's pointers stay valid.
    std::vector<OEChem::OEGraphMol> mols;
    {
        OEChem::OEGraphMol m;
        while (OEChem::OEReadMolecule(ims, m)) {
            mols.push_back(m);
            m.Clear();
        }
    }
    if (mols.empty()) {
        return report_startup_error(
            QStringLiteral("No molecule could be read from: %1")
                .arg(QString::fromStdString(input)),
            interactive);
    }

    // --grab: headless one-frame capture via the offscreen Viewer (own RhiContext,
    // explicit camera framing) -- robust where a never-shown QRhiWidget's
    // grabFramebuffer() is platform-dependent.
    if (!grab_path.empty()) {
        try {
            oe3dmol::Viewer viewer(800, 600);
            for (const OEChem::OEGraphMol& m : mols) {
                const OEChem::OEMolBase& base = m;
                const int model = viewer.add_model(&base);
                viewer.set_style(model, styles_for(initial_rep, oe3dmol::AromaticStyle::Kekule));
            }
            viewer.zoom_to();
            if (!viewer.render_to_png(grab_path)) {
                std::cerr << "failed to render " << grab_path << "\n";
                return 1;
            }
        } catch (const std::exception& e) {
            std::cerr << "grab failed: " << e.what() << "\n";
            return 1;
        }
        std::cout << "wrote " << grab_path << "\n";
        return 0;
    }

    // Interactive: a representation dropdown above the viewer.
    QWidget window;
    auto* layout = new QVBoxLayout(&window);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(6, 4, 6, 4);
    auto* label = new QLabel(QStringLiteral("Representation:"));
    // Hint the re-frame shortcut so the fit/center action is discoverable.
    auto* hint = new QLabel(QStringLiteral("  (press R to re-fit)"));
    auto* combo = new QComboBox;
    combo->addItem(QStringLiteral("Sphere"));
    combo->addItem(QStringLiteral("Ball-and-stick"));
    combo->addItem(QStringLiteral("Sticks"));
    toolbar->addWidget(label);
    toolbar->addWidget(combo);
    auto* arom_label = new QLabel(QStringLiteral("  Aromatics:"));
    auto* arom_combo = new QComboBox;
    arom_combo->addItem(QStringLiteral("Kekule"));       // index 0 (default)
    arom_combo->addItem(QStringLiteral("Delocalized"));  // index 1
    toolbar->addWidget(arom_label);
    toolbar->addWidget(arom_combo);
    auto* animate_button = new QPushButton(QStringLiteral("Test Animate"));
    toolbar->addWidget(animate_button);
    toolbar->addWidget(hint);
    toolbar->addStretch(1);

    // The layout takes ownership of the viewer once added below.
    auto arom = std::make_shared<oe3dmol::AromaticStyle>(oe3dmol::AromaticStyle::Kekule);
    auto* viewer = new oe3dmol::MoleculeViewerWidget;
    std::vector<int> model_ids;
    for (const OEChem::OEGraphMol& m : mols) {
        const OEChem::OEMolBase& base = m;
        const int model = viewer->add_model(&base);
        viewer->set_style(model, styles_for(initial_rep, *arom));
        model_ids.push_back(model);
    }

    layout->addLayout(toolbar);
    layout->addWidget(viewer, 1);  // viewer fills the remaining space

    // Live switching: re-apply the selected representation on change.
    QObject::connect(combo, &QComboBox::currentIndexChanged,
                     [viewer, model_ids, arom](int index) {
                         for (int id : model_ids)
                             viewer->set_style(id, styles_for(index, *arom));
                     });
    QObject::connect(arom_combo, &QComboBox::currentIndexChanged,
                     [viewer, model_ids, arom, combo](int aidx) {
                         *arom = (aidx == 1) ? oe3dmol::AromaticStyle::Delocalized
                                             : oe3dmol::AromaticStyle::Kekule;
                         for (int id : model_ids)
                             viewer->set_style(id, styles_for(combo->currentIndex(), *arom));
                     });
    combo->setCurrentIndex(initial_rep);  // reflect the CLI selection in the control

    // "Test Animate": render a 180-degree turntable (36 frames, 5 deg each) of the
    // current view to a chosen directory, via an offscreen session that adopts the
    // widget's live camera so the animation starts from exactly what's on screen.
    QObject::connect(animate_button, &QPushButton::clicked,
                     [&window, viewer, &mols, combo, arom]() {
        const QString dir = QFileDialog::getExistingDirectory(
            &window, QStringLiteral("Choose a directory to save animation frames"));
        if (dir.isEmpty()) {
            return;  // user cancelled
        }
        try {
            oe3dmol::OffscreenSession session(viewer->width(), viewer->height());
            const auto styles = styles_for(combo->currentIndex(), *arom);
            for (const OEChem::OEGraphMol& m : mols) {
                const OEChem::OEMolBase& base = m;
                const int model = session.add_model(&base);
                session.set_style(model, styles);
            }
            // Adopt the live view AFTER add_model so it overrides the session's
            // one-time auto-frame; the turntable then spins from the current angle.
            session.camera() = viewer->camera();
            constexpr int kFrames = 36;  // 36 * 5 deg = 180 deg
            session.save_turntable(dir.toStdString(), kFrames,
                                   QVector3D(0.0f, 1.0f, 0.0f), 180.0f);
            QMessageBox::information(
                &window, QStringLiteral("Test Animate"),
                QStringLiteral("Wrote %1 frames to:\n%2").arg(kFrames).arg(dir));
        } catch (const std::exception& e) {
            QMessageBox::critical(&window, QStringLiteral("Test Animate"),
                                  QStringLiteral("Animation failed: %1")
                                      .arg(QString::fromUtf8(e.what())));
        }
    });

    window.setWindowTitle(QStringLiteral("oe3dmol viewer - %1").arg(input.c_str()));
    window.resize(800, 640);
    window.show();
    return app.exec();
}
