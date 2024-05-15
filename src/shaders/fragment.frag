// !! do not define version in shader the compiler does it for us !!
// This file includes common.glsl during compilation

out vec4 FragColor;

in vec3 worldPos;

uniform int debug_id;

uniform float test_brightness;

uniform int shadow_test_max;

uniform vec3 test_emit_col; // for testing emissive surfaces

uniform int show_only_this_bounce;

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
		FragColor = vec4(test_emit_col, 1.0);
		return;
	}

	// get material from the ssbo
	Material mat = material_buffer[gl_PrimitiveID];
	Lightmap lm = lightmap_buffer[gl_PrimitiveID];

	vec3 mat_col = vec3(mat.color_r, mat.color_g, mat.color_b);
	//vec3 mat_col = lm.diffuseColor.xyz;

	// Assuming depth value is in range [0, 1]
    float depth = gl_FragCoord.z - 0.9f; // Fetch depth from the built-in 

	depth *= 10.0f;

    //FragColor = vec4(mat_col * depth * debug_dot, 1.0);

	int light_idx_offset = NUM_LIGHTS_PER_PRIMITIVE * gl_PrimitiveID;
	vec3 light_col = vec3(0.0);

	for (int light_idx = 0; light_idx < lm.numLights; light_idx++) {
		Light cur_light = lights_buffer[light_idx_offset + light_idx];
		Triangle c_prim = get_primitive(cur_light.casterIndex);

		Material og_mat = material_buffer[cur_light.ogCaster]; // get the original material of the light
		vec3 og_col = vec3(og_mat.color_r, og_mat.color_g, og_mat.color_b);

		// also add the previous distance in the future
		float probability = convolve(worldPos, c_prim, cur_light.prevDist);
		probability *= cur_light.thisIntensity;

		light_col += mat_col * cur_light.tint * test_emit_col * probability;
	}

	// fake shadows
	vec3 light_dir = vec3(0, -1, 0);

	light_col *= test_brightness;
	
	// what % obscured is the currect fragment
	float shadow = 1.0;

	//Triangle temp_caster = get_primitive(978);

//	for (int i = 0; i < shadow_test_max; i++) {
//		shadow += point_in_triangle_shadow(worldPos, light_dir, i) ? 1.0 : 0.0;
//		//shadow *= 1.0 - conic_shadow(worldPos, temp_caster, get_lod_primitive(i));
//	}

	// If any more than one of the potential shadowers block the point then we are shadowed
	shadow = 1 - clamp(shadow, 0.0, 0.5);

	light_col *= shadow;

	FragColor = vec4(light_col, 1.0);
	//FragColor = vec4(mat_col * testval, 1.0);
}