#version 460

layout(local_size_x = 1) in;

// Define the layout of the Lightmap struct
struct Lightmap {
    vec3 ambientColor;
    vec3 diffuseColor;
    vec3 specularColor;
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
// Declare the SSBOs
layout(std430, binding = 0) buffer MaterialBuffer {
    Material materials[];
};

layout(std430, binding = 1) buffer LightmapBuffer {
    Lightmap lightmaps[];
};

// Define a buffer to hold indices of vertices
layout(std430, binding = 2) buffer IndexBuffer {
    int indices[];
};

// Define a buffer to hold vertices
layout(std430, binding = 3) buffer VertexBuffer {
    float vertices[];
};

// Function to calculate vertex position from index
vec3 getVertexPosition(int index) {
    int baseIndex = index * 3; // Each vertex has 3 components
    return vec3(vertices[baseIndex], vertices[baseIndex + 1], vertices[baseIndex + 2]);
}

void main() {
    // Get the primitive ID from global invocation ID
    int primitiveID = int(gl_GlobalInvocationID.x);

    // Get indices of vertices for the primitive
    int baseIndex = primitiveID * 3; // Each primitive has 3 vertices
    int vertexIndex1 = indices[baseIndex];
    int vertexIndex2 = indices[baseIndex + 1];
    int vertexIndex3 = indices[baseIndex + 2];

    // Get the vertices themselves
    vec3 vertex1 = getVertexPosition(vertexIndex1);
    vec3 vertex2 = getVertexPosition(vertexIndex2);
    vec3 vertex3 = getVertexPosition(vertexIndex3);

    // Perform computations on the vertices...
    // Example: calculate normal, lighting, etc.

    // Access Material data (example)
    // Material material = materials[primitiveID]; // Uncomment this line if materials are needed

    // Access Lightmap data (example)
    // Lightmap lightmap = lightmaps[primitiveID]; // Uncomment this line if lightmaps are needed

    // Write results to Lightmap buffer
    lightmaps[primitiveID].ambientColor = vec3(0.1, 0.1, 0.1); // Example ambient color
    lightmaps[primitiveID].diffuseColor = vec3(0.5, 0.5, 0.5); // Example diffuse color
    lightmaps[primitiveID].specularColor = vec3(1.0, 1.0, 1.0); // Example specular color
}
