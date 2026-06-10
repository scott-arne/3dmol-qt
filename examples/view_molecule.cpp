/// \file view_molecule.cpp
/// \brief Interactive molecule viewer (orbit/zoom/pan), with a --grab PNG mode.

#include <iostream>
#include <string>
#include <vector>

#include <QApplication>

#include <oechem.h>

#include "oe3dmol/molecule_viewer_widget.h"
#include "oe3dmol/stick_style.h"
#include "oe3dmol/style_spec.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    // Parse args: <file> [sphere|ballstick] [--grab <out.png>]
    std::string input;
    std::string mode = "sphere";
    std::string grab_path;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--grab") {
            if (i + 1 >= argc) {
                std::cerr << "--grab requires an output PNG path\n";
                return 1;
            }
            grab_path = argv[++i];
        } else if (arg == "sphere" || arg == "ballstick") {
            mode = arg;
        } else if (input.empty()) {
            input = arg;
        }
    }

    if (input.empty()) {
        std::cerr << "usage: view_molecule <input.sdf|mol2|pdb> [sphere|ballstick] "
                     "[--grab <out.png>]\n";
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

    oe3dmol::MoleculeViewerWidget widget;
    const OEChem::OEMolBase& base = mol;
    int idx = widget.add_model(&base);
    if (mode == "ballstick") {
        widget.set_style(idx, std::vector<oe3dmol::StyleSpec>{
            oe3dmol::SphereStyle{}.scale(0.3).color(oe3dmol::ColorScheme::Jmol),
            oe3dmol::StickStyle{}.color(oe3dmol::ColorScheme::Jmol)});
    } else {
        widget.set_style(idx, oe3dmol::SphereStyle{}.color(oe3dmol::ColorScheme::Jmol));
    }
    widget.resize(800, 600);

    if (!grab_path.empty()) {
        // Headless one-frame capture of the ACTUAL widget render target.
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

    widget.setWindowTitle(QString("oe3dmol viewer - %1").arg(input.c_str()));
    widget.show();
    return app.exec();
}
