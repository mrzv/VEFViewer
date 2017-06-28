#include <list>

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/button.h>
#include <nanogui/entypo.h>
#include <nanogui/textbox.h>
#include <nanogui/slider.h>

#if defined(WIN32)
#include <windows.h>
#endif

#include <nanogui/glutil.h>

#include <iostream>
#include <fstream>
#include <memory>

#include <opts/opts.h>
#include <format.h>

#include <controls.h>

#include <models/vertex-model.h>
#include <models/edge-model.h>
#include <models/triangle-model.h>
#include <models/sphere-model.h>

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
    if (ends_with(fn, ".vrt") || ends_with(fn, ".vrt.gz"))
        return load_vertex_model(fn, window);
    else if (ends_with(fn, ".edg") || ends_with(fn, ".edg.gz"))
        return load_edge_model(fn, window);
    else if (ends_with(fn, ".tri") || ends_with(fn, ".tri.gz"))
        return load_triangle_model(fn, window);
    else if (ends_with(fn, ".obj") || ends_with(fn, ".obj.gz"))
        return load_object_model(fn, window);
    else if (ends_with(fn, ".sph") || ends_with(fn, ".sph.gz"))
        return load_sphere_model(fn, window);

    fmt::print(std::cerr, "Unknown file type: {}\n", fn);
    return 0;
}

class VEFViewer: public ng::Screen
{
    public:
        VEFViewer():
            ng::Screen(Eigen::Vector2i(800, 600), "VEFViewer"),
            controls_(-2.)
        {
            using namespace nanogui;

            model_window_ = new Window(this, "Models");
            Window* window = model_window_;
            window->setPosition(Vector2i(15, 15));
            window->setLayout(new GroupLayout());

            Widget* tools = new Widget(window);
            tools->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 6));
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

            background_window_ = new Window(this, "Background");
            background_window_->setLayout(new GroupLayout());
            background_window_->center();
            auto color_wheel = new ng::ColorWheel(background_window_, background_);
            color_wheel->setCallback([this](const nanogui::Color& color) { background_ = color.head<3>(); setBackground(background_); });
            background_window_->setVisible(false);

            performLayout(mNVGContext);
            controls_.setSize(size());

            setBackground(background_);
        }

        void            add_model(std::unique_ptr<Model> m)         { if (m) models_.emplace_back(std::move(m)); }
        void            perform_layout()                            { performLayout(mNVGContext); }
        ng::Window*     model_window() const                        { return model_window_; }
        void            recenter(bool visible_only = false)
        {
            ng::Vector3f scene_min, scene_max;
            std::tie(scene_min, scene_max) = scene_bbox(visible_only);

            center_ = scene_min + (scene_max - scene_min)/2;
            range_  = scene_max - scene_min;

            if (controls_.scale() == 0. || visible_only)
                controls_.set_scale(1.);

            controls_.reset();
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

        virtual void    draw(NVGcontext *ctx) override                          { Screen::draw(ctx); if (saving_) savePPM(); }

        virtual bool    resizeEvent(const ng::Vector2i& s) override
        {
            controls_.setSize(s);
            return true;
        }

        virtual bool    mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override
        {
            if (Screen::mouseButtonEvent(p,button,down,modifiers))
                return true;

            controls_.mouseButtonEvent(p, button, down, modifiers);
            return true;
        }

        virtual bool    mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override
        {
            controls_.mouseMotionEvent(p, rel, button, modifiers);
            return true;
        }

        virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override
        {
            if (Screen::keyboardEvent(key, scancode, action, modifiers))
                return true;

            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            {
                setVisible(false);
                return true;
            }

            if (key == GLFW_KEY_M && action == GLFW_PRESS && !saving_)
            {
                model_window_->setVisible(!model_window_->visible());
                return true;
            }

            if (key == GLFW_KEY_B && action == GLFW_PRESS && !saving_)
            {
                background_window_->setVisible(!background_window_->visible());
                return true;
            }

            if (key == GLFW_KEY_S && action == GLFW_PRESS)
            {
                if (saving_)
                {
                    saving_ = false;
                    model_window_->setVisible(model_visible_before_save_);
                    background_window_->setVisible(background_visible_before_save_);
                } else
                {
                    save_frame_ = 0;
                    save_name_ = ng::file_dialog({{"", "Prefix for portable pixmap images"}}, true);
                    saving_ = true;
                    model_visible_before_save_ = model_window_->visible();
                    background_visible_before_save_ = background_window_->visible();
                    model_window_->setVisible(false);
                    background_window_->setVisible(false);
                }

                return true;
            }

            return false;
        }

        virtual bool    scrollEvent(const ng::Vector2i &p, const ng::Vector2f &rel) override
        {
            controls_.scrollEvent(p, rel);
            return true;
        }

        virtual void    drawContents() override
        {
            glEnable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ng::Matrix4f projection = ng::Matrix4f::Identity();
            projection.topLeftCorner<3,3>() /= range_.norm() / 1.9;
            projection.row(0)               *= (float) mSize.y() / (float) mSize.x();
            projection                       = controls_.projection(projection);

            ng::Matrix4f model = ng::Matrix4f::Identity();
            model.topRightCorner<3,1>() = -center_;

            ng::Matrix4f view         = controls_.rotation_matrix();

            ng::Matrix4f mvp          = projection * view * model;

            for (auto& m : models_)
                m->draw(mvp, model, view, projection);
        }

        void            savePPM()
        {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            std::vector<GLubyte> pixels(3 * mFBSize.x() * mFBSize.y());
            glReadPixels(0, 0, mFBSize.x(), mFBSize.y(), GL_RGB, GL_UNSIGNED_BYTE, &pixels[0]);
            auto outfn = fmt::format("{}-{:05}.ppm", save_name_, save_frame_++);
            fmt::print("Saving screenshot to {}\n", outfn);
            std::ofstream out(outfn, std::ios::binary);
            out << "P6\n" << mFBSize.x() << ' ' << mFBSize.y() << "\n255\n";
            out.write((char*) &pixels[0], pixels.size());
        }
    private:
        std::list<std::unique_ptr<Model>>       models_;
        Controls                                controls_;
        ng::Vector3f                            center_ = ng::Vector3f::Zero();
        ng::Vector3f                            range_;
        ng::Window*                             model_window_;
        ng::Window*                             background_window_;
        ng::Vector3f                            background_ = { .5, .5, .5 };

        // screenshot support
        bool        saving_ = false;
        size_t      save_frame_;
        std::string save_name_;
        bool        model_visible_before_save_,
                    background_visible_before_save_;

};

int main(int argc, char *argv[])
{
    using namespace opts;
    Options ops(argc, argv);

    std::vector<std::string>    vertex_filenames,
                                triangle_filenames,
                                edge_filenames,
                                object_filenames,
                                sphere_filenames;
    ops
        >> Option('v', "vertex",    vertex_filenames,   "vertex file")
        >> Option('t', "triangle",  triangle_filenames, "triangle file")
        >> Option('e', "edge",      edge_filenames,     "edge file")
        >> Option('o', "object",    object_filenames,   "object file")
        >> Option('s', "sphere",    sphere_filenames,   "sphere file")
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
        for (auto& fn : sphere_filenames)
            app->add_model(load_sphere_model(fn, app->model_window()));

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
