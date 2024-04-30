// !! do not define version in shader the compiler does it for us !!
// This file includes common.glsl during compilation
#line 4

out vec4 FragColor;

in vec3 worldPos;

uniform int debug_id;

void main()
{
	// get material from the ssbo
	Material mat = material_buffer[gl_PrimitiveID];
	Lightmap lm = lightmaps[gl_PrimitiveID];

	vec3 mat_col = vec3(mat.color_r, mat.color_g, mat.color_b);
	//vec3 mat_col = lm.diffuseColor.xyz;

	// Assuming depth value is in range [0, 1]
    float depth = gl_FragCoord.z - 0.9f; // Fetch depth from the built-in 

	depth *= 10.0f;

	Triangle test_prim = get_primitive(979);

	bool is_within = point_in_triangle(worldPos, test_prim);

	float is_within_mult = is_within ? 1.0 : 0.0;

	// Debugging
	if (gl_PrimitiveID == debug_id) {
		// Set to a purple grid for debugging
		FragColor = vec4(DEBUG_COLOR * 
			(sin(worldPos.x * GRID_SHADER_SIZE) > cos(worldPos.z * GRID_SHADER_SIZE) ? 1.0 : 0.0),
			1.0);
		return;
	}

	float debug_dot = mat.emissive_strength;

    FragColor = vec4(mat_col * depth * debug_dot * is_within_mult, 1.0);
}