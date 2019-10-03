#ifndef VEFVIEWER_EDGE_MODEL_H
#define VEFVIEWER_EDGE_MODEL_H

#include <memory>
#include "model.h"
#include <zstr/zstr.hpp>

extern unsigned char edge_vrt[];
extern unsigned int  edge_vrt_len;
extern unsigned char edge_frg[];
extern unsigned int  edge_frg_len;

namespace       ng = nanogui;

// TODO: currently doesn't use line_width_

struct EdgeModel: public Model
{
    typedef         std::tuple<unsigned,unsigned>                       Edge;
    typedef         std::vector<ng::Vector3f>                           Points;
    typedef         std::vector<Edge>                                   Edges;

                    EdgeModel(const std::string&      name,
                              const Points&           points,
                              const Edges&            edges,
                              float                   line_width  = 1.);

    virtual void            draw(const ng::Matrix4f& mvp,
                                 const ng::Matrix4f& model,
                                 const ng::Matrix4f& view,
                                 const ng::Matrix4f& projection) const;
    virtual const BBox&     bbox() const                    { return bbox_; }

    private:
                        EdgeModel(const EdgeModel&)   = delete;
        EdgeModel&  operator=(const EdgeModel&)       = delete;

    private:
        BBox                    bbox_;
        size_t                  n_;
        mutable ng::GLShader    shader_;
        float                   line_width_;
        static constexpr float  max_line_width_ = 5.;
};

std::unique_ptr<Model>
load_edge_model(const std::string& fn);

std::unique_ptr<Model>
load_stream_model(const std::string& fn);

#endif
