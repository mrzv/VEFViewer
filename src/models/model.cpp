#include <models/model.h>

void
Model::
init_window(ng::Window* window)
{
    new nanogui::Label(window, name_);

    tools_ = new ng::Widget(window);
    tools_->setLayout(new ng::BoxLayout(ng::Orientation::Horizontal, ng::Alignment::Middle, 0, 6));

    auto b = new ng::Button(tools_, "", ENTYPO_ICON_EYE);
    b->setFlags(nanogui::Button::ToggleButton);
    b->setPushed(visible_);
    b->setChangeCallback([this](bool state) { visible_ = state; });

    auto color_picker = new ng::PopupButton(window, "Color");
    auto popup = color_picker->popup();
    color_wheel_ = new ng::ColorWheel(popup, color_);
    color_wheel_->setCallback([this](const nanogui::Color& color) { color_ = color; });
}

