#version 460 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 worldPos;

void main()
{
    vec4 vin = vec4(aPos, 1.0);
    vin = model * vin;

    worldPos = vin.xyz;

    // Apply view matrix
    vin = view * vin;

    // Apply projection matrix
    vin = projection * vin;

    gl_Position = vin;
}