int testfunc(int a, int b) {
	return a + b;
};

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

