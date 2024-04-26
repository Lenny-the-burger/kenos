#version 460 core

out vec4 FragColor;

uniform int debug_id;


// OpenGL has a fucked up way of including other files so now just define everything
// in the fragment shader


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

	// Assuming depth value is in range [0, 1]
    float depth = gl_FragCoord.z - 0.9f; // Fetch depth from the built-in variable

	depth *= 10.0f;

    // Output the depth value as grayscale

	if (gl_PrimitiveID == debug_id) {
		// Set to a purple for debugging
		mat_col = vec3(1.0, 0.0, 1.0);
	}

    FragColor = vec4(mat_col * depth, 1.0);
}