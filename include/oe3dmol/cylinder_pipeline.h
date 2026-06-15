#pragma once

/// \file cylinder_pipeline.h
/// \brief Instanced RHI pipeline drawing bond cylinders from a billboard base + instances.

#include <memory>
#include <vector>

#include <QSize>

#include "oe3dmol/mesh_buffer.h"
#include "oe3dmol/imposter_uniforms.h"

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

/// \brief Owns GPU buffers and the instanced graphics pipeline for bond cylinders.
///
/// Shares ImposterUniforms (the same std140 UBO layout as the sphere pipeline)
/// so lighting/fog/camera are identical across representations.
class CylinderPipeline {
public:
    /// \brief Build the pipeline against a device + render pass.
    /// \throws RenderError if shaders fail to load or pipeline creation fails.
    CylinderPipeline(QRhi* rhi, QRhiRenderPassDescriptor* rp);
    ~CylinderPipeline();

    CylinderPipeline(const CylinderPipeline&) = delete;
    CylinderPipeline& operator=(const CylinderPipeline&) = delete;

    /// \brief Upload the shared billboard base geometry and the per-instance buffer.
    void upload(const MeshBuffer& mesh, const std::vector<float>& instances,
                QRhiResourceUpdateBatch* batch);

    /// \brief Update the shared uniform buffer for the next draw.
    void set_uniforms(const ImposterUniforms& u, QRhiResourceUpdateBatch* batch);

    /// \brief Record an instanced draw (no-op if zero instances).
    void draw(QRhiCommandBuffer* cb, const QSize& viewport);

private:
    QRhi* rhi_ = nullptr;
    std::unique_ptr<QRhiBuffer> vbuf_;     // billboard base vertices
    std::unique_ptr<QRhiBuffer> ibuf_;     // billboard base indices
    std::unique_ptr<QRhiBuffer> instbuf_;  // per-instance data
    std::unique_ptr<QRhiBuffer> ubuf_;
    std::unique_ptr<QRhiShaderResourceBindings> srb_;
    std::unique_ptr<QRhiGraphicsPipeline> pipeline_;
    quint32 index_count_ = 0;
    quint32 instance_count_ = 0;
};

}  // namespace oe3dmol
