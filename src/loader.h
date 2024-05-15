/*
* The loader class is used to load files. Shader loading is handled by a seperate class, 
* as this is a "dumb" loader and does not do anything apart from reading files and returning
* the contents in various formats.
*/

#pragma once
#include <iostream>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <windows.h>

#include <fstream>
#include <nlohmann/json.hpp>

#include <glm/glm.hpp> // shouldn't be here but it is what it is
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "engine_constants.h"

#include <vector>
#include <map>
#include <set>

// !! Remember to use the constructor and destructor to free memory !!
struct Mesh {
	std::string name;

	// Actually we dont need to use c arrays here since we build a monobuffer
	std::vector<glm::vec3> vertices;
	std::vector<glm::ivec3> indices;
};

// No name because this is stored per primive
struct Material {
	float emissive_strength;
	float color_r; // seperate color channels so we dont have to deal with arrays
	float color_g;
	float color_b;
	float roughness;
};

// Per scene information
struct Scene_information {
	std::string name;
	std::string path;
	std::string description;

	glm::vec3 camera_position;
	glm::vec3 camera_lookat;
	float camera_fov;
};

struct Scene_object {
	int mesh_index;
	int lod_mesh_index;
	int material_index;
	glm::mat4 transform;
	glm::mat4 lod_transform;
	std::string name;
};

// Loads a json scene file and contructs the monobuffer using meshes.
class Loader {

public:
	Loader() {};
	~Loader() {};

	// Load a json scene file !! may take a while !!
	void load_scene(const std::string& filepath);

	// Get the scene information
	Scene_information get_scene_info() { return scene_info; }




	// Get the vertices of the monobuffer
	float* get_vertices() { return all_vertices; }
	int get_num_vertices() { return num_vertices; }

	// Get the indices of the monobuffer
	int* get_indices() { return all_indices; }
	int get_num_indices() { return num_indices; }

	// Get the per primitive material properties
	Material* get_materials() { return all_materials; }
	int get_num_materials() { return num_materials; }

	int get_num_primitives() { return num_materials; } // one material per primitive




	// Get the vertices of the lod monobuffer
	float* get_lod_vertices() { return all_lod_vertices; }
	int get_num_lod_vertices() { return num_lod_vertices; }

	// Get the indices of the lod monobuffer
	int* get_lod_indices() { return all_lod_indices; }
	int get_num_lod_indices() { return num_lod_indices; }

	// Get the per primitive material properties
	Material* get_lod_materials() { return all_lod_materials; }
	int get_num_lod_materials() { return num_lod_materials; }

	int get_num_lod_primitives() { return num_lod_materials; } // one material per primitive



	// Get the adj information
	int* get_adj_information() { return adj_information_final; }
	int get_num_adj_information() { return num_adj_information; }

private:
	std::vector<Mesh> loaded_meshes;
	std::vector<Material> loaded_materials;

	Scene_information scene_info;

	std::vector<Scene_object> scene_objects;

	// Map that goes from mesh name to mesh index
	std::map<std::string, int> mesh_name_to_index;

	// Map that goes from material name to material index
	std::map<std::string, int> material_name_to_index;

	// Monobuffer

	float* all_vertices;
	int num_vertices;

	int* all_indices;
	int num_indices;

	Material* all_materials;
	int num_materials;

	// lod monobuffer

	float* all_lod_vertices;
	int num_lod_vertices;

	int* all_lod_indices;
	int num_lod_indices;

	Material* all_lod_materials;
	int num_lod_materials;

	// Adj information

	int* adj_information_final;
	int num_adj_information;
};