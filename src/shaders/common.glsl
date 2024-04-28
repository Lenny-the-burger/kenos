// This is a common library of functions and structs that are used in multiple shaders
// This file is included in all shaders that need it
// !! do not specify version in the shader this is done by the compiler !!

//! ========================= STRUCTS =========================

// Define the layout of the Lightmap struct
struct Lightmap {
    vec4 ambientColor;
    vec4 diffuseColor;
    vec4 specularColor;
    // Add other properties as needed
};

// Define the layout of the Material struct
struct Material {
	float emissive_strength;
	float color_r; // seperate color channels so we dont have to deal with arrays
	float color_g;
	float color_b;
	float roughness;
};

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
};

// ========================= BUFFERS =========================

/**
 * ==== Vertex/Index buffers ====
 */

// Define a buffer to hold vertices
layout(std430, binding = 0) buffer VertexBuffer {
    float vertices[];
};

// Define a buffer to hold indices of vertices
layout(std430, binding = 1) buffer IndexBuffer {
    int indices[];
};

/**
 * ==== SSBOs ====
 */

layout(std430, binding = 2) buffer MaterialBuffer {
	Material material_buffer[];
};

layout(std430, binding = 3) buffer LightmapBuffer {
    Lightmap lightmaps[];
};

// ========================= FUNCTIONS =========================

float plane_sdf(vec3 origin, vec3 normal, vec3 point) {
	return dot(point - origin, normal);
}

bool point_in_triangle(vec3 point, Triangle triangle) {
	return plane_sdf(triangle.v0, triangle.in_norm0, point) <= 0.0 &&
		   plane_sdf(triangle.v1, triangle.in_norm1, point) <= 0.0 &&
		   plane_sdf(triangle.v2, triangle.in_norm2, point) <= 0.0;
}

// Function to calculate vertex position from index
vec3 getVertexPosition(int index) {
    int baseIndex = index * 3; // Each vertex has 3 components
    return vec3(vertices[baseIndex], vertices[baseIndex + 1], vertices[baseIndex + 2]);
}


// Get the primitive at the given index
Triangle get_primitive(int index) {
	int baseIndex = index * 3; // Each primitive has 3 vertices
	int vertexIndex1 = indices[baseIndex];
	int vertexIndex2 = indices[baseIndex + 1];
	int vertexIndex3 = indices[baseIndex + 2];

	Triangle prim;
	prim.v0 = getVertexPosition(vertexIndex1);
	prim.v1 = getVertexPosition(vertexIndex2);
	prim.v2 = getVertexPosition(vertexIndex3);

    prim.mean = (prim.v0 + prim.v1 + prim.v2) / 3.0;
    prim.normal = normalize(cross(prim.v1 - prim.v0, prim.v2 - prim.v0));

	// Calculate interior wall normals
	prim.in_norm0 = normalize(cross(prim.v0 - prim.v1, prim.normal));
	prim.in_norm1 = normalize(cross(prim.v1 - prim.v2, prim.normal));
	prim.in_norm2 = normalize(cross(prim.v2 - prim.v0, prim.normal));

	return prim;
}