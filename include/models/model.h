#ifndef VEFVIEWER_MODEL_H
#define VEFVIEWER_MODEL_H

#include <nanogui/nanogui.h>

struct Model
{
    typedef std::tuple<nanogui::Vector3f, nanogui::Vector3f>            BBox;

    virtual void            draw(const nanogui::Matrix4f& mvp) const    =0;
    virtual const BBox&     bbox() const                                =0;
};

#endif
