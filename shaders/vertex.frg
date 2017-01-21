#version 330
out vec4 clr;
uniform vec4 color;
void main()
{
    vec2 p = gl_PointCoord * 2.0 - vec2(1.0);
    float r = sqrt(dot(p,p));

    if (r > 1.)
        discard;
    else
    {
        //clr = color * (1. - r)*2;
        //clr.a = 1.;
        clr = color;
    }

    //clr = color;
}
