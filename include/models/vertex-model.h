#ifndef VEFVIEWER_VERTEX_MODEL_H
#define VEFVIEWER_VERTEX_MODEL_H

#include "model.h"
#include <zstr/zstr.hpp>
#include <fmt/format.h>
#include <highfive/H5File.hpp>

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
                        Model(name, window, ng::Color(0.f,0.f,1.f,1.f)),
                        window_(window),
                        point_size_(point_size)
    {
        auto panel = new ng::Widget(window_);
        panel->setLayout(new ng::BoxLayout(ng::Orientation::Horizontal, ng::Alignment::Middle, 0, 20));

        auto slider = new ng::Slider(panel);
        slider->setValue(point_size/max_point_size_);
        slider->setTooltip("point size");

        auto textBox = new ng::TextBox(panel);
        textBox->setFixedSize(ng::Vector2i(60, 25));
        textBox->setValue(fmt::format("{:.2f}", point_size_));

        slider->setCallback([this,textBox](float ps) { point_size_ = max_point_size_*ps; textBox->setValue(fmt::format("{:.2f}", point_size_)); });

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
    zstr::ifstream  in(fn.c_str());
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

std::unique_ptr<Model>
load_vertex_hdf5_model(const std::string& fn, ng::Window* window)
{
    typedef     VertexModel::Points     Points;

    HighFive::File in(fn);

    HighFive::DataSet x = in.getDataSet("x");
    HighFive::DataSet y = in.getDataSet("y");

    std::vector<float> xs, ys, zs;
    x.read(xs);
    y.read(ys);

    if (in.exist("z"))
    {
        HighFive::DataSet z = in.getDataSet("z");
        z.read(zs);
    } else
        zs.resize(xs.size());

    if (xs.size() != ys.size() || xs.size() != zs.size())
        throw std::runtime_error(fmt::format("Dataset sizes don't match: {} {} {}\n", xs.size(), ys.size(), zs.size()));

    Points points;
    for (size_t i = 0; i < xs.size(); ++i)
        points.push_back({xs[i], ys[i], zs[i]});

    return std::unique_ptr<Model>(new VertexModel(fn, points, window));
}

#endif
