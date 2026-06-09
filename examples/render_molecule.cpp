/// \file render_molecule.cpp
/// \brief Reads a molecule and renders a VDW sphere PNG via oe3dmol::Viewer.

#include <iostream>

#include <QGuiApplication>

#include <oechem.h>

#include "oe3dmol/viewer.h"

#include <string>
#include <vector>

#include "oe3dmol/stick_style.h"
#include "oe3dmol/style_spec.h"

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);

    if (argc < 3) {
        std::cerr << "usage: render_molecule <input.sdf|mol2|pdb> <out.png> [sphere|ballstick]\n";
        return 1;
    }

    OEChem::oemolistream ims;
    if (!ims.open(argv[1])) {
        std::cerr << "cannot open " << argv[1] << "\n";
        return 1;
    }

    OEChem::OEGraphMol mol;
    if (!OEChem::OEReadMolecule(ims, mol)) {
        std::cerr << "cannot read a molecule from " << argv[1] << "\n";
        return 1;
    }

    try {
        oe3dmol::Viewer viewer(800, 600);
        const OEChem::OEMolBase& base = mol;
        int idx = viewer.add_model(&base);
        const std::string mode = (argc > 3) ? argv[3] : "sphere";
        if (mode == "ballstick") {
            viewer.set_style(idx, std::vector<oe3dmol::StyleSpec>{
                oe3dmol::SphereStyle{}.scale(0.3).color(oe3dmol::ColorScheme::Jmol),
                oe3dmol::StickStyle{}.color(oe3dmol::ColorScheme::Jmol)});
        } else {
            viewer.set_style(idx, oe3dmol::SphereStyle{}.color(oe3dmol::ColorScheme::Jmol));
        }
        viewer.zoom_to();
        if (!viewer.render_to_png(argv[2])) {
            std::cerr << "render failed\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    std::cout << "wrote " << argv[2] << "\n";
    return 0;
}
