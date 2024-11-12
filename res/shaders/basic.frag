#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform bool useColor;

void main()
{
    if (useColor)
        FragColor = vec4(ourColor, 1.0f);
    else
        FragColor = texture(texture1, TexCoord);
}
