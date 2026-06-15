#include "oe3dmol/sphere_imposter_pipeline.h"

#include "oe3dmol/imposter_uniforms.h"
#include "oe3dmol/render_errors.h"

#include <QFile>
#include <QtResource>

#include <rhi/qrhi.h>
#include <rhi/qshader.h>

// Force registration of the embedded shader resources. The .qsb files live in a
// generated translation unit whose static initializer is dropped by the linker
// when oe3dmol_render is consumed as a static library, so reference it
// explicitly. Q_INIT_RESOURCE expands to a declaration of the global
// qInitResources_shaders symbol, so it must live at true global scope (outside
// any named or anonymous namespace) for the symbol to resolve.
static void ensure_shader_resources() {
    Q_INIT_RESOURCE(shaders);
}

namespace oe3dmol {

namespace {

QShader load_shader(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        throw RenderError("SphereImposterPipeline: cannot open shader " + path.toStdString());
    }
    QShader s = QShader::fromSerialized(f.readAll());
    if (!s.isValid()) {
        throw RenderError("SphereImposterPipeline: invalid shader " + path.toStdString());
    }
    return s;
}

}  // namespace

SphereImposterPipeline::SphereImposterPipeline(QRhi* rhi, QRhiRenderPassDescriptor* rp)
    : rhi_(rhi) {
    ensure_shader_resources();

    ubuf_.reset(rhi_->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, IMPOSTER_UBO_SIZE));
    render_require(ubuf_ && ubuf_->create(),
                   "SphereImposterPipeline: failed to create uniform buffer");

    srb_.reset(rhi_->newShaderResourceBindings());
    render_require(srb_ != nullptr, "SphereImposterPipeline: failed to create SRB");
    srb_->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0,
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            ubuf_.get()),
    });
    render_require(srb_->create(), "SphereImposterPipeline: failed to create SRB");

    QShader vs = load_shader(QStringLiteral(":/shaders/sphereimposter.vert.qsb"));
    QShader fs = load_shader(QStringLiteral(":/shaders/sphereimposter.frag.qsb"));

    pipeline_.reset(rhi_->newGraphicsPipeline());
    render_require(pipeline_ != nullptr,
                   "SphereImposterPipeline: failed to create graphics pipeline");
    pipeline_->setShaderStages({
        {QRhiShaderStage::Vertex, vs},
        {QRhiShaderStage::Fragment, fs},
    });

    QRhiVertexInputLayout layout;
    layout.setBindings({{VertexLayout::FLOATS_PER_VERTEX * static_cast<quint32>(sizeof(float))}});
    layout.setAttributes({
        {0, 0, QRhiVertexInputAttribute::Float3,
         VertexLayout::POSITION_OFFSET * static_cast<quint32>(sizeof(float))},
        {0, 1, QRhiVertexInputAttribute::Float3,
         VertexLayout::NORMAL_OFFSET * static_cast<quint32>(sizeof(float))},
        {0, 2, QRhiVertexInputAttribute::Float4,
         VertexLayout::COLOR_OFFSET * static_cast<quint32>(sizeof(float))},
    });
    pipeline_->setVertexInputLayout(layout);
    pipeline_->setShaderResourceBindings(srb_.get());
    pipeline_->setRenderPassDescriptor(rp);
    pipeline_->setTopology(QRhiGraphicsPipeline::Triangles);

    // Standard (non-premultiplied) source-over alpha blending so per-atom
    // opacity carried in the vertex color alpha composites over the target.
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    pipeline_->setTargetBlends({blend});

    pipeline_->setDepthTest(true);
    pipeline_->setDepthWrite(true);
    render_require(pipeline_->create(),
                   "SphereImposterPipeline: failed to create graphics pipeline");
}

SphereImposterPipeline::~SphereImposterPipeline() = default;

void SphereImposterPipeline::upload(const MeshBuffer& mesh, QRhiResourceUpdateBatch* batch) {
    const quint32 vbytes = static_cast<quint32>(mesh.vertices.size() * sizeof(float));
    const quint32 ibytes = static_cast<quint32>(mesh.indices.size() * sizeof(quint32));
    index_count_ = static_cast<quint32>(mesh.indices.size());

    if (!vbuf_ || vbuf_->size() < vbytes) {
        vbuf_.reset(rhi_->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vbytes));
        render_require(vbuf_ && vbuf_->create(),
                       "SphereImposterPipeline: failed to create vertex buffer");
    }
    if (!ibuf_ || ibuf_->size() < ibytes) {
        ibuf_.reset(rhi_->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, ibytes));
        render_require(ibuf_ && ibuf_->create(),
                       "SphereImposterPipeline: failed to create index buffer");
    }
    batch->uploadStaticBuffer(vbuf_.get(), mesh.vertices.data());
    batch->uploadStaticBuffer(ibuf_.get(), mesh.indices.data());
}

void SphereImposterPipeline::set_uniforms(const ImposterUniforms& u,
                                          QRhiResourceUpdateBatch* batch) {
    float data[IMPOSTER_UBO_SIZE / sizeof(float)] = {0};
    pack_imposter_uniforms(u, data);
    batch->updateDynamicBuffer(ubuf_.get(), 0, IMPOSTER_UBO_SIZE, data);
}

void SphereImposterPipeline::draw(QRhiCommandBuffer* cb, const QSize& viewport) {
    cb->setGraphicsPipeline(pipeline_.get());
    cb->setViewport(QRhiViewport(0.0f, 0.0f, static_cast<float>(viewport.width()),
                                 static_cast<float>(viewport.height())));
    cb->setShaderResources(srb_.get());
    const QRhiCommandBuffer::VertexInput vin(vbuf_.get(), 0);
    cb->setVertexInput(0, 1, &vin, ibuf_.get(), 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(index_count_);
}

}  // namespace oe3dmol
