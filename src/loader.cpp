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
		mat.color_r = material["albedo"][0];
		mat.color_g = material["albedo"][1];
		mat.color_b = material["albedo"][2];
		mat.roughness = material["roughness"];

		loaded_materials.push_back(mat);

		// update the material name to index map
		material_name_to_index[material["name"]] = loaded_materials.size() - 1;
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
		new_mesh.indices = new int[new_mesh.num_indices + 3];

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
		new_object.material_index = material_name_to_index[object["material"]];
		new_object.name = object["name"];

		// fill out the transform
		glm::vec3 position = glm::vec3(object["position"][0], object["position"][1], object["position"][2]);
		glm::vec3 rotation = glm::vec3(object["rotation"][0], object["rotation"][1], object["rotation"][2]);
		glm::vec3 scale = glm::vec3(object["scale"][0], object["scale"][1], object["scale"][2]);

		new_object.transform = glm::mat4(1.0f);
		new_object.transform = glm::scale(new_object.transform, scale);
		//new_object.transform = glm::rotate(new_object.transform, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		//new_object.transform = glm::rotate(new_object.transform, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		//new_object.transform = glm::rotate(new_object.transform, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		new_object.transform = glm::translate(new_object.transform, position);
		

		scene_objects.push_back(new_object);
	}

	// 6. Contruct the monobuffer
	// Loop over all the scene objects and add thew meshes to the vertex and index buffers
	num_vertices = 0;
	num_indices = 0;
	num_materials = 0;

	// Temporary vectors to store stuff, these are later converted to c arrays
	vector<float> temp_vertices;
	vector<int> temp_indices;
	vector<Material> temp_materials;

	for (Scene_object& object : scene_objects) {

		// append all the vertices to the vertex buffer
		for (int i = 0; i < loaded_meshes[object.mesh_index].num_vertices; i++) {
			// vertex has to be transformed before adding to the buffer
			glm::vec4 vertex = glm::vec4(loaded_meshes[object.mesh_index].vertices[i * 3],
								loaded_meshes[object.mesh_index].vertices[i * 3 + 1],
								loaded_meshes[object.mesh_index].vertices[i * 3 + 2], 1.0f);

			vertex = object.transform * vertex;

			temp_vertices.push_back(vertex.x);
			temp_vertices.push_back(vertex.y);
			temp_vertices.push_back(vertex.z);
		}

		// Go through ech tri, add the vertex indices and create a per primitive material
		// We could avoid storing stuff like material per primitive if we had some sort of
		// "range hash map" that makes it easy to go from primid to object id, but i dont
		// know of any better way than just storing a 1:1 look up table so it wont save that
		// much mem anyway. Until there is a better way we do this boowomp

		// Div the number by 3 because we want to iterate per triangle
		for (int i = 0; i < loaded_meshes[object.mesh_index].num_indices / 3; i++) {
			// When we add indeces, they should be offset by the previous number of vertices
			temp_indices.push_back(loaded_meshes[object.mesh_index].indices[i * 3    ] + num_vertices);
			temp_indices.push_back(loaded_meshes[object.mesh_index].indices[i * 3 + 1] + num_vertices);
			temp_indices.push_back(loaded_meshes[object.mesh_index].indices[i * 3 + 2] + num_vertices);

			// Add the material
			temp_materials.push_back(loaded_materials[object.material_index]);
		}

		// These are updated after the object are processed as during the loop we assume these
		// represent the numbers of completed work
		num_indices   = temp_indices.size();
		num_vertices  = temp_vertices.size();
		num_materials = temp_materials.size();
	}

	// Convert the vectors to c arrays
	all_vertices  = new float[temp_vertices.size()];
	all_indices   = new int[temp_indices.size()];
	all_materials = new Material[temp_materials.size()];

	for (int i = 0; i < temp_vertices.size(); i++) {
		all_vertices[i] = temp_vertices[i];
	}

	for (int i = 0; i < temp_indices.size(); i++) {
		all_indices[i] = temp_indices[i];
	}

	for (int i = 0; i < temp_materials.size(); i++) {
		all_materials[i] = temp_materials[i];
	}
}
