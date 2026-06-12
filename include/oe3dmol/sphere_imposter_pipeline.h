#pragma once

/// \file sphere_imposter_pipeline.h
/// \brief RHI pipeline that draws sphere imposters from a MeshBuffer.

#include <memory>

#include <QMatrix4x4>
#include <QSize>
#include <QVector3D>

#include "oe3dmol/mesh_buffer.h"

QT_BEGIN_NAMESPACE
class QRhi;
class QRhiBuffer;
class QRhiCommandBuffer;
class QRhiGraphicsPipeline;
class QRhiShaderResourceBindings;
class QRhiRenderPassDescriptor;
class QRhiResourceUpdateBatch;
QT_END_NAMESPACE

namespace oe3dmol {

/// \brief Per-frame uniforms passed to the imposter shaders (std140 layout).
struct ImposterUniforms {
    QMatrix4x4 model_view;
    QMatrix4x4 projection;
    QMatrix4x4 view;
    /// Maps the OpenGL-convention clip space (z in [-1,1], from
    /// QMatrix4x4::perspective) into the active backend's clip volume
    /// (z in [0,1] for Metal/D3D/Vulkan). Apply to gl_Position ONLY; the depth
    /// math keeps using the uncorrected projection. Defaults to identity so
    /// code that sets matrices by hand is unaffected until it opts in.
    QMatrix4x4 clip_correction;
    QVector3D light_color = QVector3D(1.0f, 1.0f, 1.0f);
    QVector3D light_direction = QVector3D(0.0f, 0.0f, 1.0f);
    QVector3D fog_color = QVector3D(1.0f, 1.0f, 1.0f);
    float fog_near = 1.0f;
    float fog_far = 1.0f;  // fog_near == fog_far disables fog
    float opacity = 1.0f;
};

/// \brief Owns GPU buffers and the graphics pipeline for sphere imposters.
class SphereImposterPipeline {
public:
    /// \brief Build the pipeline against a device + render pass.
    /// \throws RenderError if shaders fail to load or pipeline creation fails.
    SphereImposterPipeline(QRhi* rhi, QRhiRenderPassDescriptor* rp);
    ~SphereImposterPipeline();

    SphereImposterPipeline(const SphereImposterPipeline&) = delete;
    SphereImposterPipeline& operator=(const SphereImposterPipeline&) = delete;

    /// \brief Upload mesh data; call before draw when geometry changes.
    void upload(const MeshBuffer& mesh, QRhiResourceUpdateBatch* batch);

    /// \brief Update the uniform buffer for the next draw.
    void set_uniforms(const ImposterUniforms& u, QRhiResourceUpdateBatch* batch);

    /// \brief Record draw commands for the uploaded mesh.
    void draw(QRhiCommandBuffer* cb, const QSize& viewport);

private:
    QRhi* rhi_ = nullptr;
    std::unique_ptr<QRhiBuffer> vbuf_;
    std::unique_ptr<QRhiBuffer> ibuf_;
    std::unique_ptr<QRhiBuffer> ubuf_;
    std::unique_ptr<QRhiShaderResourceBindings> srb_;
    std::unique_ptr<QRhiGraphicsPipeline> pipeline_;
    quint32 index_count_ = 0;
};

}  // namespace oe3dmol
