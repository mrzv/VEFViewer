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

#include <models/vertex-model.h>
#include <models/edge-model.h>
#include <models/triangle-model.h>

namespace   ng = nanogui;

class VEFViewer: public ng::Screen
{
    public:
        VEFViewer():
            ng::Screen(Eigen::Vector2i(800, 600), "VEFViewer")
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
                               auto fn = file_dialog({ {"vrt", "Vertices"}, {"tri", "Triangles"} }, false);
                               if (fn.empty())
                                   return;
                               add_model(load_vertex_model(fn, window));
                               recenter();
                               perform_layout();
                           });

            performLayout(mNVGContext);
            arcball_.setSize(size());
            setBackground({ .5, .5, .5 });
        }

        void            add_model(std::unique_ptr<Model> m)         { models_.emplace_back(std::move(m)); }
        void            perform_layout()                            { performLayout(mNVGContext); }
        ng::Window*     model_window() const                        { return model_window_; }
        void            recenter()
        {
            ng::Vector3f scene_min, scene_max;
            std::tie(scene_min, scene_max) = scene_bbox();

            center_ = scene_min + (scene_max - scene_min)/2;

            if (scale_ == 0.)
            {
                float range = std::max({ scene_max[0] - scene_min[0],
                                         scene_max[1] - scene_min[1],
                                         scene_max[2] - scene_min[2] });
                scale_ = 1./range;
                scale_factor_ = scale_/10;
            }
        }

        Model::BBox     scene_bbox() const
        {
            if (models_.empty())
                return Model::BBox { ng::Vector3f::Zero(), ng::Vector3f::Zero() };

            Model::BBox bbox = models_.front()->bbox();
            ng::Vector3f& scene_min = std::get<0>(bbox);
            ng::Vector3f& scene_max = std::get<1>(bbox);
            for (auto& m : models_)
            {
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

        virtual bool    mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers)
        {
            if (Screen::mouseButtonEvent(p,button,down,modifiers))
                return true;

            arcball_.button(p, down);

            return true;
        }

        virtual bool    mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers)
        {
            arcball_.motion(p);
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
            ng::Matrix4f mvp = ng::Matrix4f::Identity();

            mvp.topLeftCorner<3,3>() *= scale_;
            mvp.row(0)               *= (float) mSize.y() / (float) mSize.x();

            ng::Matrix4f translation = ng::Matrix4f::Identity();
            translation.topRightCorner<3,1>() = -center_;

            ng::Matrix4f rotation     = arcball_.matrix(ng::Matrix4f());
            mvp                      *= rotation * translation;

            for (auto& m : models_)
                m->draw(mvp);
        }
    private:
        std::list<std::unique_ptr<Model>>       models_;
        float                                   scale_ = 0.;
        float                                   scale_factor_ = .1;
        ng::Arcball                             arcball_;
        ng::Vector3f                            center_ = ng::Vector3f::Zero();
        ng::Window*                             model_window_;
};

int main(int argc, char *argv[])
{
    using namespace opts;
    Options ops(argc, argv);

    std::vector<std::string>    vertex_filenames;
    std::vector<std::string>    triangle_filenames;
    std::vector<std::string>    edge_filenames;
    ops
        >> Option('v', "vertex",    vertex_filenames,   "vertex file")
        >> Option('t', "triangle",  triangle_filenames, "triangle file")
        >> Option('e', "edge",      edge_filenames,     "edge file")
    ;

    if (ops >> Present('h', "help", "show help"))
    {
        fmt::print("Usage: {} [OPTIONS] FILENAMES\n{}", argv[0], ops);
        return 1;
    }

    std::string fn;
    while (ops >> PosOption(fn))
    {
        // determine filetype of fn and put it accordingly
        vertex_filenames.push_back(fn);
    }

    try
    {
        nanogui::init();

        VEFViewer* app = new VEFViewer();
        glEnable(GL_PROGRAM_POINT_SIZE);

        for (auto& fn : vertex_filenames)
            app->add_model(load_vertex_model(fn, app->model_window()));
        for (auto& fn : triangle_filenames)
            app->add_model(load_triangle_model(fn, app->model_window()));
        for (auto& fn : edge_filenames)
            app->add_model(load_edge_model(fn, app->model_window()));
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
