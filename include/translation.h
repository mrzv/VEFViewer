#ifndef VEFVIEWER_TRANSLATION_H
#define VEFVIEWER_TRANSLATION_H

#include <nanogui/nanogui.h>

namespace   ng = nanogui;

struct Translation
{
    void                button(const ng::Vector2i& p, bool down)
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
                mat_(0,3) +=  float(last_.x() - start_.x()) / size_.x();
                mat_(1,3) += -float(last_.y() - start_.y()) / size_.y();
            }
            active_ = false;
        }
    }

    bool                active() const                              { return active_; }
    void                motion(const ng::Vector2i& p)               { last_ = p; }
    void                setSize(const ng::Vector2i& s)              { size_ = s; }
    void                reset()                                     { mat_ = ng::Matrix4f::Identity(); }

    const ng::Matrix4f& matrix() const
    {
        if (!active_)
            return mat_;

        res_ = mat_;

        res_(0,3) +=  float(last_.x() - start_.x()) / size_.x();
        res_(1,3) += -float(last_.y() - start_.y()) / size_.y();

        return res_;
    }

    ng::Vector2i            start_, last_;
    ng::Vector2i            size_;
    ng::Matrix4f            mat_;
    mutable ng::Matrix4f    res_;
    bool                    active_;
};

#endif
