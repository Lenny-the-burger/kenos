// !! THIS FILE IS INCLUDED IN BOTH GLSL AND C++ CODE !!
// !! ALL DEFINITION MUST EITHER BE VALID IN BOTH LANGUAGES, THE GLSL IMPORTER 
// !! DOES NOT SUPPORT CONDITIONAL COMPILATION !!
#define EPSILON 0.0001

#define NUM_LIGHTS_PER_PRIMITIVE 10
#define NUM_SHADOWS_PER_PRIMITIVE 10

// how many adjacent primitives can a primitive have
#define MAX_ADJACENT_PRIMITIVES 3
// how many shared vertices two primitives must have to be considered adjacent.
#define MIN_SHARED_VERTICES 2

#if MIN_SHARED_VERTICES > 3
#error MIN_SHARED_VERTICES should be less than or equal to 3. Triangle only have three vertices at most.
#endif
#if MIN_SHARED_VERTICES < 1
#error MIN_SHARED_VERTICES should be greater than or equal to 1.
#endif

// Minimum dot prod difference between normals of two primitives to be considered adjacent.
#define MIN_NORMAL_DOT_DIFF 0.1