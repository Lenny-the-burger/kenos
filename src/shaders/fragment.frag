// !! do not define version in shader the compiler does it for us !!
// This file includes common.glsl during compilation
#line 4

out vec4 FragColor;

in vec3 worldPos;

uniform int debug_id;

uniform float test_brightness;

uniform int shadow_test_max;

void main()
{
	Triangle tri = get_primitive(gl_PrimitiveID);

	// Debugging
	if (gl_PrimitiveID == debug_id) {
		// Set to a purple grid for debugging
		FragColor = debug_shader(worldPos, tri);
		return;
	}

	// early exit for test emissive surfaces
	if (gl_PrimitiveID == 979 || gl_PrimitiveID == 978) {
		FragColor = vec4(1.0, 1.0, 1.0, 1.0);
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

    //FragColor = vec4(mat_col * depth * debug_dot, 1.0);

	Triangle test_prim = get_primitive(979);

	float testval = convolve(worldPos, test_prim);

	test_prim = get_primitive(978);

	testval += convolve(worldPos, test_prim);

	testval *= test_brightness;

	// shadow test
	test_prim = get_primitive(979);
	
	// what % obscured is the currect fragment
	float shadow = 0.0;

	vec3 light_dir = vec3(0, -1, 0);

	// get all the vertices we need for shadows at once (hard coded for now)

	int s_indeces[2904];
	float s_vertices[5904];


	for (int i = 0; i < shadow_test_max; i++) {
		shadow += point_in_triangle_shadow(worldPos, light_dir, i) ? 1.0 : 0.0;
	}

	// If any more than one of the potential shadowers block the point then we are shadowed
	shadow = 1 - clamp(shadow, 0.0, 0.5);

	testval *= shadow;

	FragColor = vec4(vec3(testval), 1.0);
	//FragColor = vec4(mat_col * testval, 1.0);
}