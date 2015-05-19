#ifndef VEFVIEWER_EDGE_MODEL_H
#define VEFVIEWER_EDGE_MODEL_H

#include "model.h"
#include <format.h>

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
                              ng::Window*             window,
                              float                   line_width  = 1.):
                        Model(name, window, { 0., 1., 1., 1. }), window_(window), line_width_(line_width)
    {
        //auto slider = new ng::Slider(window_);
        //slider->setValue(line_width_/max_line_width_);
        //slider->setCallback([this](float ls) { line_width_ = max_line_width_*ls; });


        ng::Vector3f min = points[0], max = points[0];
        for (auto& p : points)
        {
            for (unsigned i = 0; i < 3; ++i)
            {
                if (p[i] > max[i])
                    max[i] = p[i];
                if (p[i] < min[i])
                    min[i] = p[i];
            }
        }

        bbox_ = BBox { min, max };

        shader_.init(
            name,       // an identifying name
            std::string(edge_vrt, edge_vrt + edge_vrt_len),
            std::string(edge_frg, edge_frg + edge_frg_len)
        );

        n_ = edges.size();
        ng::MatrixXu indices(2, edges.size());
        ng::MatrixXf positions(3, points.size());
        int i = 0;
        for (auto& p : points)
            positions.col(i++) = p;

        i = 0;
        for (auto& edge : edges)
        {
            indices(0,i) = std::get<0>(edge);
            indices(1,i) = std::get<1>(edge);
            ++i;
        }

        shader_.bind();
        shader_.uploadIndices(indices);
        shader_.uploadAttrib("position", positions);
    }

    virtual void            draw(const ng::Matrix4f& mvp,
                                 const ng::Matrix4f& model,
                                 const ng::Matrix4f& view,
                                 const ng::Matrix4f& projection) const
    {
        if (visible())
        {
            shader_.bind();
            shader_.setUniform("modelViewProj", mvp);
            shader_.setUniform("color", color());
            shader_.drawIndexed(GL_LINES, 0, n_);
        }
    }
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
        ng::Window*             window_;
};

std::unique_ptr<Model>
load_edge_model(const std::string& fn, ng::Window* window)
{
    typedef     EdgeModel::Points      Points;
    typedef     EdgeModel::Edges       Edges;
    typedef     EdgeModel::Edge        Edge;

    Points          points;
    Edges           edges;

    std::ifstream   in(fn.c_str());
    std::string     line;

    size_t i = 0;
    while (std::getline(in, line))
    {
        if (line[0] == '#') continue;
        std::istringstream  iss(line);
        float x,y,z;
        iss >> x >> y >> z;
        points.push_back({x,y,z});

        if (i % 2 == 1)
            edges.push_back(Edge { i - 1, i });

        ++i;
    }

    return std::unique_ptr<Model>(new EdgeModel(fn, points, edges, window));
}


#endif
