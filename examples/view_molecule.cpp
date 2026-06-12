/// \file view_molecule.cpp
/// \brief Interactive molecule viewer (orbit/zoom/pan) with a live representation
///        dropdown, plus a --grab PNG mode for headless capture.

#include <iostream>
#include <string>
#include <vector>

#include <QApplication>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include <oechem.h>

#include "oe3dmol/molecule_viewer_widget.h"
#include "oe3dmol/stick_style.h"
#include "oe3dmol/style_spec.h"

namespace {

// Representation choices, in dropdown order. Index 0 is the default.
enum Representation { kSphere = 0, kBallAndStick = 1, kSticks = 2 };

// Map a representation index to the StyleSpec list the widget should apply.
std::vector<oe3dmol::StyleSpec> styles_for(int index) {
    using namespace oe3dmol;
    switch (index) {
        case kBallAndStick:
            return {SphereStyle{}.scale(0.3).color(ColorScheme::Jmol),
                    StickStyle{}.color(ColorScheme::Jmol)};
        case kSticks:
            return {StickStyle{}.color(ColorScheme::Jmol)};
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

    OEChem::oemolistream ims;
    if (!ims.open(input.c_str())) {
        std::cerr << "cannot open " << input << "\n";
        return 1;
    }
    // The widget holds a non-owning pointer, so the molecule must outlive it.
    static OEChem::OEGraphMol mol;
    if (!OEChem::OEReadMolecule(ims, mol)) {
        std::cerr << "cannot read a molecule from " << input << "\n";
        return 1;
    }

    const OEChem::OEMolBase& base = mol;

    // --grab: headless one-frame capture of the initial representation, then exit
    // before any window/dropdown is built.
    if (!grab_path.empty()) {
        oe3dmol::MoleculeViewerWidget widget;
        const int model = widget.add_model(&base);
        widget.set_style(model, styles_for(initial_rep));
        widget.resize(800, 600);
        const QImage frame = widget.grabFramebuffer();
        if (frame.isNull()) {
            std::cerr << "grabFramebuffer returned a null image\n";
            return 1;
        }
        if (!frame.save(QString::fromStdString(grab_path), "PNG")) {
            std::cerr << "failed to save " << grab_path << "\n";
            return 1;
        }
        std::cout << "wrote " << grab_path << " (" << frame.width() << "x"
                  << frame.height() << ")\n";
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
    toolbar->addWidget(hint);
    toolbar->addStretch(1);

    // The layout takes ownership of the viewer once added below.
    auto* viewer = new oe3dmol::MoleculeViewerWidget;
    const int model = viewer->add_model(&base);
    viewer->set_style(model, styles_for(initial_rep));

    layout->addLayout(toolbar);
    layout->addWidget(viewer, 1);  // viewer fills the remaining space

    // Live switching: re-apply the selected representation on change.
    QObject::connect(combo, &QComboBox::currentIndexChanged,
                     [viewer, model](int index) {
                         viewer->set_style(model, styles_for(index));
                     });
    combo->setCurrentIndex(initial_rep);  // reflect the CLI selection in the control

    window.setWindowTitle(QStringLiteral("oe3dmol viewer - %1").arg(input.c_str()));
    window.resize(800, 640);
    window.show();
    return app.exec();
}
