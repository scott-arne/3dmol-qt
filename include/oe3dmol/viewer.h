#pragma once

/// \file viewer.h
/// \brief Fluent facade tying OpenEye molecules to offscreen multi-style rendering.

#include <memory>
#include <string>
#include <vector>

#include <QColor>
#include <QImage>
#include <QSize>

#include "oe3dmol/scene_builder.h"
#include "oe3dmol/style.h"

namespace OEChem {
class OEMolBase;
}

namespace oe3dmol {

/// \brief 3dmol.js-style viewer: add models, set styles, render to image.
class Viewer {
public:
    /// \brief Construct a viewer with an offscreen framebuffer size.
    /// \throws RenderError if width or height is not positive.
    explicit Viewer(int width, int height);
    ~Viewer();

    Viewer(const Viewer&) = delete;
    Viewer& operator=(const Viewer&) = delete;

    /// \brief Add a molecule (non-owning). \returns its model index.
    int add_model(const OEChem::OEMolBase* mol);

    /// \brief Apply a single sphere style to a model (whole-model selection).
    void set_style(int model_index, const SphereStyle& style);

    /// \brief Apply a list of styles to a model (e.g. sphere + stick for
    /// ball-and-stick). Replaces any previously set styles on that model.
    void set_style(int model_index, const std::vector<StyleSpec>& styles);

    /// \brief Set the background color (default white, matching 3dmol).
    void set_background_color(const QColor& color) { background_ = color; }

    /// \brief Fit the camera to all current models' bounds (3dmol parity hook).
    void zoom_to();

    /// \brief Render the scene to a QImage.
    /// \throws RenderError if there are no models or the device fails.
    QImage render();

    /// \brief Render the scene and write a PNG. \returns true on success.
    bool render_to_png(const std::string& path);

private:
    QSize size_;
    QColor background_ = QColor(255, 255, 255);
    std::vector<SceneModel> models_;
};

}  // namespace oe3dmol
