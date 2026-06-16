#include "oe3dmol/cylinder_pipeline.h"

#include "oe3dmol/cylinder_layouts.h"
#include "oe3dmol/detail/checked_cast.h"
#include "oe3dmol/detail/shader_resources.h"
#include "oe3dmol/imposter_uniforms.h"
#include "oe3dmol/render_errors.h"

#include <QFile>

#include <rhi/qrhi.h>
#include <rhi/qshader.h>

namespace oe3dmol {

namespace {

QShader load_shader(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        throw RenderError("CylinderPipeline: cannot open shader " + path.toStdString());
    }
    QShader s = QShader::fromSerialized(f.readAll());
    if (!s.isValid()) {
        throw RenderError("CylinderPipeline: invalid shader " + path.toStdString());
    }
    return s;
}

}  // namespace

CylinderPipeline::CylinderPipeline(QRhi* rhi, QRhiRenderPassDescriptor* rp)
    : rhi_(rhi) {
    oe3dmol_ensure_shader_resources();

    ubuf_.reset(rhi_->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, IMPOSTER_UBO_SIZE));
    render_require(ubuf_ && ubuf_->create(),
                   "CylinderPipeline: failed to create uniform buffer");

    srb_.reset(rhi_->newShaderResourceBindings());
    render_require(srb_ != nullptr, "CylinderPipeline: failed to create SRB");
    srb_->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0,
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            ubuf_.get()),
    });
    render_require(srb_->create(), "CylinderPipeline: failed to create SRB");

    QShader vs = load_shader(QStringLiteral(":/shaders/cylinder.vert.qsb"));
    QShader fs = load_shader(QStringLiteral(":/shaders/cylinder.frag.qsb"));

    pipeline_.reset(rhi_->newGraphicsPipeline());
    render_require(pipeline_ != nullptr, "CylinderPipeline: failed to create graphics pipeline");
    pipeline_->setShaderStages({
        {QRhiShaderStage::Vertex, vs},
        {QRhiShaderStage::Fragment, fs},
    });

    const quint32 vstride =
        CylinderBillboardLayout::FLOATS_PER_VERTEX * static_cast<quint32>(sizeof(float));
    const quint32 istride =
        CylinderInstanceLayout::FLOATS_PER_INSTANCE * static_cast<quint32>(sizeof(float));

    QRhiVertexInputLayout layout;
    layout.setBindings({
        {vstride},
        {istride, QRhiVertexInputBinding::PerInstance, 1},
    });
    layout.setAttributes({
        // binding 0: billboard base vertex (endpoint_sel, corner_sign)
        {0, 0, QRhiVertexInputAttribute::Float2,
         CylinderBillboardLayout::ENDPOINT_SEL_OFFSET * static_cast<quint32>(sizeof(float))},
        // binding 1: per-instance start, end, radius, color
        {1, 1, QRhiVertexInputAttribute::Float3,
         CylinderInstanceLayout::START_OFFSET * static_cast<quint32>(sizeof(float))},
        {1, 2, QRhiVertexInputAttribute::Float3,
         CylinderInstanceLayout::END_OFFSET * static_cast<quint32>(sizeof(float))},
        {1, 3, QRhiVertexInputAttribute::Float,
         CylinderInstanceLayout::RADIUS_OFFSET * static_cast<quint32>(sizeof(float))},
        {1, 4, QRhiVertexInputAttribute::Float4,
         CylinderInstanceLayout::COLOR_OFFSET * static_cast<quint32>(sizeof(float))},
    });
    pipeline_->setVertexInputLayout(layout);
    pipeline_->setShaderResourceBindings(srb_.get());
    pipeline_->setRenderPassDescriptor(rp);
    pipeline_->setTopology(QRhiGraphicsPipeline::Triangles);
    pipeline_->setDepthTest(true);
    pipeline_->setDepthWrite(true);

    // Standard (non-premultiplied) source-over alpha blending so per-bond
    // opacity carried in the instance color alpha composites over the target.
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    pipeline_->setTargetBlends({blend});

    render_require(pipeline_->create(), "CylinderPipeline: failed to create graphics pipeline");
}

CylinderPipeline::~CylinderPipeline() = default;

void CylinderPipeline::upload(const MeshBuffer& mesh, const std::vector<float>& instances,
                              QRhiResourceUpdateBatch* batch) {
    const quint32 vbytes = detail::checked_u32(mesh.vertices.size() * sizeof(float),
                                               "cylinder vertex buffer bytes");
    const quint32 ibytes = detail::checked_u32(mesh.indices.size() * sizeof(quint32),
                                               "cylinder index buffer bytes");
    index_count_ = detail::checked_u32(mesh.indices.size(), "cylinder index count");
    instance_count_ = detail::checked_u32(
        instances.size() / CylinderInstanceLayout::FLOATS_PER_INSTANCE,
        "cylinder instance count");

    if (!vbuf_ || vbuf_->size() < vbytes) {
        vbuf_.reset(rhi_->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vbytes));
        render_require(vbuf_ && vbuf_->create(),
                       "CylinderPipeline: failed to create vertex buffer");
    }
    if (!ibuf_ || ibuf_->size() < ibytes) {
        ibuf_.reset(rhi_->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, ibytes));
        render_require(ibuf_ && ibuf_->create(),
                       "CylinderPipeline: failed to create index buffer");
    }
    batch->uploadStaticBuffer(vbuf_.get(), mesh.vertices.data());
    batch->uploadStaticBuffer(ibuf_.get(), mesh.indices.data());

    // The instance buffer is re-uploaded every render (bonds can move / restyle),
    // so it is Static (re-uploadable), unlike the upload-once billboard base geometry.
    // When called with zero instances, instbuf_ keeps stale data; that is safe
    // because draw() early-returns on instance_count_ == 0 and never binds it.
    if (instance_count_ > 0) {
        const quint32 instbytes = detail::checked_u32(instances.size() * sizeof(float),
                                                      "cylinder instance buffer bytes");
        if (!instbuf_ || instbuf_->size() < instbytes) {
            instbuf_.reset(
                rhi_->newBuffer(QRhiBuffer::Static, QRhiBuffer::VertexBuffer, instbytes));
            render_require(instbuf_ && instbuf_->create(),
                           "CylinderPipeline: failed to create instance buffer");
        }
        batch->uploadStaticBuffer(instbuf_.get(), instances.data());
    }
}

void CylinderPipeline::set_uniforms(const ImposterUniforms& u, QRhiResourceUpdateBatch* batch) {
    float data[IMPOSTER_UBO_SIZE / sizeof(float)] = {0};
    pack_imposter_uniforms(u, data);
    batch->updateDynamicBuffer(ubuf_.get(), 0, IMPOSTER_UBO_SIZE, data);
}

void CylinderPipeline::draw(QRhiCommandBuffer* cb, const QSize& viewport) {
    if (instance_count_ == 0 || index_count_ == 0) {
        return;
    }
    cb->setGraphicsPipeline(pipeline_.get());
    cb->setViewport(QRhiViewport(0.0f, 0.0f, static_cast<float>(viewport.width()),
                                 static_cast<float>(viewport.height())));
    cb->setShaderResources(srb_.get());
    const QRhiCommandBuffer::VertexInput inputs[] = {
        {vbuf_.get(), 0},
        {instbuf_.get(), 0},
    };
    cb->setVertexInput(0, 2, inputs, ibuf_.get(), 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(index_count_, instance_count_);
}

}  // namespace oe3dmol
