#version 330
out vec4 clr;
uniform vec4 color;
uniform float wireframe;
smooth in vec3 n;
smooth in vec4 position_;
void main()
{
    // flat shading
    vec3 ec_normal = normalize(cross(dFdx(vec3(position_)), dFdy(vec3(position_))));
    clr.rgb = abs(vec3(color) * ec_normal[2]);
    float depth = ((position_.z / position_.w) + 1.0) * 0.5;
    clr.rgb = max(clr.rgb, vec3(color) * wireframe * (1 - depth));

    // normal shading
    //clr.rgb = abs(vec3(color) * (n[0] * .33 + n[1] * .33 + n[2] * .33));
    //clr.rgb = abs(vec3(color) * n[2]);

    // depth buffer shading
    //float depth = ((position_.z / position_.w) + 1.0) * 0.5;
    //clr.rgb = vec3(depth, depth, depth);

    clr.a   = color.a;
}
