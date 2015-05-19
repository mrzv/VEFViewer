#ifndef VEFVIEWER_TRANSLATION_H
#define VEFVIEWER_TRANSLATION_H

#include <nanogui/nanogui.h>
#include <nanogui/glutil.h>

namespace   ng = nanogui;

struct Controls: public ng::Arcball
{
                        Controls(float speed):
                            Arcball(speed)                  {}

    virtual void    mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers)
    {
        if (!down)
        {
            this->button(p, down);
            Arcball::button(p, down);
        } else
        {
            if (button == GLFW_MOUSE_BUTTON_1 && modifiers != GLFW_MOD_SHIFT)
                Arcball::button(p, down);
            else if (button == GLFW_MOUSE_BUTTON_2 || (button == GLFW_MOUSE_BUTTON_1 && modifiers == GLFW_MOD_SHIFT))
                this->button(p, down);
        }
    }

    virtual void    mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers)
    {
        if (Arcball::active())
            Arcball::motion(p);
        else if (active_)
            motion(p);
    }

    virtual void    scrollEvent(const ng::Vector2i &p, const ng::Vector2f &rel)
    {
        scale_ += rel.y()*scale_factor_;
        if (scale_ < 0)
            scale_ = 0;
    }

    float           scale() const               { return scale_; }
    void            set_scale(float s)          { scale_ = s; }

    void            button(const ng::Vector2i& p, bool down)
    {
        if (down)
        {
            active_ = true;
            start_ = p;
            last_  = p;
        } else
        {
            if (active_)
            {
                mat_(0,3) +=  float(last_.x() - start_.x()) / mSize.x() * 2;
                mat_(1,3) += -float(last_.y() - start_.y()) / mSize.y() * 2;
            }
            active_ = false;
        }
    }

    void                motion(const ng::Vector2i& p)               { last_ = p; }
    void                reset()                                     { mat_ = ng::Matrix4f::Identity(); }

    ng::Matrix4f        projection(ng::Matrix4f m) const
    {
        m.topLeftCorner<2,2>() *= scale_;
        return translation_matrix() * m;
    }

    const ng::Matrix4f& translation_matrix() const
    {
        if (!active_)
            return mat_;

        res_ = mat_;

        res_(0,3) +=  float(last_.x() - start_.x()) / mSize.x() * 2;
        res_(1,3) += -float(last_.y() - start_.y()) / mSize.y() * 2;

        return res_;
    }

    ng::Matrix4f rotation_matrix() const
    {
        return Arcball::matrix(ng::Matrix4f());
    }

    ng::Vector2i            start_, last_;
    ng::Matrix4f            mat_;
    mutable ng::Matrix4f    res_;
    bool                    active_;
    float                   scale_ = 0.;
    float                   scale_factor_ = .1;
};

#endif
