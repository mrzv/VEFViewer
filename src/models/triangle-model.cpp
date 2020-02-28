#include <models/triangle-model.h>
#include <fmt/format.h>
#include <highfive/H5File.hpp>
#include <happly.h>
#include <nano_obj.h>

extern unsigned char triangle_vrt[];
extern unsigned int  triangle_vrt_len;
extern unsigned char triangle_frg[];
extern unsigned int  triangle_frg_len;

TriangleModel::
TriangleModel(const std::string&      name,
              const Points&           points,
              const Triangles&        triangles):
    Model(name, ng::Color(1.f, 1.f, 0.f, 1.f))
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

    bbox_ = BBox { min, max };

    //shader_.initFromFiles(name, "vertex.glsl", "fragment.glsl");
    shader_.init(
        name,       // an identifying name
        std::string(triangle_vrt, triangle_vrt + triangle_vrt_len),
        std::string(triangle_frg, triangle_frg + triangle_frg_len)
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

void
TriangleModel::
init_window(ng::Window* window)
{
    Model::init_window(window);

    auto b = new ng::Button(tools_, "", ENTYPO_ICON_SHARE);
    b->setFlags(nanogui::Button::ToggleButton);
    b->setPushed(wireframe_);
    b->setChangeCallback([this](bool state) { wireframe_ = state; });
    b->setTooltip("wireframe");
}

void
TriangleModel::
draw(const ng::Matrix4f& mvp,
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

std::unique_ptr<Model>
load_triangle_model(const std::string& fn)
{
    typedef     TriangleModel::Points      Points;
    typedef     TriangleModel::Triangles   Triangles;
    typedef     TriangleModel::Triangle    Triangle;

    Points          points;
    Triangles       triangles;

    zstr::ifstream  in(fn.c_str());
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

    return std::unique_ptr<Model>(new TriangleModel(fn, points, triangles));
}

std::unique_ptr<Model>
load_object_model(const std::string& fn)
{
    typedef     TriangleModel::Points      Points;
    typedef     TriangleModel::Triangles   Triangles;
    typedef     TriangleModel::Triangle    Triangle;

    Points          points;
    Triangles       triangles;

    zstr::ifstream  in(fn.c_str());

    obj::mesh m = obj::read(in);
    for (auto& v : m.vertices)
        points.push_back({v.x, v.y, v.z});

    for (auto& f : m.faces)
    {
        if (f.vertices.size() != 3)
            throw std::runtime_error(fmt::format("Cannot read from {} a face with {} vertices", fn, f.vertices.size()));
        triangles.push_back(Triangle { f.vertices[0].v, f.vertices[1].v, f.vertices[2].v });
    }

    return std::unique_ptr<Model>(new TriangleModel(fn, points, triangles));
}

std::unique_ptr<Model>
load_stl_model(const std::string& fn)
{
    typedef     TriangleModel::Points      Points;
    typedef     TriangleModel::Triangles   Triangles;
    typedef     TriangleModel::Triangle    Triangle;

    Points          points;
    Triangles       triangles;

    zstr::ifstream  in(fn.c_str());

    auto data = stl::parse_stl(in);

    for (auto& t : data.triangles)
    {
        points.emplace_back(t.v1.x,t.v1.y,t.v1.z);
        points.emplace_back(t.v2.x,t.v2.y,t.v2.z);
        points.emplace_back(t.v3.x,t.v3.y,t.v3.z);

        size_t n = points.size();
        triangles.emplace_back(n-3, n-2, n-1);
    }

    return std::unique_ptr<Model>(new TriangleModel(fn, points, triangles));
}

std::unique_ptr<Model>
load_triangle_hdf5_model(const std::string& fn)
{
    typedef     TriangleModel::Points      Points;
    typedef     TriangleModel::Triangles   Triangles;
    typedef     TriangleModel::Triangle    Triangle;

    Points          points;
    Triangles       triangles;

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

    for (size_t i = 0; i < xs.size(); ++i)
        points.emplace_back(xs[i], ys[i], zs[i]);

    HighFive::DataSet tri = in.getDataSet("triangles");
    std::vector<size_t> tris;
    tri.read(tris);

    for (size_t i = 0; i < tris.size(); i += 3)
        triangles.emplace_back(tris[i], tris[i+1], tris[i+2]);

    return std::unique_ptr<Model>(new TriangleModel(fn, points, triangles));
}

std::unique_ptr<Model>
load_ply_model(const std::string& fn)
{
    using Points    = TriangleModel::Points;
    using Triangles = TriangleModel::Triangles;

    // Construct the data object by reading from file
    happly::PLYData plyIn(fn);

    Points points;
    std::vector<std::array<double, 3>> vPos = plyIn.getVertexPositions();
    for(auto& p : vPos)
        points.emplace_back(p[0], p[1], p[2]);

    Triangles triangles;
    std::vector<std::vector<size_t>>   fInd = plyIn.getFaceIndices<size_t>();
    for (auto& t : fInd)
    {
        if (t.size() != 3)
            throw std::runtime_error(fmt::format("Cannot read from {} a face with {} vertices", fn, t.size()));

        triangles.emplace_back(t[0], t[1], t[2]);
    }

    return std::unique_ptr<Model>(new TriangleModel(fn, points, triangles));
}

