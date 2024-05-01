// !! do not define version in shader the compiler does it for us !!
// This file includes common.glsl during compilation
#line 4

out vec4 FragColor;

in vec3 worldPos;

uniform int debug_id;

void main()
{
	Triangle tri = get_primitive(gl_PrimitiveID);

	// Debugging
	if (gl_PrimitiveID == debug_id) {
		// Set to a purple grid for debugging
		FragColor = debug_shader(worldPos, tri);
		return;
	}

	// get material from the ssbo
	Material mat = material_buffer[gl_PrimitiveID];
	Lightmap lm = lightmaps[gl_PrimitiveID];

	vec3 mat_col = vec3(mat.color_r, mat.color_g, mat.color_b);
	//vec3 mat_col = lm.diffuseColor.xyz;

	// Assuming depth value is in range [0, 1]
    float depth = gl_FragCoord.z - 0.9f; // Fetch depth from the built-in 

	depth *= 10.0f;

	float debug_dot = mat.emissive_strength;

    //FragColor = vec4(mat_col * depth * debug_dot * is_within_mult, 1.0);

	vec3 tempOrg = vec3(0.0, 0.0, 0.0);

	float testval = FRDF_gauss_adj(distance(worldPos, tempOrg), 0, 1);

	FragColor = vec4(vec3(testval), 1.0);
}