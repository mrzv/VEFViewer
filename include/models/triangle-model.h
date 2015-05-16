#ifndef VEFVIEWER_TRIANGLE_MODEL_H
#define VEFVIEWER_TRIANGLE_MODEL_H

#include "model.h"
#include <format.h>

namespace       ng = nanogui;

struct TriangleModel: public Model
{
    typedef         std::tuple<unsigned,unsigned,unsigned>              Triangle;
    typedef         std::vector<ng::Vector3f>                           Points;
    typedef         std::vector<Triangle>                               Triangles;

                    TriangleModel(const std::string&      name,
                                  const Points&           points,
                                  const Triangles&        triangles,
                                  ng::Window*             window):
                        Model(name, window, { 1., 1., 0., 1. }), window_(window)
    {
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

        auto b = new ng::Button(tools_, "", ENTYPO_ICON_SHARE);
        b->setButtonFlags(nanogui::Button::ToggleButton);
        b->setPushed(wireframe_);
        b->setChangeCallback([this](bool state) { wireframe_ = state; });

        bbox_ = BBox { min, max };

        //shader_.initFromFiles(name, "vertex.glsl", "fragment.glsl");
        shader_.init(
            name,       // an identifying name

            /* Vertex shader */
            "#version 330\n"
            "uniform mat4 modelViewProj;\n"
            "in vec3 position;\n"
            "in vec3 normal;\n"
            "smooth out vec3 n;\n"
            "smooth out vec4 position_;\n"
            "void main()\n"
            "{\n"
            "    gl_Position = modelViewProj * vec4(position, 1.0);\n"
            "    position_   = gl_Position;\n"
            "    n = normalize(normal);\n"
            "}\n",

            /* Fragment shader */
            "#version 330\n"
            "out vec4 clr;\n"
            "uniform vec4 color;\n"
            "uniform float wireframe;\n"
            "smooth in vec3 n;\n"
            "smooth in vec4 position_;\n"
            "void main()\n"
            "{\n"
            "    // flat shading\n"
            "    vec3 ec_normal = normalize(cross(dFdx(vec3(position_)), dFdy(vec3(position_))));\n"
            "    clr.rgb = abs(vec3(color) * ec_normal[2]);\n"
            "    float depth = ((position_.z / position_.w) + 1.0) * 0.5;\n"
            "    clr.rgb = max(clr.rgb, vec3(color) * wireframe * (1 - depth));\n"
            ""
            "    // normal shading\n"
            "    //clr.rgb = abs(vec3(color) * (n[0] * .33 + n[1] * .33 + n[2] * .33));\n"
            "    //clr.rgb = abs(vec3(color) * n[2]);\n"
            ""
            "    // depth buffer shading\n"
            "    //float depth = ((position_.z / position_.w) + 1.0) * 0.5;\n"
            "    //clr.rgb = vec3(depth, depth, depth);\n"
            ""
            "    clr.a   = color.a;\n"
            "}\n"
        );

        n_ = triangles.size();
        ng::MatrixXu indices(3, triangles.size());
        ng::MatrixXf normals(3, points.size());
        ng::MatrixXf positions(3, points.size());
        int i = 0;
        for (auto& p : points)
        {
            positions.col(i) = p;
            ++i;
        }

        typedef     Eigen::Matrix<unsigned, 3, 1>      Vector3u;
        i = 0;
        for (auto& tri : triangles)
        {
            unsigned v0,v1,v2;
            std::tie(v0,v1,v2) = tri;
            indices.col(i) = Vector3u { v0, v1, v2 };
            auto n = Eigen::Hyperplane<float,3>::Through(positions.col(v0), positions.col(v1), positions.col(v2)).normal();
            normals.col(v0) += n;
            normals.col(v1) += n;
            normals.col(v2) += n;
            ++i;
        }

        shader_.bind();
        shader_.uploadIndices(indices);
        shader_.uploadAttrib("position", positions);
        shader_.uploadAttrib("normal",   normals);
    }

    virtual void            draw(const ng::Matrix4f& mvp,
                                 const ng::Matrix4f& model,
                                 const ng::Matrix4f& view,
                                 const ng::Matrix4f& projection) const
    {
        if (visible())
        {
            shader_.bind();
            if (wireframe_)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                shader_.setUniform("wireframe", 1.f);
            } else
                shader_.setUniform("wireframe", 0.f);

            shader_.setUniform("modelViewProj", mvp);
            shader_.setUniform("color", color());

            shader_.drawIndexed(GL_TRIANGLES, 0, n_);

            if (wireframe_)
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }
    virtual const BBox&     bbox() const                    { return bbox_; }

    private:
                        TriangleModel(const TriangleModel&)   = delete;
        TriangleModel&  operator=(const TriangleModel&)       = delete;

    private:
        BBox                    bbox_;
        size_t                  n_;
        mutable ng::GLShader    shader_;
        ng::Window*             window_;
        bool                    wireframe_ = false;
};

std::unique_ptr<Model>
load_triangle_model(const std::string& fn, ng::Window* window)
{
    typedef     TriangleModel::Points      Points;
    typedef     TriangleModel::Triangles   Triangles;
    typedef     TriangleModel::Triangle    Triangle;

    Points          points;
    Triangles       triangles;

    std::ifstream   in(fn.c_str());
    std::string     line;

    size_t i = 0;
    while (std::getline(in, line))
    {
        if (line[0] == '#') continue;
        if (line.empty()) continue;

        std::istringstream  iss(line);
        float x,y,z;
        iss >> x >> y >> z;
        points.push_back({x,y,z});

        if (i % 3 == 2)
            triangles.push_back(Triangle { i - 2, i - 1, i });

        ++i;
    }

    return std::unique_ptr<Model>(new TriangleModel(fn, points, triangles, window));
}

std::unique_ptr<Model>
load_object_model(const std::string& fn, ng::Window* window)
{
    typedef     TriangleModel::Points      Points;
    typedef     TriangleModel::Triangles   Triangles;
    typedef     TriangleModel::Triangle    Triangle;

    Points          points;
    Triangles       triangles;

    std::ifstream   in(fn.c_str());
    std::string     line;

    while (std::getline(in, line))
    {
        if (line[0] == '#') continue;
        if (line.empty()) continue;

        std::istringstream  iss(line);
        char t;
        iss >> t;
        if (t == 'v')
        {
            float x,y,z;
            iss >> x >> y >> z;
            points.push_back({x,y,z});
        } else if (t == 'f')
        {
            int i,j,k;
            iss >> i >> j >> k;
            triangles.push_back(Triangle { i - 1, j - 1, k - 1 });
        }
    }

    return std::unique_ptr<Model>(new TriangleModel(fn, points, triangles, window));
}

#endif
