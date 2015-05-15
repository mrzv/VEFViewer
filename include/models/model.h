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
                                  nanogui::Window*   window,
                                  const ng::Vector4f& color = { 0., 0., 0., 1. }):
                                color_(color)
    {
        new nanogui::Label(window, name);

        tools_ = new ng::Widget(window);
        tools_->setLayout(new ng::BoxLayout(ng::BoxLayout::Horizontal, ng::BoxLayout::Middle, 0, 6));

        auto b = new ng::Button(tools_, "", ENTYPO_ICON_EYE);
        b->setButtonFlags(nanogui::Button::ToggleButton);
        b->setPushed(visible_);
        b->setChangeCallback([this](bool state) { visible_ = state; });

        auto color_picker = new ng::PopupButton(window, "Color");
        auto popup = color_picker->popup();
        auto color_wheel = new ng::ColorWheel(popup, color_.topLeftCorner<3,1>());
        color_wheel->setCallback([this](const nanogui::Vector3f& color) { color_.topLeftCorner<3,1>() = color; });
    }

    virtual void            draw(const nanogui::Matrix4f& mvp) const    =0;
    virtual const BBox&     bbox() const                                =0;

    bool                    visible() const                             { return visible_; }

    const ng::Vector4f&     color() const                               { return color_; }

    bool            visible_ = true;
    ng::Vector4f    color_;
    ng::Widget*     tools_;
};

#endif
