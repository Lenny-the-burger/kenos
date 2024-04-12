#include "loader.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace std;

void Loader::load_scene(const std::string& filepath)
{
	/*
	* 1. Read scene file
	* 2. Fill out the scene info struct
	* 3. Fill out the material structs
	* 4. Load meshes
	* 5. Read and construct scene objects
	* 6. Contruct the monobuffer
	*/

	// 1. Read scene file
	ifstream f(filepath);
	if (!f.good()) {
		string errorMessage = "Could not find '" + filepath + "', file may be mistyped or missing.";
		MessageBoxA(NULL, errorMessage.c_str(), "Fatal error", MB_ICONERROR | MB_OK);
		exit(0);
	}

	json data = json::parse(f);


	// 2. Fill out the scene info struct
	scene_info = Scene_information();

	// Get the basic strings
	scene_info.name = data["sceneName"];
	scene_info.description = data["sceneDescription"];
	scene_info.path = filepath;
	
	// Other scene info
	scene_info.camera_position = glm::vec3(data["camera"]["position"][0], data["camera"]["position"][1], data["camera"]["position"][2]);
	scene_info.camera_lookat = glm::vec3(data["camera"]["lookat"][0], data["camera"]["lookat"][1], data["camera"]["lookat"][2]);
	scene_info.camera_fov = data["camera"]["fov"];

	// 3. Fill out the material structs
	for (auto& material : data["materials"]) {
		Material mat = Material();
		mat.emissive_strength = material["emissiveIntensity"];
		mat.color[0] = material["color"][0];
		mat.color[1] = material["color"][1];
		mat.color[2] = material["color"][2];
		mat.roughness = material["roughness"];

		mat.name = material["name"];

		loaded_materials.push_back(mat);

		// update the material name to index map
		mesh_name_to_index[material["name"]] = loaded_materials.size() - 1;
	}

	// 4. Load meshes
	for (string mesh : data["meshes"]) {

		// check if the mesh file exists
		ifstream f(mesh);
		if (!f.good()) {
			string errorMessage = "Could not find '" + mesh + "', file may be mistyped or missing.";
			MessageBoxA(NULL, errorMessage.c_str(), "Fatal error", MB_ICONERROR | MB_OK);
			exit(0);
		}

		// use AssImp to import mesh

		Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile(mesh, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

		// check if the mesh file is valid
		if (!scene) {
			string errorMessage = "Mesh file '" + mesh + "' is invalid (AssImp error)!";
			MessageBoxA(NULL, errorMessage.c_str(), "Fatal error", MB_ICONERROR | MB_OK);
			exit(0);
		}

		// get the first mesh in the scene
		aiMesh* aiMesh = scene->mMeshes[0];

		// create a new mesh object
		Mesh new_mesh = Mesh();

		// fill out the vertices
		new_mesh.num_vertices = aiMesh->mNumVertices;
		new_mesh.vertices = new float[new_mesh.num_vertices * 3];

		for (int i = 0; i < new_mesh.num_vertices; i++) {
			new_mesh.vertices[i * 3] = aiMesh->mVertices[i].x;
			new_mesh.vertices[i * 3 + 1] = aiMesh->mVertices[i].y;
			new_mesh.vertices[i * 3 + 2] = aiMesh->mVertices[i].z;
		}

		// fill out the indices
		new_mesh.num_indices = aiMesh->mNumFaces * 3;
		new_mesh.indices = new int[new_mesh.num_indices];

		for (int i = 0; i < aiMesh->mNumFaces; i++) {
			new_mesh.indices[i * 3]     = aiMesh->mFaces[i].mIndices[0];
			new_mesh.indices[i * 3 + 1] = aiMesh->mFaces[i].mIndices[1];
			new_mesh.indices[i * 3 + 2] = aiMesh->mFaces[i].mIndices[2];
		}

		new_mesh.name = mesh;

		// add the mesh to the list of loaded meshes
		loaded_meshes.push_back(new_mesh);

		// update the mesh name to index map
		mesh_name_to_index[mesh] = loaded_meshes.size() - 1;
	}

	// 5. Read and construct scene objects
	for (auto& object : data["objects"]) {
		Scene_object new_object = Scene_object();

		new_object.mesh_index = mesh_name_to_index[object["mesh"]];
		new_object.material_index = mesh_name_to_index[object["material"]];
		new_object.name = object["name"];

		// fill out the transform
		glm::vec3 position = glm::vec3(object["position"][0], object["position"][1], object["position"][2]);
		glm::vec3 rotation = glm::vec3(object["rotation"][0], object["rotation"][1], object["rotation"][2]);
		glm::vec3 scale = glm::vec3(object["scale"][0], object["scale"][1], object["scale"][2]);

		new_object.transform = glm::mat4(1.0f);
		new_object.transform = glm::scale(new_object.transform, scale);
		new_object.transform = glm::rotate(new_object.transform, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		new_object.transform = glm::rotate(new_object.transform, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		new_object.transform = glm::rotate(new_object.transform, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		new_object.transform = glm::translate(new_object.transform, position);
		

		scene_objects.push_back(new_object);
	}

	// 6. Contruct the monobuffer
}
