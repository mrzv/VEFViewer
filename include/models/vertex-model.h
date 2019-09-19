#ifndef VEFVIEWER_VERTEX_MODEL_H
#define VEFVIEWER_VERTEX_MODEL_H

#include "model.h"
#include <zstr/zstr.hpp>
#include <fmt/format.h>
#include <highfive/H5File.hpp>

namespace       ng = nanogui;

struct VertexModel: public Model
{
    typedef         std::vector<ng::Vector3f>       Points;

                    VertexModel(const std::string&      name,
                                const Points&           points,
                                float                   point_size  = 2.);

    virtual void            init_window(ng::Window* window) override;

    virtual void            draw(const ng::Matrix4f& mvp,
                                 const ng::Matrix4f& model,
                                 const ng::Matrix4f& view,
                                 const ng::Matrix4f& projection) const;
    virtual const BBox&     bbox() const override           { return bbox_; }

    private:
                        VertexModel(const VertexModel&)   = delete;
        VertexModel&    operator=(const VertexModel&)  = delete;

    private:
        BBox                    bbox_;
        size_t                  n_;
        mutable ng::GLShader    shader_;
        float                   point_size_;
        static constexpr float  max_point_size_ = 5.;
};

std::unique_ptr<Model>
load_vertex_model(const std::string& fn);

std::unique_ptr<Model>
load_vertex_hdf5_model(const std::string& fn);

#endif
