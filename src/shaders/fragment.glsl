#version 460 core
out vec4 FragColor;

struct Material {
	float emissive_strength;
	float color_r; // seperate color channels so we dont have to deal with arrays
	float color_g;
	float color_b;
	float roughness;
};

// bind the material ssbo
layout(std430, binding = 0) buffer MaterialBuffer {
	Material material_buffer[];
};

void main()
{
	// get material from the ssbo
	Material mat = material_buffer[gl_PrimitiveID];

	vec3 mat_col = vec3(mat.color_r, mat.color_g, mat.color_b);

    FragColor = vec4(mat_col, 1.0);

	// Assuming depth value is in range [0, 1]
    //float depth = gl_FragCoord.z; // Fetch depth from the built-in variable

    // Output the depth value as grayscale
    //FragColor = vec4(depth, depth, depth, 1.0);
}