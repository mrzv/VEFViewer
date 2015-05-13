#ifndef VEFVIEWER_MODEL_H
#define VEFVIEWER_MODEL_H

#include <nanogui/nanogui.h>

struct Model
{
    typedef std::tuple<nanogui::Vector3f, nanogui::Vector3f>            BBox;

                            Model(const std::string& name,
                                  nanogui::Window*   window)
    {
        new nanogui::Label(window, name);

        auto b = new nanogui::Button(window, "Visible");
        b->setButtonFlags(nanogui::Button::ToggleButton);
        b->setPushed(visible_);
        b->setChangeCallback([this](bool state) { visible_ = state; });
    }

    virtual void            draw(const nanogui::Matrix4f& mvp) const    =0;
    virtual const BBox&     bbox() const                                =0;

    bool                    visible() const                             { return visible_; }

    bool visible_ = true;
};

#endif
