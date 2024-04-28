#version 460 core

out vec4 FragColor;

uniform int debug_id;


// OpenGL has a fucked up way of including other files so now just define everything
// in the fragment shader

struct Lightmap {
    vec4 ambientColor;
    vec4 diffuseColor;
    vec4 specularColor;
    // Add other properties as needed
};


struct Material {
	float emissive_strength;
	float color_r; // seperate color channels so we dont have to deal with arrays
	float color_g;
	float color_b;
	float roughness;
};

// bind the material ssbo
layout(std430, binding = 2) buffer MaterialBuffer {
	Material material_buffer[];
};

layout(std430, binding = 3) buffer LightmapBuffer {
    Lightmap lightmaps[];
};

void main()
{
	// get material from the ssbo
	Material mat = material_buffer[gl_PrimitiveID];
	Lightmap lm = lightmaps[gl_PrimitiveID];

	//vec3 mat_col = vec3(mat.color_r, mat.color_g, mat.color_b);
	vec3 mat_col = lm.diffuseColor.xyz;

	// Assuming depth value is in range [0, 1]
    float depth = gl_FragCoord.z - 0.9f; // Fetch depth from the built-in variable

	depth *= 10.0f;

    // Output the depth value as grayscale

	if (gl_PrimitiveID == debug_id) {
		// Set to a purple for debugging
		mat_col = vec3(1.0, 0.0, 1.0);
	}

	float debug_dot = mat.emissive_strength;

    FragColor = vec4(mat_col * depth * debug_dot, 1.0);
}