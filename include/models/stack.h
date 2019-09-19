#ifndef VEFVIEWER_STACK_H
#define VEFVIEWER_STACK_H

#include <memory>
#include "model.h"

namespace ng = nanogui;

struct Stack: public Model
{
            Stack(std::vector<std::unique_ptr<Model>>& models);

    virtual void            init_window(ng::Window* window) override;

    virtual void            draw(const ng::Matrix4f& mvp,
                                 const ng::Matrix4f& model,
                                 const ng::Matrix4f& view,
                                 const ng::Matrix4f& projection) const override;

    virtual const BBox&     bbox() const override   { return bbox_; }

    int     i_ = 0;
    BBox    bbox_;
    std::vector<std::unique_ptr<Model>> models_;
};

#endif
