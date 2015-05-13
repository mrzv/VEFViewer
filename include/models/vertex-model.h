#ifndef VEFVIEWER_VERTEX_MODEL_H
#define VEFVIEWER_VERTEX_MODEL_H

#include "model.h"

namespace       ng = nanogui;

struct VertexModel: public Model
{
    typedef         std::vector<ng::Vector3f>       Points;

                    VertexModel(const std::string&      name,
                                const Points&           points,
                                ng::Window*             window,
                                float                   point_size  = 2.,
                                const ng::Vector4f&     color       = { 0., 0., 1., 1. }):
                        Model(name, window), window_(window), point_size_(point_size), color_(color)
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

            /* Vertex shader */
            "#version 330\n"
            "uniform mat4 modelViewProj;\n"
            "in vec3 position;\n"
            "uniform float point_size;\n"
            "void main() {\n"
            "    gl_Position = modelViewProj * vec4(position, 1.0);\n"
            "    gl_PointSize = point_size;\n"
            "}",

            /* Fragment shader */
            "#version 330\n"
            "out vec4 clr;\n"
            "uniform vec4 color;\n"
            "void main() {\n"
            "    clr = color;\n"
            "}"
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
        shader_.setUniform("color", color_);
    }

    virtual void            draw(const Eigen::Matrix4f& mvp) const
    {
        if (visible())
        {
            shader_.bind();
            shader_.setUniform("modelViewProj", mvp);
            shader_.setUniform("point_size", point_size_);
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
        ng::Vector4f            color_;
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
