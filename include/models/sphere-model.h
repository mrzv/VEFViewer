#ifndef VEFVIEWER_SPHERE_MODEL_H
#define VEFVIEWER_SPHERE_MODEL_H

#include <memory>

#include "model.h"
#include <zstr/zstr.hpp>

#include <nanogui/window.h>
namespace       ng = nanogui;

struct SphereModel: public Model
{
    typedef         std::tuple<unsigned,unsigned,unsigned>              Triangle;
    typedef         std::vector<ng::Vector4f>                           Spheres;
    typedef         std::vector<Triangle>                               Triangles;

                    SphereModel(const std::string&      name,
                                const Spheres&          spheres);

    virtual void            init_window(ng::Window* window) override;

    virtual void            draw(const ng::Matrix4f& mvp,
                                 const ng::Matrix4f& model,
                                 const ng::Matrix4f& view,
                                 const ng::Matrix4f& projection) const override;

    virtual const BBox&     bbox() const override           { return bbox_; }

    private:
                        SphereModel(const SphereModel&)     = delete;
        SphereModel&    operator=(const SphereModel&)       = delete;

    private:
        BBox                    bbox_;
        size_t                  n_;
        mutable ng::GLShader    shader_;
        Spheres                 spheres_;
        bool                    wireframe_ = false;
};

std::unique_ptr<Model>
load_sphere_model(const std::string& fn);

#endif

