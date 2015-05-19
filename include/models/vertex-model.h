#ifndef VEFVIEWER_VERTEX_MODEL_H
#define VEFVIEWER_VERTEX_MODEL_H

#include "model.h"

extern unsigned char vertex_vrt[];
extern unsigned int  vertex_vrt_len;
extern unsigned char vertex_frg[];
extern unsigned int  vertex_frg_len;

namespace       ng = nanogui;

struct VertexModel: public Model
{
    typedef         std::vector<ng::Vector3f>       Points;

                    VertexModel(const std::string&      name,
                                const Points&           points,
                                ng::Window*             window,
                                float                   point_size  = 2.):
                        Model(name, window, { 0,0,1,1}),
                        window_(window),
                        point_size_(point_size)
    {
        auto slider = new ng::Slider(window_);
        slider->setValue(point_size/max_point_size_);
        slider->setCallback([this](float ps) { point_size_ = max_point_size_*ps; });

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
            std::string(vertex_vrt, vertex_vrt + vertex_vrt_len),
            std::string(vertex_frg, vertex_frg + vertex_frg_len)
        );

        n_ = points.size();
        ng::MatrixXu indices(n_, 1);
        ng::MatrixXf positions(3, n_);
        int i = 0;
        for (auto& p : points)
        {
            positions.col(i) = p;
            indices(i,0) = i;
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
            shader_.setUniform("point_size", point_size_);
            shader_.setUniform("color", color());
            shader_.drawIndexed(GL_POINTS, 0, n_);
        }
    }
    virtual const BBox&     bbox() const                    { return bbox_; }

    private:
                        VertexModel(const VertexModel&)   = delete;
        VertexModel&    operator=(const VertexModel&)  = delete;

    private:
        BBox                    bbox_;
        size_t                  n_;
        mutable ng::GLShader    shader_;
        float                   point_size_;
        ng::Window*             window_;
        static constexpr float  max_point_size_ = 5.;
};

std::unique_ptr<Model>
load_vertex_model(const std::string& fn, ng::Window* window)
{
    typedef     VertexModel::Points     Points;

    Points          points;
    std::ifstream   in(fn.c_str());
    std::string line;

    while (std::getline(in, line))
    {
        if (line[0] == '#') continue;
        std::istringstream  iss(line);
        float x,y,z;
        iss >> x >> y >> z;
        points.push_back({x,y,z});
    }

    return std::unique_ptr<Model>(new VertexModel(fn, points, window));
}


#endif
