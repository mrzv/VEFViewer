#include <models/edge-model.h>
#include <zstr/zstr.hpp>

extern unsigned char edge_vrt[];
extern unsigned int  edge_vrt_len;
extern unsigned char edge_frg[];
extern unsigned int  edge_frg_len;

// TODO: currently doesn't use line_width_

EdgeModel::
EdgeModel(const std::string&      name,
          const Points&           points,
          const Edges&            edges,
          float                   line_width):
    Model(name, ng::Color(0.f, 1.f, 1.f, 1.f)), line_width_(line_width)
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

void
EdgeModel::
draw(const ng::Matrix4f& mvp,
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

std::unique_ptr<Model>
load_edge_model(const std::string& fn)
{
    typedef     EdgeModel::Points      Points;
    typedef     EdgeModel::Edges       Edges;
    typedef     EdgeModel::Edge        Edge;

    Points          points;
    Edges           edges;

    zstr::ifstream  in(fn.c_str());
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

    return std::unique_ptr<Model>(new EdgeModel(fn, points, edges));
}
