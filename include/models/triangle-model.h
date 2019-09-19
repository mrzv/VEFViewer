#ifndef VEFVIEWER_TRIANGLE_MODEL_H
#define VEFVIEWER_TRIANGLE_MODEL_H

#include <memory>

#include "model.h"
#include "../parse_stl.h"
#include <zstr/zstr.hpp>

namespace       ng = nanogui;

struct TriangleModel: public Model
{
    typedef         std::tuple<unsigned,unsigned,unsigned>              Triangle;
    typedef         std::vector<ng::Vector3f>                           Points;
    typedef         std::vector<Triangle>                               Triangles;

                    TriangleModel(const std::string&      name,
                                  const Points&           points,
                                  const Triangles&        triangles);

    virtual void            init_window(ng::Window* window) override;

    virtual void            draw(const ng::Matrix4f& mvp,
                                 const ng::Matrix4f& model,
                                 const ng::Matrix4f& view,
                                 const ng::Matrix4f& projection) const;

    virtual const BBox&     bbox() const                    { return bbox_; }

    private:
                        TriangleModel(const TriangleModel&)   = delete;
        TriangleModel&  operator=(const TriangleModel&)       = delete;

    private:
        BBox                    bbox_;
        size_t                  n_;
        mutable ng::GLShader    shader_;
        bool                    wireframe_ = false;
};

std::unique_ptr<Model>
load_triangle_model(const std::string& fn);

std::unique_ptr<Model>
load_object_model(const std::string& fn);

std::unique_ptr<Model>
load_stl_model(const std::string& fn);

std::unique_ptr<Model>
load_triangle_hdf5_model(const std::string& fn);

#endif
