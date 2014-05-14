#ifndef JSONXMLCONVERTER_HPP_
#define JSONXMLCONVERTER_HPP_

#include "primitives/InfiniteSphere.hpp"
#include "primitives/Sphere.hpp"
#include "primitives/Quad.hpp"
#include "primitives/Mesh.hpp"

#include "materials/CheckerTexture.hpp"
#include "materials/BitmapTexture.hpp"

#include "cameras/ThinlensCamera.hpp"
#include "cameras/PinholeCamera.hpp"

#include "volume/HeterogeneousMedium.hpp"
#include "volume/HomogeneousMedium.hpp"
#include "volume/Medium.hpp"

#include "bsdfs/RoughDielectricBsdf.hpp"
#include "bsdfs/RoughConductorBsdf.hpp"
#include "bsdfs/DielectricBsdf.hpp"
#include "bsdfs/SmoothCoatBsdf.hpp"
#include "bsdfs/ConductorBsdf.hpp"
#include "bsdfs/OrenNayarBsdf.hpp"
#include "bsdfs/ThinSheetBsdf.hpp"
#include "bsdfs/LambertBsdf.hpp"
#include "bsdfs/ForwardBsdf.hpp"
#include "bsdfs/PlasticBsdf.hpp"
#include "bsdfs/MirrorBsdf.hpp"
#include "bsdfs/NullBsdf.hpp"
#include "bsdfs/PhongBsdf.hpp"
#include "bsdfs/MixedBsdf.hpp"
#include "bsdfs/Bsdf.hpp"

#include "io/Scene.hpp"

#include "Debug.hpp"

#include <iostream>
#include <string>
#include <stack>

namespace Tungsten
{

class SceneXmlWriter
{
    std::string _folder;
    std::ostream &_stream;
    std::string _indent;
    std::stack<std::string> _blocks;
    Scene &_scene;

    void begin(const std::string &block)
    {
        _stream << _indent << "<" << block << " ";
        _indent += '\t';
        _blocks.push(block);
    }

    void beginPost()
    {
        _stream << ">" << std::endl;
    }

    void endInline()
    {
        _stream << "/>" << std::endl;
        _indent.erase(_indent.end() - 1);
        _blocks.pop();
    }

    void end()
    {
        _indent.erase(_indent.end() - 1);
        _stream << _indent << "</" << _blocks.top() << ">" << std::endl;
        _blocks.pop();
    }

    template<typename T>
    void assign(const std::string &name, const T &value)
    {
        _stream << name << "=\"" << value << "\" ";
    }

    template<typename T>
    void convert(const std::string &blockname, const std::string &name, const T &v)
    {
        begin(blockname);
        assign("name", name);
        assign("value", v);
        endInline();
    }

    template<typename Type, unsigned Size>
    void convert(const std::string &blockname, const std::string &name, const Vec<Type, Size> &v)
    {
        begin(blockname);
        assign("name", name);
        _stream << "value=\"";
        for (int i = 0; i < Size; ++i)
            _stream << v[i] << (i == Size - 1 ? "" : ", ");
        _stream << "\"";
        endInline();
    }

    void convert(const std::string &name, const Mat4f &v)
    {
        begin("transform");
        assign("name", name);
        beginPost();
        begin("matrix");
        _stream << "value=\"";
        for (int i = 0; i < 15; ++i)
            _stream << v[i] << " ";
        _stream << v[15];
        _stream << "\"";
        endInline();
        end();
    }

    void convert(const std::string &name, bool v)
    {
        convert("boolean", name, v ? "true" : "false");
    }

    void convert(const std::string &name, int v)
    {
        convert("integer", name, v);
    }

    void convert(const std::string &name, float v)
    {
        convert("float", name, v);
    }

    void convert(const std::string &name, const char *v)
    {
        convert("string", name, v);
    }

    void convert(const std::string &name, const std::string &v)
    {
        convert("string", name, v);
    }

    template<typename Type, unsigned Size>
    void convertPoint(const std::string &name, const Vec<Type, Size> &v)
    {
        begin("point");
        assign("name", name);
        for (int i = 0; i < Size; ++i)
            assign(std::string(1, 'x' + i), v[i]);
        endInline();
    }

    template<typename Type, unsigned Size>
    void convertVector(const std::string &name, const Vec<Type, Size> &v)
    {
        begin("vector");
        assign("name", name);
        for (int i = 0; i < Size; ++i)
            assign(std::string(1, 'x' + i), v[i]);
        endInline();
    }

    void convertSpectrum(const std::string &name, float v)
    {
        convert(name, v);
    }

    void convertSpectrum(const std::string &name, const Vec3f &v)
    {
        convert("rgb", name, v);
    }

    template<bool Scalar, int Dimension>
    void convert(const std::string &name, ConstantTexture<Scalar, Dimension> *c)
    {
        convertSpectrum(name, c->average());
    }

    template<bool Scalar>
    void convert(const std::string &name, CheckerTexture<Scalar> *c)
    {
        begin("texture");
        if (!name.empty())
            assign("name", name);
        assign("type", "checkerboard");
        beginPost();
        convertSpectrum("color1", Vec3f(c->offColor()));
        convertSpectrum("color0", Vec3f(c->onColor()));
        convert("uoffset", 0.0f);
        convert("voffset", 0.0f);
        convert("uscale", float(c->resU())*0.5f);
        convert("vscale", float(c->resV())*0.5f);
        end();
    }

    template<bool Scalar>
    void convert(const std::string &name, BitmapTexture<Scalar, 2> *c)
    {
        begin("texture");
        if (!name.empty())
            assign("name", name);
        assign("type", "bitmap");
        beginPost();
        convert("filename", c->path());
        convert("filterType", "trilinear");
        end();
    }

    template<bool Scalar, int Dimension>
    void convert(const std::string &name, Texture<Scalar, Dimension> *a)
    {
        if (ConstantTexture<Scalar, Dimension> *a2 =
                dynamic_cast<ConstantTexture<Scalar, Dimension> *>(a))
            convert(name, a2);
        else if (BitmapTexture<Scalar, 2> *a2 =
                dynamic_cast<BitmapTexture<Scalar, 2> *>(a))
            convert(name, a2);
        else if (CheckerTexture<Scalar> *a2 =
                dynamic_cast<CheckerTexture<Scalar> *>(a))
            convert(name, a2);
        else
            FAIL("Unknown texture type!");
    }

    template<typename T>
    void convertOrRef(T *x);

    void convert(HomogeneousMedium *med)
    {
        begin("medium");
        assign("type", "homogeneous");
        if (!med->unnamed())
            assign("name", med->name());
        beginPost();
        convertSpectrum("sigmaS", med->sigmaS());
        convertSpectrum("sigmaA", med->sigmaA());
    }

    void convert(Medium *med)
    {
        if (HomogeneousMedium *med2 = dynamic_cast<HomogeneousMedium *>(med))
            convert(med2);
        else if (HeterogeneousMedium *med2 = dynamic_cast<HeterogeneousMedium *>(med))
            convert(med2);
        else
            FAIL("Unknown medium type!");

        switch(med->phaseFunctionType()) {
        case PhaseFunction::Isotropic:
            break;
        case PhaseFunction::HenyeyGreenstein:
            begin("phase");
            assign("type", "hg");
            beginPost();
            convert("g", med->phaseG());
            end();
            break;
        case PhaseFunction::Rayleigh:
            begin("phase");
            assign("type", "rayleigh");
            endInline();
        }

        end();
    }

    void convert(LambertBsdf *bsdf)
    {
        begin("bsdf");
        assign("type", "diffuse");
        if (!bsdf->unnamed())
            assign("id", bsdf->name());
        beginPost();
        convert("reflectance", bsdf->color().get());
        end();
    }

    void convert(OrenNayarBsdf *bsdf)
    {
        begin("bsdf");
        assign("type", "roughdiffuse");
        if (!bsdf->unnamed())
            assign("id", bsdf->name());
        beginPost();
        convert("reflectance", bsdf->color().get());
        convert("alpha", bsdf->roughness().get());
        end();
    }

    void convert(PhongBsdf *bsdf)
    {
        begin("bsdf");
        assign("type", "phong");
        if (!bsdf->unnamed())
            assign("id", bsdf->name());
        beginPost();
        convert("specularReflectance", bsdf->color().get());
        convert("exponent", float(bsdf->hardness()));
        convertSpectrum("diffuseReflectance", Vec3f(0.0f));
        end();
    }

    void convert(MixedBsdf *bsdf)
    {
        begin("bsdf");
        assign("type", "blendbsdf");
        if (!bsdf->unnamed())
            assign("id", bsdf->name());
        beginPost();
        convert("weight", bsdf->ratio().get());
        convertOrRef(bsdf->bsdf0().get());
        convertOrRef(bsdf->bsdf1().get());
        end();
    }

    void convert(DielectricBsdf *bsdf)
    {
        begin("bsdf");
        assign("type", "dielectric");
        if (!bsdf->unnamed())
            assign("id", bsdf->name());
        beginPost();
        convert("intIOR", bsdf->ior());
        convert("extIOR", 1.0f);
        end();
    }

    void convert(ThinSheetBsdf *bsdf)
    {
        begin("bsdf");
        assign("type", "thindielectric");
        if (!bsdf->unnamed())
            assign("id", bsdf->name());
        beginPost();
        convert("intIOR", bsdf->ior());
        convert("extIOR", 1.0f);
        end();
    }

    void convert(MirrorBsdf *bsdf)
    {
        begin("bsdf");
        assign("type", "conductor");
        if (!bsdf->unnamed())
            assign("id", bsdf->name());
        beginPost();
        convert("material", "none");
        end();
    }

    void convert(PlasticBsdf *bsdf)
    {
        begin("bsdf");
        assign("type", "plastic");
        if (!bsdf->unnamed())
            assign("id", bsdf->name());
        beginPost();
        convert("intIOR", bsdf->ior());
        convert("extIOR", 1.0f);
        convert("nonlinear", true);
        convert("diffuseReflectance", bsdf->color().get());
        end();
    }

    void convert(ConductorBsdf *bsdf)
    {
        begin("bsdf");
        assign("type", "conductor");
        if (!bsdf->unnamed())
            assign("id", bsdf->name());
        beginPost();
        convert("extEta", 1.0f);
        convert("specularReflectance", bsdf->color().get());
        convertSpectrum("eta", bsdf->eta());
        convertSpectrum("k", bsdf->k());
        end();
    }

    void convert(RoughConductorBsdf *bsdf)
    {
        begin("bsdf");
        assign("type", "roughconductor");
        if (!bsdf->unnamed())
            assign("id", bsdf->name());
        beginPost();
        convert("alpha", bsdf->roughness().get());
        convert("distribution", bsdf->distributionName());
        convert("extEta", 1.0f);
        convert("specularReflectance", bsdf->color().get());
        convertSpectrum("eta", bsdf->eta());
        convertSpectrum("k", bsdf->k());
        end();
    }

    void convert(RoughDielectricBsdf *bsdf)
    {
        begin("bsdf");
        assign("type", "roughdielectric");
        if (!bsdf->unnamed())
            assign("id", bsdf->name());
        beginPost();
        convert("alpha", bsdf->roughness().get());
        convert("distribution", bsdf->distributionName());
        convert("intIOR", bsdf->ior());
        convert("extIOR", 1.0f);
        end();
    }

    void convert(SmoothCoatBsdf *bsdf)
    {
        begin("bsdf");
        assign("type", "coating");
        if (!bsdf->unnamed())
            assign("id", bsdf->name());
        beginPost();
        convert("intIOR", bsdf->ior());
        convert("extIOR", 1.0f);
        convert("thickness", bsdf->thickness());
        convertSpectrum("sigmaA", bsdf->sigmaA());
        convertOrRef(bsdf->substrate().get());
        end();
    }

    void convert(NullBsdf *bsdf)
    {
        begin("bsdf");
        assign("type", "diffuse");
        if (!bsdf->unnamed())
            assign("id", bsdf->name());
        beginPost();
        convertSpectrum("reflectance", Vec3f(0.0f));
        end();
    }

    void convert(Bsdf *bsdf)
    {
        bool hasBump = !bsdf->bump()->isConstant();
        bool hasAlpha = !bsdf->alpha()->isConstant() || bsdf->alpha()->average() != 1.0f;
        std::string nameTmp = bsdf->name();
        if (hasBump) {
            begin("bsdf");
            assign("type", "bumpmap");
            if (!bsdf->unnamed())
                assign("id", bsdf->name());
            bsdf->setName("");
            beginPost();
            convert("map", bsdf->bump().get());
        }
        if (hasAlpha) {
            begin("bsdf");
            assign("type", "mask");
            if (!bsdf->unnamed())
                assign("id", bsdf->name());
            bsdf->setName("");
            beginPost();
            convert("opacity", bsdf->alpha().get());
        }
        if (LambertBsdf *bsdf2 = dynamic_cast<LambertBsdf *>(bsdf))
            convert(bsdf2);
        else if (PhongBsdf *bsdf2 = dynamic_cast<PhongBsdf *>(bsdf))
            convert(bsdf2);
        else if (MixedBsdf *bsdf2 = dynamic_cast<MixedBsdf *>(bsdf))
            convert(bsdf2);
        else if (DielectricBsdf *bsdf2 = dynamic_cast<DielectricBsdf *>(bsdf))
            convert(bsdf2);
        else if (MirrorBsdf *bsdf2 = dynamic_cast<MirrorBsdf *>(bsdf))
            convert(bsdf2);
        else if (ConductorBsdf *bsdf2 = dynamic_cast<ConductorBsdf *>(bsdf))
            convert(bsdf2);
        else if (RoughConductorBsdf *bsdf2 = dynamic_cast<RoughConductorBsdf *>(bsdf))
            convert(bsdf2);
        else if (RoughDielectricBsdf *bsdf2 = dynamic_cast<RoughDielectricBsdf *>(bsdf))
            convert(bsdf2);
        else if (SmoothCoatBsdf *bsdf2 = dynamic_cast<SmoothCoatBsdf *>(bsdf))
            convert(bsdf2);
        else if (NullBsdf *bsdf2 = dynamic_cast<NullBsdf *>(bsdf))
            convert(bsdf2);
        else if (ThinSheetBsdf *bsdf2 = dynamic_cast<ThinSheetBsdf *>(bsdf))
            convert(bsdf2);
        else if (OrenNayarBsdf *bsdf2 = dynamic_cast<OrenNayarBsdf *>(bsdf))
            convert(bsdf2);
        else if (PlasticBsdf *bsdf2 = dynamic_cast<PlasticBsdf *>(bsdf))
            convert(bsdf2);
        else if (dynamic_cast<ForwardBsdf *>(bsdf)) {
        } else
            FAIL("Unknown bsdf type with name '%s'!", bsdf->name());

        if (hasBump)
            end();
        if (hasAlpha)
            end();
        bsdf->setName(nameTmp);
    }

    void convert(PinholeCamera *cam)
    {
        begin("sensor");
        assign("type", "perspective");
        beginPost();
        convert("fov", cam->fovDeg());
    }

    void convert(ThinlensCamera *cam)
    {
        begin("sensor");
        assign("type", "thinlens");
        beginPost();
        convert("fov", cam->fovDeg());
        convert("focusDistance", cam->focusDist());
        convert("apertureRadius", cam->apertureSize());
    }

    void convert(Camera *cam)
    {
        if (PinholeCamera *cam2 = dynamic_cast<PinholeCamera *>(cam))
            convert(cam2);
        else if (ThinlensCamera *cam2 = dynamic_cast<ThinlensCamera *>(cam))
            convert(cam2);
        else
            FAIL("Unknown camera type!");

        convert("toWorld", cam->transform()*Mat4f::scale(Vec3f(-1.0f, 1.0f, 1.0f)));

        begin("sampler");
        if (_scene.rendererSettings().useSobol())
            assign("type", "sobol");
        else
            assign("type", "independent");
        beginPost();
        convert("sampleCount", int(cam->spp()));
        end();

        begin("film");
        assign("type", "ldrfilm");
        beginPost();
        convert("width", int(cam->resolution().x()));
        convert("height", int(cam->resolution().y()));
        convert("fileFormat", "png");
        convert("pixelFormat", "rgb");
        convert("gamma", 2.2f);
        convert("banner", false);

        begin("rfilter");
        assign("type", "box");
        endInline();

        end();

        end();
    }

    void convert(TriangleMesh *prim)
    {
        begin("shape");
        assign("type", "obj");
        beginPost();
        std::string objFile(FileUtils::stripExt(prim->path()) + ".obj");
        std::string fullObjFile(_folder + "/" + objFile);
        if (!FileUtils::createDirectory(FileUtils::extractDir(fullObjFile)))
            FAIL("Unable to create target folder for obj at: '%s'", fullObjFile);
        std::ofstream out(fullObjFile, std::ios_base::out | std::ios_base::binary);
        if (!out.good())
            FAIL("Unable to create obj for writing at: '%s'", fullObjFile);
        prim->saveAsObj(out);
        out.close();
        convert("filename", objFile);
        convert("toWorld", prim->transform());
    }

    void convert(Sphere *prim)
    {
        begin("shape");
        assign("type", "sphere");
        beginPost();
        convert("radius", prim->radius());
        convertPoint("center", prim->pos());
    }

    void convert(Quad *prim)
    {
        begin("shape");
        assign("type", "rectangle");
        beginPost();
        convert("toWorld", prim->transform()*
                Mat4f::rotXYZ(Vec3f(90.0f, 0.0f, 0.0f))*
                Mat4f::scale(Vec3f(0.5f)));
    }

    void convert(InfiniteSphere *prim)
    {
        if (prim->emission()->isConstant()) {
            begin("emitter");
            assign("type", "constant");
            beginPost();
            convertSpectrum("radiance", prim->emission()->average());
            end();
        } else if (BitmapTextureRgb *tex = dynamic_cast<BitmapTextureRgb *>(prim->emission().get())) {
            begin("emitter");
            assign("type", "envmap");
            beginPost();
            convert("toWorld", prim->transform()*Mat4f::rotXYZ(Vec3f(0.0f, 90.0f, 0.0f)));
            convert("filename", FileUtils::stripExt(tex->path()) + ".hdr");
            end();
        } else {
            FAIL("Infinite sphere has to be a constant or bitmap textured light source!");
        }
    }

    void convert(Primitive *prim)
    {
        prim->prepareForRender();

        if (TriangleMesh *prim2 = dynamic_cast<TriangleMesh *>(prim))
            convert(prim2);
        else if (Sphere *prim2 = dynamic_cast<Sphere *>(prim))
            convert(prim2);
        else if (Quad *prim2 = dynamic_cast<Quad *>(prim))
            convert(prim2);
        else if (InfiniteSphere *prim2 = dynamic_cast<InfiniteSphere *>(prim)) {
            convert(prim2);
            return;
        } else
            FAIL("Unknown primitive type!");

        if (!dynamic_cast<ForwardBsdf *>(prim->bsdf().get()))
            convertOrRef(prim->bsdf().get());
        if (prim->bsdf()->intMedium()) {
            prim->bsdf()->intMedium()->setName("interior");
            convert(prim->bsdf()->intMedium().get());
        }
        if (prim->bsdf()->extMedium()) {
            prim->bsdf()->extMedium()->setName("exterior");
            convert(prim->bsdf()->extMedium().get());
        }
        if (prim->isEmissive()) {
            begin("emitter");
            assign("type", "area");
            beginPost();
            convert("radiance", prim->emission().get());//*prim->area());
            end();
        }
        end();
    }

    void convert(Scene *scene)
    {
        begin("scene");
        assign("version", "0.5.0");
        beginPost();

        begin("integrator");
        if (scene->media().empty())
            assign("type", "path");
        else
            assign("type", "volpath");
        beginPost();
        convert("strictNormals", true);
        end();

        convert(scene->camera().get());

        for (const std::shared_ptr<Bsdf> &bsdf : scene->bsdfs())
            if (!bsdf->unnamed())
                convert(bsdf.get());
        for (const std::shared_ptr<Primitive> &prim : scene->primitives())
            convert(prim.get());

        end();
    }

public:
    SceneXmlWriter(const std::string &folder, Scene &scene, std::ostream &stream)
    : _folder(FileUtils::stripSlash(folder)),
      _stream(stream),
      _scene(scene)
    {
        _stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\n";

        convert (&scene);
    }
};

template<typename T>
void SceneXmlWriter::convertOrRef(T *x)
{
    if (x->unnamed())
        convert(x);
    else {
        begin("ref");
        assign("id", x->name());
        endInline();
    }
}

}



#endif /* JSONXMLCONVERTER_HPP_ */
