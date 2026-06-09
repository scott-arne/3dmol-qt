#include "oe3dmol/cylinder_pipeline.h"

#include "oe3dmol/cylinder_layouts.h"
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
static void ensure_cylinder_shader_resources() {
    Q_INIT_RESOURCE(shaders);
}

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

// std140 size: 3 mat4 (192) + 3 vec4 (48) + 4 floats (16) = 256 bytes.
// Matches the shared ImposterUniforms layout consumed by the cylinder shaders.
constexpr quint32 UBO_SIZE = 256;

void pack_uniforms(const ImposterUniforms& u, float* out) {
    int o = 0;
    auto put_mat = [&](const QMatrix4x4& m) {
        const float* d = m.constData();  // column-major, matches std140 mat4
        for (int i = 0; i < 16; ++i) out[o++] = d[i];
    };
    put_mat(u.model_view);
    put_mat(u.projection);
    put_mat(u.view);
    auto put_vec4 = [&](const QVector3D& v) {
        out[o++] = v.x();
        out[o++] = v.y();
        out[o++] = v.z();
        out[o++] = 0.0f;
    };
    put_vec4(u.light_color);
    put_vec4(u.light_direction);
    put_vec4(u.fog_color);
    out[o++] = u.fog_near;
    out[o++] = u.fog_far;
    out[o++] = u.opacity;
    out[o++] = 0.0f;
}

}  // namespace

CylinderPipeline::CylinderPipeline(QRhi* rhi, QRhiRenderPassDescriptor* rp)
    : rhi_(rhi) {
    ensure_cylinder_shader_resources();

    ubuf_.reset(rhi_->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBO_SIZE));
    if (!ubuf_->create()) {
        throw RenderError("CylinderPipeline: failed to create uniform buffer");
    }

    srb_.reset(rhi_->newShaderResourceBindings());
    srb_->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0,
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            ubuf_.get()),
    });
    if (!srb_->create()) {
        throw RenderError("CylinderPipeline: failed to create SRB");
    }

    QShader vs = load_shader(QStringLiteral(":/shaders/cylinder.vert.qsb"));
    QShader fs = load_shader(QStringLiteral(":/shaders/cylinder.frag.qsb"));

    pipeline_.reset(rhi_->newGraphicsPipeline());
    pipeline_->setShaderStages({
        {QRhiShaderStage::Vertex, vs},
        {QRhiShaderStage::Fragment, fs},
    });

    const quint32 vstride =
        CylinderVertexLayout::FLOATS_PER_VERTEX * static_cast<quint32>(sizeof(float));
    const quint32 istride =
        CylinderInstanceLayout::FLOATS_PER_INSTANCE * static_cast<quint32>(sizeof(float));

    QRhiVertexInputLayout layout;
    layout.setBindings({
        {vstride},
        {istride, QRhiVertexInputBinding::PerInstance, 1},
    });
    layout.setAttributes({
        {0, 0, QRhiVertexInputAttribute::Float3,
         CylinderVertexLayout::POSITION_OFFSET * static_cast<quint32>(sizeof(float))},
        {0, 1, QRhiVertexInputAttribute::Float3,
         CylinderVertexLayout::NORMAL_OFFSET * static_cast<quint32>(sizeof(float))},
        {1, 2, QRhiVertexInputAttribute::Float3,
         CylinderInstanceLayout::START_OFFSET * static_cast<quint32>(sizeof(float))},
        {1, 3, QRhiVertexInputAttribute::Float3,
         CylinderInstanceLayout::END_OFFSET * static_cast<quint32>(sizeof(float))},
        {1, 4, QRhiVertexInputAttribute::Float,
         CylinderInstanceLayout::RADIUS_OFFSET * static_cast<quint32>(sizeof(float))},
        {1, 5, QRhiVertexInputAttribute::Float4,
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

    if (!pipeline_->create()) {
        throw RenderError("CylinderPipeline: failed to create graphics pipeline");
    }
}

CylinderPipeline::~CylinderPipeline() = default;

void CylinderPipeline::upload(const MeshBuffer& mesh, const std::vector<float>& instances,
                              QRhiResourceUpdateBatch* batch) {
    const quint32 vbytes = static_cast<quint32>(mesh.vertices.size() * sizeof(float));
    const quint32 ibytes = static_cast<quint32>(mesh.indices.size() * sizeof(quint32));
    index_count_ = static_cast<quint32>(mesh.indices.size());
    instance_count_ =
        static_cast<quint32>(instances.size() / CylinderInstanceLayout::FLOATS_PER_INSTANCE);

    if (!vbuf_ || vbuf_->size() < vbytes) {
        vbuf_.reset(rhi_->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vbytes));
        vbuf_->create();
    }
    if (!ibuf_ || ibuf_->size() < ibytes) {
        ibuf_.reset(rhi_->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, ibytes));
        ibuf_->create();
    }
    batch->uploadStaticBuffer(vbuf_.get(), mesh.vertices.data());
    batch->uploadStaticBuffer(ibuf_.get(), mesh.indices.data());

    if (instance_count_ > 0) {
        const quint32 instbytes = static_cast<quint32>(instances.size() * sizeof(float));
        if (!instbuf_ || instbuf_->size() < instbytes) {
            instbuf_.reset(
                rhi_->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, instbytes));
            instbuf_->create();
        }
        batch->uploadStaticBuffer(instbuf_.get(), instances.data());
    }
}

void CylinderPipeline::set_uniforms(const ImposterUniforms& u, QRhiResourceUpdateBatch* batch) {
    float data[UBO_SIZE / sizeof(float)] = {0};
    pack_uniforms(u, data);
    batch->updateDynamicBuffer(ubuf_.get(), 0, UBO_SIZE, data);
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
