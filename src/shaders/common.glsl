// This is a common library of functions and structs that are used in multiple shaders
// This file is included in all shaders that need it
// !! do not specify version in the shader this is done by the compiler !!

// ========================= ENGINE CONTANTS =========================

uniform float debug_grid_size_uniform;

#define GRID_SHADER_SIZE 1.0 / debug_grid_size_uniform
#define DEBUG_COLOR vec3(1.0, 0.0, 1.0)

#define EPSILON 0.0001

uniform int KS_NUM_PRIMITIVES;
uniform int KS_NUM_LOD_PRIMITIVES;

uniform float light_min_intensity;
uniform float max_light_distance;

// !! has to be synchronized with the definitionin common.glsl !!
#define NUM_LIGHTS_PER_PRIMITIVE 10

float PI = 3.14159265359;

// ========================= STRUCTS =========================

struct Lightmap {
    int numLights;
    int lightIndex;

    int padding[2];
};

struct Light {
	vec3 tint;
	int casterIndex;

	float prevDist;
	int ogCaster;
	int bounce;

    float thisIntensity;
	float prevIntensity;

	float padding[3];
};

// Define the layout of the Material struct
struct Material {
	float emissive_strength;
	float color_r; // seperate color channels so we dont have to deal with arrays
	float color_g;
	float color_b;
	float roughness;
};

// This is not stored so we can be sloppy with how much data we use
struct Triangle {
	vec3 v0;
	vec3 v1;
	vec3 v2;
	vec3 normal;
	vec3 mean;

	// Interior wall normals
	vec3 in_norm0;
	vec3 in_norm1;
	vec3 in_norm2;

	// Relative axes
	vec3 up;
	vec3 right;
	vec3 left;
};

// ========================= BUFFERS =========================

// ==== Vertex/Index buffers ====

layout(std430, binding = 0) buffer VertexBuffer {
    float vertices[];
};

layout(std430, binding = 1) buffer IndexBuffer {
    int indices[];
};

// ==== SSBOs ====

layout(std430, binding = 2) buffer MaterialBuffer {
	Material material_buffer[];
};

layout(std430, binding = 3) buffer LightmapBuffer {
    Lightmap lightmap_buffer[];
};

layout(std430, binding = 4) buffer LightBuffer {
	Light lights_buffer[];
};

layout(std430, binding = 5) buffer ShadowBuffer {
	int shadows_buffer[];
};

// ==== LOD v/i buffers ====

layout(std430, binding = 6) buffer LODVertexBuffer {
	float lod_vertices[];
};

layout(std430, binding = 7) buffer LODIndexBuffer {
	int lod_indices[];
};


// ========================= FUNCTIONS =========================

// Function to calculate vertex position from index
vec3 getVertexPosition(int index) {
    int baseIndex = index * 3; // Each vertex has 3 components
    return vec3(vertices[baseIndex], vertices[baseIndex + 1], vertices[baseIndex + 2]);
}

// !! THIS FILE SHOULD NOT BE INCLUDED INTO THE VERTEX SHADER !!
uniform mat4 model;

// Get the primitive at the given index !! slow !!
Triangle get_primitive(int index) {
	int baseIndex = index * 3; // Each primitive has 3 vertices
	int vertexIndex1 = indices[baseIndex];
	int vertexIndex2 = indices[baseIndex + 1];
	int vertexIndex3 = indices[baseIndex + 2];

	Triangle prim;
	prim.v0 = getVertexPosition(vertexIndex1);
	prim.v1 = getVertexPosition(vertexIndex2);
	prim.v2 = getVertexPosition(vertexIndex3);

	// Transform the vertices by the model matrix
	// Im sure i can do this in a faster way in the vertex shader but this is fine for now
	prim.v0 = (model * vec4(prim.v0, 1.0)).xyz;
	prim.v1 = (model * vec4(prim.v1, 1.0)).xyz;
	prim.v2 = (model * vec4(prim.v2, 1.0)).xyz;

    prim.mean = (prim.v0 + prim.v1 + prim.v2) / 3.0;
    prim.normal = normalize(cross(prim.v1 - prim.v0, prim.v2 - prim.v0));

	// Calculate interior wall normals
	prim.in_norm0 = normalize(cross(prim.v0 - prim.v1, prim.normal));
	prim.in_norm1 = normalize(cross(prim.v1 - prim.v2, prim.normal));
	prim.in_norm2 = normalize(cross(prim.v2 - prim.v0, prim.normal));

	// Calculate relative axes
	prim.up = prim.normal;
	prim.right = prim.in_norm0;
	prim.left = cross(prim.in_norm0, prim.normal);

	return prim;
}

// o-p might be the other way around
float plane_sdf(vec3 origin, vec3 normal, vec3 point) {
	return dot(origin - point, normal);
}

// im lazy
float plane_sdf(vec3 point, Triangle triangle) {
	return plane_sdf(triangle.mean, triangle.normal, point);
}

// This should be > 0 idk why it only works when it the opposite
bool point_in_triangle(vec3 point, Triangle triangle) {
	return plane_sdf(triangle.v0, triangle.in_norm0, point) <= 0.0 &&
		   plane_sdf(triangle.v1, triangle.in_norm1, point) <= 0.0 &&
		   plane_sdf(triangle.v2, triangle.in_norm2, point) <= 0.0;
}

vec3 project_onto_plane(vec3 point, Triangle triangle) {
	return point + (plane_sdf(triangle.mean, triangle.normal, point) * triangle.normal);
}

vec4 debug_shader(vec3 point, Triangle surface) {
	// convert to local coordinates of triangle
	vec3 point_local = vec3(
		plane_sdf(surface.mean, surface.right, point),
		plane_sdf(surface.mean, surface.left, point),
		plane_sdf(surface.mean, surface.up, point)
	);

	return vec4(DEBUG_COLOR * 
		(sin(point_local.x * GRID_SHADER_SIZE) > cos(point_local.y * GRID_SHADER_SIZE) ? 1.0 : 0.0),
		1.0);
}

// Gaussian approximation for the free ray distribution function of 
// a perfectly lambertian BRDF.
float FRDF_gauss(float smple, float stdev, float mean) {
	return exp(
		-(pow(smple - mean, 2.0) / (2.0 * pow(stdev, 2.0)))
	);
}

// Gaussian approximation for the FRDF but different to make it look more 
// accurate (thanks stole) see: https://www.desmos.com/calculator/8k36ti44zx
float FRDF_gauss_adj(float smple, float stdev, float mean) {
	// What a and a2 here are isnt important they are just random equations
	// that happen to make the function look more accurate.
	float a  = (1.0 / stdev) * (smple - mean);
	float a2 = (pow(abs(a), 3)) / (10.0 + pow(a, 4));

	return FRDF_gauss(smple, stdev, mean) + a2;
}

uniform int convolution_samples; // this should be multiple of 2
uniform float convolution_distance_mult;
uniform float convolution_smaple_scale;

// Convolve the frdf with the rdf of given triangle
float convolve(vec3 point, Triangle caster, float prevdist) {

	int convolution_samples_side = convolution_samples - (convolution_samples/2);

	// This should probably be in the loop
	float frag_dist = convolution_distance_mult * plane_sdf(point, caster);

	frag_dist = sqrt(pow(frag_dist, 2.0) + prevdist);

	vec3 point_flat = project_onto_plane(point, caster);

    // This should ideally just be a monte carlo integration but random samples
	// at very low samples look bad so a square works better in this case
	float total = 0.0;
	for (int xi = -convolution_samples_side; xi < convolution_samples_side; xi++) {
		for (int yi = -convolution_samples_side; yi < convolution_samples_side; yi++) {

			// this used to need to be 2d, but now both the clipping and frdf can be done
			// in 3d mostly thanks to sdfs even though this is a 2d convolution
			vec3 sample_point = caster.mean;
			sample_point += xi * caster.right * convolution_smaple_scale * (1.0 / float(convolution_samples));
			sample_point += yi * caster.left *  convolution_smaple_scale * (1.0 / float(convolution_samples));

			bool in_triangle = point_in_triangle(sample_point, caster);
			if (!in_triangle) {
				// This branch only reduces fps by 1 so whatever
				continue;
			}

			float sample_frdf = FRDF_gauss_adj(distance(sample_point, point_flat), frag_dist, 0.0);
			// Has to be squared since we are doing only one calculation for two directions at once
			sample_frdf = pow(sample_frdf, 2.0);

			// Should be the distance of the sample point instead
			sample_frdf *= 1.0 / pow(frag_dist, 2.0);

			total += sample_frdf;
		}
	}

	// You might think that total should be averaged, but ti shouldnt since ideally we would
	// be able to take lim->inf samples 

	return total;
}

// All the solid angle math is form here: https://www.desmos.com/geometry/gbiivkbluo
// its 2d but i think its the same in 3d

// Calculate the solid angle of a triangle from a point
float solid_angle(vec3 point, Triangle triangle) {
	vec3 v0 = normalize(triangle.v0 - point);
	vec3 v1 = normalize(triangle.v1 - point);
	vec3 v2 = normalize(triangle.v2 - point);
	vec3 vmean = normalize(triangle.mean - point);

	float solid = min(min(dot(vmean, v0), dot(vmean, v1)), dot(vmean, v2));
	solid = 2.0 * acos(solid);

	return solid;
}

// Get the angle of the two normals of two triangles in relation to a point.
// This is used whenever any solid angle math is done with two triangles
// !! this is not exactly that do not use this as the actual angle its fucked up !!
float get_nang(vec3 point, Triangle a, Triangle b) {
	vec3 a2mean =  normalize(a.mean - point);
	vec3 b2mean =  normalize(b.mean - point);

	return 2.0 * acos(dot(a2mean, b2mean));
}

bool do_overlap(vec3 point, Triangle a, Triangle b) {
	float solid_a = solid_angle(point, a);
	float solid_b = solid_angle(point, b);
	float nang = get_nang(point, a, b);

	return nang < solid_a + solid_b;
}

// Compute conic (solid angle) shadows of a caster and a shadower. 
// Returns 0->1 shadowed %
float conic_shadow(vec3 point, Triangle caster, Triangle shadower) {
	float solid_c = solid_angle(point, caster);
	float solid_s = solid_angle(point, shadower);
	float nang = get_nang(point, caster, shadower);

	// Are we in totality or do we need to "shift" the two angles
	float shift_does_matter = nang + solid_s > solid_c ? 1.0 : 0.0;

	float shift_offset = ((nang + solid_s) - solid_c) / 2.0;

	float shadowed = (solid_s - (shift_does_matter * shift_offset)) / solid_c;

	return clamp(shadowed, 0.0, 1.0);
}

// Function to calculate vertex position from index
vec3 getVertexPositionLOD(int index) {
    int baseIndex = index * 3; // Each vertex has 3 components
    return vec3(lod_vertices[baseIndex], lod_vertices[baseIndex + 1], lod_vertices[baseIndex + 2]);
}

// Get the lod primitive at the given index !! slow !!
Triangle get_lod_primitive(int index) {
	int baseIndex = index * 3; // Each primitive has 3 vertices
	int vertexIndex1 = lod_indices[baseIndex];
	int vertexIndex2 = lod_indices[baseIndex + 1];
	int vertexIndex3 = lod_indices[baseIndex + 2];

	Triangle prim;
	prim.v0 = getVertexPositionLOD(vertexIndex1);
	prim.v1 = getVertexPositionLOD(vertexIndex2);
	prim.v2 = getVertexPositionLOD(vertexIndex3);

	// Transform the vertices by the model matrix
	// Im sure i can do this in a faster way in the vertex shader but this is fine for now
	prim.v0 = (model * vec4(prim.v0, 1.0)).xyz;
	prim.v1 = (model * vec4(prim.v1, 1.0)).xyz;
	prim.v2 = (model * vec4(prim.v2, 1.0)).xyz;

	prim.mean = (prim.v0 + prim.v1 + prim.v2) / 3.0;
	prim.normal = normalize(cross(prim.v1 - prim.v0, prim.v2 - prim.v0));

	// Calculate interior wall normals
	prim.in_norm0 = normalize(cross(prim.v0 - prim.v1, prim.normal));
	prim.in_norm1 = normalize(cross(prim.v1 - prim.v2, prim.normal));
	prim.in_norm2 = normalize(cross(prim.v2 - prim.v0, prim.normal));

	// Calculate relative axes
	prim.up = prim.normal;
	prim.right = prim.in_norm0;
	prim.left = cross(prim.in_norm0, prim.normal);

	return prim;
}

// Point in triangle shadow. Gets stuff from buffers by itself so we dont have
// to create an entire Triangle struct every time.
bool point_in_triangle_shadow(vec3 point, vec3 light_norm, int shadow_idx) {
	// get triangle verts
	int baseIndex = shadow_idx * 3; // Each primitive has 3 vertices
	int vertexIndex1 = lod_indices[baseIndex];
	int vertexIndex2 = lod_indices[baseIndex + 1];
	int vertexIndex3 = lod_indices[baseIndex + 2];

	vec3 t0 = getVertexPositionLOD(vertexIndex1);
	vec3 t1 = getVertexPositionLOD(vertexIndex2);
	vec3 t2 = getVertexPositionLOD(vertexIndex3);

	vec3 tnorm = normalize(cross(t1 - t0, t2 - t0));
	
	// backface
	if (dot(tnorm, light_norm) < 0.0) {
		return false;
	}

	// Transform the vertices by the model matrix
	// Im sure i can do this in a faster way in the vertex shader but this is fine for now
	t0 = (model * vec4(t0, 1.0)).xyz;
	t1 = (model * vec4(t1, 1.0)).xyz;
	t2 = (model * vec4(t2, 1.0)).xyz;

	// Turns out we dont even need to normalize these
	vec3 in_norm0 = cross(t0 - t1, light_norm);
	vec3 in_norm1 = cross(t1 - t2, light_norm);
	vec3 in_norm2 = cross(t2 - t0, light_norm);

	float d0 = plane_sdf(t0, in_norm0, point);
	float d1 = plane_sdf(t1, in_norm1, point);
	float d2 = plane_sdf(t2, in_norm2, point);

	// Make sure we are above the triangle
	float d4 = plane_sdf(t0, tnorm, point);

	return  d0 <= EPSILON && 
			d1 <= EPSILON && 
			d2 <= EPSILON && 
			d4 <= EPSILON;
}