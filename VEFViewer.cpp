#include <list>

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/button.h>
#include <nanogui/toolbutton.h>
#include <nanogui/popupbutton.h>
#include <nanogui/combobox.h>
#include <nanogui/progressbar.h>
#include <nanogui/entypo.h>
#include <nanogui/messagedialog.h>
#include <nanogui/textbox.h>
#include <nanogui/slider.h>
#include <nanogui/imagepanel.h>
#include <nanogui/imageview.h>
#include <nanogui/vscrollpanel.h>

#if defined(WIN32)
#include <windows.h>
#endif

#include <nanogui/glutil.h>

#include <iostream>
#include <fstream>
#include <memory>

#include <opts/opts.h>
#include <format.h>

#include <translation.h>

#include <models/vertex-model.h>
#include <models/edge-model.h>
#include <models/triangle-model.h>

namespace   ng = nanogui;

bool ends_with(const std::string& s, const std::string& ending)
{
    if (s.length() >= ending.length())
        return (0 == s.compare(s.length() - ending.length(), ending.length(), ending));
    else
        return false;
}

std::unique_ptr<Model>
load_model_by_filetype(const std::string& fn, ng::Window* window)
{
    if (ends_with(fn, ".vrt"))
        return load_vertex_model(fn, window);
    else if (ends_with(fn, ".edg"))
        return load_edge_model(fn, window);
    else if (ends_with(fn, ".tri"))
        return load_triangle_model(fn, window);
    else if (ends_with(fn, ".obj"))
        return load_object_model(fn, window);

    return 0;
}

class VEFViewer: public ng::Screen
{
    public:
        VEFViewer():
            ng::Screen(Eigen::Vector2i(800, 600), "VEFViewer"),
            arcball_(-2.)
        {
            using namespace nanogui;

            model_window_ = new Window(this, "Models");
            Window* window = model_window_;
            window->setPosition(Vector2i(15, 15));
            window->setLayout(new GroupLayout());

            Widget* tools = new Widget(window);
            tools->setLayout(new BoxLayout(BoxLayout::Horizontal, BoxLayout::Middle, 0, 6));
            Button* b = new Button(tools, "Open");
            b->setCallback([&,window]
                           {
                               auto fn = file_dialog({
                                                        {"vrt", "Vertices"},
                                                        {"edg", "Edges"},
                                                        {"obj", "Object files"},
                                                        {"tri", "Triangles"}
                                                     },
                                                     false);
                               if (fn.empty())
                                   return;
                               add_model(load_model_by_filetype(fn, window));
                               recenter();
                               perform_layout();
                           });
            b = new Button(tools, "Recenter");
            b->setCallback([&] { recenter(true); });

            performLayout(mNVGContext);
            arcball_.setSize(size());
            translation_.setSize(size());
            setBackground({ .5, .5, .5 });
        }

        void            add_model(std::unique_ptr<Model> m)         { models_.emplace_back(std::move(m)); }
        void            perform_layout()                            { performLayout(mNVGContext); }
        ng::Window*     model_window() const                        { return model_window_; }
        void            recenter(bool visible_only = false)
        {
            ng::Vector3f scene_min, scene_max;
            std::tie(scene_min, scene_max) = scene_bbox(visible_only);

            center_ = scene_min + (scene_max - scene_min)/2;
            range_  = scene_max - scene_min;

            if (scale_ == 0. || visible_only)
            {
                scale_ = 1.;
                scale_factor_ = scale_/10;
            }

            translation_.reset();
        }

        Model::BBox     scene_bbox(bool visible_only = false) const
        {
            auto it = models_.begin();
            while (it != models_.end() && !(*it)->visible())
                ++it;

            if (it == models_.end())
                return Model::BBox { ng::Vector3f::Zero(), ng::Vector3f::Zero() };

            Model::BBox bbox = (*it)->bbox();
            ng::Vector3f& scene_min = std::get<0>(bbox);
            ng::Vector3f& scene_max = std::get<1>(bbox);
            for (auto& m : models_)
            {
                if (visible_only && !m->visible()) continue;

                ng::Vector3f min, max;
                std::tie(min,max) = m->bbox();
                for (unsigned i = 0; i < 3; ++i)
                {
                    if (min[i] < scene_min[i])
                        scene_min[i] = min[i];
                    if (max[i] > scene_max[i])
                        scene_max[i] = max[i];
                }
            }

            return bbox;
        }

        virtual void    draw(NVGcontext *ctx)                                   { Screen::draw(ctx); }


        virtual void    framebufferSizeChanged()
        {
            arcball_.setSize(size());
            translation_.setSize(size());
        }

        virtual bool    mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers)
        {
            if (Screen::mouseButtonEvent(p,button,down,modifiers))
                return true;

            if (!down)
            {
                translation_.button(p, down);
                arcball_.button(p, down);
            } else
            {
                if (button == GLFW_MOUSE_BUTTON_1 && modifiers != GLFW_MOD_SHIFT)
                    arcball_.button(p, down);
                else if (button == GLFW_MOUSE_BUTTON_2 || (button == GLFW_MOUSE_BUTTON_1 && modifiers == GLFW_MOD_SHIFT))
                    translation_.button(p, down);
            }


            return true;
        }

        virtual bool    mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers)
        {
            if (arcball_.active())
                arcball_.motion(p);
            else if (translation_.active())
                translation_.motion(p);

            return true;
        }

        virtual bool    scrollEvent(const ng::Vector2i &p, const ng::Vector2f &rel)
        {
            scale_ += rel.y()*scale_factor_;
            if (scale_ < 0)
                scale_ = 0;
            return true;
        }

        virtual void    drawContents()
        {
            glEnable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ng::Matrix4f projection = ng::Matrix4f::Identity();

            projection.topLeftCorner<3,3>() /= range_.maxCoeff() / 1.9;
            projection.topLeftCorner<2,2>() *= scale_;
            projection.row(0)               *= (float) mSize.y() / (float) mSize.x();
            projection                      = translation_.matrix() * projection;

            ng::Matrix4f model = ng::Matrix4f::Identity();
            model.topRightCorner<3,1>() = -center_;

            ng::Matrix4f rotation     = arcball_.matrix(ng::Matrix4f());
            ng::Matrix4f view         = rotation;

            ng::Matrix4f mvp          = projection * view * model;

            for (auto& m : models_)
                m->draw(mvp, model, view, projection);
        }
    private:
        std::list<std::unique_ptr<Model>>       models_;
        float                                   scale_ = 0.;
        float                                   scale_factor_ = .1;
        ng::Arcball                             arcball_;
        ng::Vector3f                            center_ = ng::Vector3f::Zero();
        ng::Vector3f                            range_;
        ng::Window*                             model_window_;
        Translation                             translation_;
};

int main(int argc, char *argv[])
{
    using namespace opts;
    Options ops(argc, argv);

    std::vector<std::string>    vertex_filenames,
                                triangle_filenames,
                                edge_filenames,
                                object_filenames;
    ops
        >> Option('v', "vertex",    vertex_filenames,   "vertex file")
        >> Option('t', "triangle",  triangle_filenames, "triangle file")
        >> Option('e', "edge",      edge_filenames,     "edge file")
        >> Option('o', "object",    object_filenames,   "object file")
    ;

    if (ops >> Present('h', "help", "show help"))
    {
        fmt::print("Usage: {} [OPTIONS] FILENAMES\n{}", argv[0], ops);
        return 1;
    }

    try
    {
        nanogui::init();

        VEFViewer* app = new VEFViewer();
        glEnable(GL_PROGRAM_POINT_SIZE);

        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


        for (auto& fn : vertex_filenames)
            app->add_model(load_vertex_model(fn, app->model_window()));
        for (auto& fn : triangle_filenames)
            app->add_model(load_triangle_model(fn, app->model_window()));
        for (auto& fn : edge_filenames)
            app->add_model(load_edge_model(fn, app->model_window()));
        for (auto& fn : object_filenames)
            app->add_model(load_object_model(fn, app->model_window()));

        std::string fn;
        while (ops >> PosOption(fn))
            app->add_model(load_model_by_filetype(fn, app->model_window()));

        app->recenter();
        app->perform_layout();

        app->drawAll();
        app->setVisible(true);

        nanogui::mainloop();

        delete app;

        nanogui::shutdown();
    } catch (const std::runtime_error &e)
    {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
        #if defined(WIN32)
            MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
        #else
            fmt::print(std::cerr, "{}\n", error_msg);
        #endif
        return -1;
    }

    return 0;
}
