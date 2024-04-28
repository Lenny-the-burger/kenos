// !! do not define version in shader the compiler does it for us !!
// This file includes common.glsl during compilation
#line 4

out vec4 FragColor;

uniform int debug_id;


void main()
{
	// get material from the ssbo
	Material mat = material_buffer[gl_PrimitiveID];
	Lightmap lm = lightmaps[gl_PrimitiveID];

	vec3 mat_col = vec3(mat.color_r, mat.color_g, mat.color_b);
	//vec3 mat_col = lm.diffuseColor.xyz;

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