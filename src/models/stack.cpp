#include <models/stack.h>

Stack::
Stack(std::vector<std::unique_ptr<Model>>& models):
    Model("Stack")
{
    models_.swap(models);
    ng::Vector3f g_min, g_max;
    std::tie(g_min, g_max) = models_[0]->bbox();
    for (auto& m : models_)
    {
        ng::Vector3f min, max;
        std::tie(min,max) = m->bbox();
        for (unsigned i = 0; i < 3; ++i)
        {
            if (min[i] < g_min[i]) g_min[i] = min[i];
            if (max[i] > g_max[i]) g_max[i] = max[i];
        }
    }
    bbox_ = BBox { g_min, g_max };
}

void
Stack::
init_window(ng::Window* window)
{
    Model::init_window(window);

    auto panel = new ng::Widget(window);
    panel->setLayout(new ng::BoxLayout(ng::Orientation::Horizontal, ng::Alignment::Middle, 0, 20));

    auto slider = new ng::Slider(panel);
    slider->setValue(0);
    slider->setTooltip("model");

    auto intBox = new ng::IntBox<int>(panel);
    intBox->setFixedSize(ng::Vector2i(60, 25));
    intBox->setValue(i_);
    intBox->setMinMaxValues(0, models_.size() - 1);
    intBox->setSpinnable(true);
    intBox->setCallback([this,slider](int i)
                        {
                            i_ = i;
                            slider->setValue(float(i_) / (models_.size() - 1));
                        });
    slider->setCallback([this,intBox](float x)
                        {
                            i_ = x * (models_.size() - 1);
                            intBox->setValue(i_);
                        });

    color_wheel_->setCallback([this](const nanogui::Color& color)
                              {
                                color_ = color;
                                for(auto& m : models_)
                                    m->color_ = color;
                              });
}

void
Stack::
draw(const ng::Matrix4f& mvp,
     const ng::Matrix4f& model,
     const ng::Matrix4f& view,
     const ng::Matrix4f& projection) const
{
    models_[i_]->draw(mvp, model, view, projection);
}
