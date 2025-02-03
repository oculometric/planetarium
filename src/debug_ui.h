#pragma once

static const char* debug_mainpage_sls = 
R"(
HorizontalBox
(children = {
    SizeLimiter
    (
        max_size = [ 42, -1 ],
        child = VerticalBox
        (children = {
            HorizontalBox (children = { Label (text = "frametiming:"), Label : "frame_label" (alignment = 1)}),
            BorderedBox : "scene_box" (name = "scene properties", child = SizeLimiter (max_size = [-1, 12])),
            BorderedBox : "object_box" (name = "object properties", child = SizeLimiter (max_size = [-1, 12])),
            Label (text = "you can do it!")
        })
    ),
    BorderedBox (name = "log", child = TextArea : "console" ())
})
)";