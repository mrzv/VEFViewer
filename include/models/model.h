#ifndef VEFVIEWER_MODEL_H
#define VEFVIEWER_MODEL_H

#include <nanogui/nanogui.h>
#include <nanogui/colorwheel.h>
#include <nanogui/popupbutton.h>

namespace ng = nanogui;

struct Model
{
    typedef std::tuple<nanogui::Vector3f, nanogui::Vector3f>            BBox;

                            Model(const std::string& name,
                                  const ng::Color&   color = ng::Color(0.f, 0.f, 0.f, 1.f)):
                                name_(name),
                                color_(color)
    {}

    virtual                 ~Model()                                    {}

    virtual void            init_window(ng::Window* window);

    virtual void            draw(const ng::Matrix4f& mvp,
                                 const ng::Matrix4f& model,
                                 const ng::Matrix4f& view,
                                 const ng::Matrix4f& projection) const  =0;
    virtual const BBox&     bbox() const                                =0;

    bool                    visible() const                             { return visible_; }

    const ng::Vector4f&     color() const                               { return color_; }

    bool            visible_ = true;
    std::string     name_;
    ng::Color       color_;
    ng::Widget*     tools_;
    ng::ColorWheel* color_wheel_;
};

#endif
