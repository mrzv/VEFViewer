#ifndef VEFVIEWER_SPHERE_MODEL_H
#define VEFVIEWER_SPHERE_MODEL_H

#include "model.h"
#include <format.h>

extern unsigned char sphere_vrt[];
extern unsigned int  sphere_vrt_len;
extern unsigned char triangle_frg[];
extern unsigned int  triangle_frg_len;

namespace       ng = nanogui;

struct SphereModel: public Model
{
    typedef         std::tuple<unsigned,unsigned,unsigned>              Triangle;
    typedef         std::vector<ng::Vector4f>                           Spheres;
    typedef         std::vector<Triangle>                               Triangles;

                    SphereModel(const std::string&      name,
                                const Spheres&          spheres,
                                ng::Window*             window):
                        Model(name, window, ng::Color(1.f, 1.f, 0.f, 1.f)), window_(window),
                        spheres_(spheres)
    {
        ng::Vector3f min = ng::Vector3f { spheres[0][0], spheres[0][1], spheres[0][2] } - spheres[0][3] * ng::Vector3f { 1., 1., 1. },
                     max = ng::Vector3f { spheres[0][0], spheres[0][1], spheres[0][2] } + spheres[0][3] * ng::Vector3f { 1., 1., 1. };
        for (auto& s : spheres)
        {
            for (unsigned i = 0; i < 3; ++i)
            {
                if (s[i] + s[3] > max[i])
                    max[i] = s[i] + s[3];
                if (s[i] - s[3] < min[i])
                    min[i] = s[i] - s[3];
            }
        }

        auto b = new ng::Button(tools_, "", ENTYPO_ICON_SHARE);
        b->setFlags(nanogui::Button::ToggleButton);
        b->setPushed(wireframe_);
        b->setChangeCallback([this](bool state) { wireframe_ = state; });
        b->setTooltip("wireframe");

        bbox_ = BBox { min, max };

        //shader_.initFromFiles(name, "vertex.glsl", "fragment.glsl");
        shader_.init(
            name,       // an identifying name
            std::string(sphere_vrt,   sphere_vrt   + sphere_vrt_len),
            std::string(triangle_frg, triangle_frg + triangle_frg_len)
        );

        // fill a sphere
        unsigned    rings   = 60;
        unsigned    sectors = 60;
        n_ = rings * sectors;
        ng::MatrixXu indices(3, n_ * 2);
        ng::MatrixXf normals(3, n_);
        ng::MatrixXf positions(3, n_);
        float const R = 1./(float)rings;
        float const S = 1./(float)sectors;

        size_t vidx = 0, tidx = 0;
        for(int r = 0; r < rings; ++r)
        {
            for(int s = 0; s < sectors; ++s)
            {
                float const x = cos(2*M_PI * s * S) * sin(2*M_PI * r * R );
                float const y = sin(2*M_PI * s * S) * sin(2*M_PI * r * R );
                float const z = cos(2*M_PI * r * R);

                positions.col(vidx++) = ng::Vector3f { x, y, z };
                {
                    int curRow  = r     * sectors;
                    int nextRow = (r+1) * sectors;

                    using Vector3u = Eigen::Matrix<unsigned, 3, 1>;
                    indices.col(tidx++) = Vector3u { curRow + s, nextRow + s,               nextRow + (s+1) % sectors };
                    indices.col(tidx++) = Vector3u { curRow + s, nextRow + (s+1) % sectors, curRow  + (s+1) % sectors };
                }
                // TODO: fill normals in case we use better shading
            }
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

            for (auto& s : spheres_)
            {
                shader_.setUniform("sphere", s);
                shader_.drawIndexed(GL_TRIANGLES, 0, n_);
            }

            if (wireframe_)
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }
    virtual const BBox&     bbox() const                    { return bbox_; }

    private:
                        SphereModel(const SphereModel&)     = delete;
        SphereModel&    operator=(const SphereModel&)       = delete;

    private:
        BBox                    bbox_;
        size_t                  n_;
        mutable ng::GLShader    shader_;
        ng::Window*             window_;
        Spheres                 spheres_;
        bool                    wireframe_ = false;
};

std::unique_ptr<Model>
load_sphere_model(const std::string& fn, ng::Window* window)
{
    typedef         std::vector<ng::Vector4f>   Spheres;
    Spheres         spheres;
    std::ifstream   in(fn.c_str());
    std::string line;

    while (std::getline(in, line))
    {
        if (line[0] == '#') continue;
        std::istringstream  iss(line);
        float x,y,z,r;
        iss >> x >> y >> z >> r;
        spheres.push_back({x,y,z,r});
    }

    return std::unique_ptr<Model>(new SphereModel(fn, spheres, window));
}

#endif

