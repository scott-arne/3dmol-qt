/// \file view_molecule.cpp
/// \brief Opens an interactive window viewing a molecule (orbit/zoom/pan).

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

    if (argc < 2) {
        std::cerr << "usage: view_molecule <input.sdf|mol2|pdb> [sphere|ballstick]\n";
        return 1;
    }

    OEChem::oemolistream ims;
    if (!ims.open(argv[1])) {
        std::cerr << "cannot open " << argv[1] << "\n";
        return 1;
    }
    // The widget holds a non-owning pointer, so the molecule must outlive it.
    static OEChem::OEGraphMol mol;
    if (!OEChem::OEReadMolecule(ims, mol)) {
        std::cerr << "cannot read a molecule from " << argv[1] << "\n";
        return 1;
    }

    oe3dmol::MoleculeViewerWidget widget;
    const OEChem::OEMolBase& base = mol;
    int idx = widget.add_model(&base);
    const std::string mode = (argc > 2) ? argv[2] : "sphere";
    if (mode == "ballstick") {
        widget.set_style(idx, std::vector<oe3dmol::StyleSpec>{
            oe3dmol::SphereStyle{}.scale(0.3).color(oe3dmol::ColorScheme::Jmol),
            oe3dmol::StickStyle{}.color(oe3dmol::ColorScheme::Jmol)});
    } else {
        widget.set_style(idx, oe3dmol::SphereStyle{}.color(oe3dmol::ColorScheme::Jmol));
    }
    widget.resize(800, 600);
    widget.setWindowTitle(QString("oe3dmol viewer - %1").arg(argv[1]));
    widget.show();

    return app.exec();
}
