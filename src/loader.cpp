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
		mat.color_r = material["albedo"][0] / 255.0f; // these should be clamped to 0-1
		mat.color_g = material["albedo"][1] / 255.0f;
		mat.color_b = material["albedo"][2] / 255.0f;
		mat.roughness = material["roughness"];

		loaded_materials.push_back(mat);

		// update the material name to index map
		material_name_to_index[material["name"]] = loaded_materials.size() - 1;
	}

	// 4. Load meshes
	// this also loads lodMeshes like regular meshes. The only difference is how scene objects
	// refernce them.
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

		const aiScene* scene = importer.ReadFile(mesh, aiProcess_Triangulate);

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
		for (int i = 0; i < aiMesh->mNumVertices; i++) {
			new_mesh.vertices.push_back(glm::vec3(
				aiMesh->mVertices[i].x,
				aiMesh->mVertices[i].y,
				aiMesh->mVertices[i].z));
		}

		// fill out the indices
		for (int i = 0; i < aiMesh->mNumFaces; i++) {
			new_mesh.indices.push_back(glm::ivec3(
				aiMesh->mFaces[i].mIndices[0],
				aiMesh->mFaces[i].mIndices[1],
				aiMesh->mFaces[i].mIndices[2]));
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
		new_object.transform = glm::translate(new_object.transform, position);
		new_object.transform = glm::rotate(new_object.transform, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		new_object.transform = glm::rotate(new_object.transform, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		new_object.transform = glm::rotate(new_object.transform, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		new_object.transform = glm::scale(new_object.transform, scale);
		
		// see if the object has a lodMesh and set it
		if (object.find("lodMesh") != object.end()) {
			new_object.lod_mesh_index = mesh_name_to_index[object["lodMesh"]];

			// read and construct lod transform
			glm::vec3 lod_rotation = glm::vec3(object["lodRotation"][0], object["lodRotation"][1], object["lodRotation"][2]);
			glm::vec3 lod_scale = glm::vec3(object["lodScale"][0], object["lodScale"][1], object["lodScale"][2]);
			glm::vec3 lod_position = glm::vec3(object["lodPosition"][0], object["lodPosition"][1], object["lodPosition"][2]);

			// lod position is parented to the regular position
			lod_position += position;

			new_object.lod_transform = glm::mat4(1.0f);
			new_object.lod_transform = glm::translate(new_object.lod_transform, lod_position);
			new_object.lod_transform = glm::rotate(new_object.lod_transform, lod_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
			new_object.lod_transform = glm::rotate(new_object.lod_transform, lod_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
			new_object.lod_transform = glm::rotate(new_object.lod_transform, lod_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
			new_object.lod_transform = glm::scale(new_object.lod_transform, lod_scale);
		}
		else {
			// else set it to the same as the regular mesh
			new_object.lod_mesh_index = new_object.mesh_index;

			// transform same as regular mesh
			new_object.lod_transform = new_object.transform;
		}

		scene_objects.push_back(new_object);
	}

	// 6. Contruct the monobuffer
	// Loop over all the scene objects and add thew meshes to the vertex and index buffers
	num_vertices = 0;
	num_indices = 0;
	num_materials = 0;

	// Temporary vectors to store stuff, these are later converted to c arrays
	vector<glm::vec3> temp_vertices;
	vector<glm::ivec3> temp_indices;
	vector<Material> temp_materials;

	for (Scene_object& object : scene_objects) {
		// can probably use insert() here but we need to modify the vertices and build materials
		// so probably not

		// append all the vertices to the vertex buffer
		for (int i = 0; i < loaded_meshes[object.mesh_index].vertices.size(); i++) {
			// vertex has to be transformed before adding to the buffer
			glm::vec4 vertex = glm::vec4(loaded_meshes[object.mesh_index].vertices[i], 1.0f);

			vertex = object.transform * vertex;

			temp_vertices.push_back(glm::vec3(vertex.x, vertex.y, vertex.z));
		}

		// Go through ech tri, add the vertex indices and create a per primitive material
		// We could avoid storing stuff like material per primitive if we had some sort of
		// "range hash map" that makes it easy to go from primid to object id, but i dont
		// know of any better way than just storing a 1:1 look up table so it wont save that
		// much mem anyway. Until there is a better way we do this boowomp

		// Div the number by 3 because we want to iterate per triangle
		for (int i = 0; i < loaded_meshes[object.mesh_index].indices.size(); i++) {
			// When we add indeces, they should be offset by the previous number of vertices
			temp_indices.push_back(loaded_meshes[object.mesh_index].indices[i] + glm::ivec3(num_vertices));

			// Add the material
			temp_materials.push_back(loaded_materials[object.material_index]);
		}

		// These are updated after the object are processed as during the loop we assume these
		// represent the numbers of completed 
		num_vertices  = temp_vertices.size();
		num_indices = temp_indices.size();
		num_materials = temp_materials.size();
	}

	// Multiply verts and indxs by 3 since we store them as vec3 and ivec3
	num_vertices *= 3;
	num_indices *= 3;

	// Convert the vectors to c arrays
	all_vertices  = new float[num_vertices];
	all_indices   = new int[num_indices];
	all_materials = new Material[num_materials];

	for (int i = 0; i < temp_vertices.size(); i++) {
		all_vertices[i * 3 + 0] = temp_vertices[i].x;
		all_vertices[i * 3 + 1] = temp_vertices[i].y;
		all_vertices[i * 3 + 2] = temp_vertices[i].z;
	}

	for (int i = 0; i < temp_indices.size(); i++) {
		all_indices[i * 3 + 0] = temp_indices[i].x;
		all_indices[i * 3 + 1] = temp_indices[i].y;
		all_indices[i * 3 + 2] = temp_indices[i].z;
	}

	for (int i = 0; i < temp_materials.size(); i++) {
		all_materials[i] = temp_materials[i];
	}


	// 7. now do the same thing but for the lod meshes
	// This is a bit of a copy paste but we only call it twice with different names each time
	// so it should be fine
	// Loop over all the scene objects and add thew meshes to the vertex and index buffers
	num_lod_vertices = 0;
	num_lod_indices = 0;
	num_lod_materials = 0;

	// Temporary vectors to store stuff, these are later converted to c arrays
	vector<glm::vec3> temp_lod_vertices;
	vector<glm::ivec3> temp_lod_indices;
	vector<Material> temp_lod_materials;

	for (Scene_object& object : scene_objects) {
		// can probably use insert() here but we need to modify the vertices and build materials
		// so probably not

		// append all the vertices to the vertex buffer
		for (int i = 0; i < loaded_meshes[object.lod_mesh_index].vertices.size(); i++) {
			// vertex has to be transformed before adding to the buffer
			glm::vec4 vertex = glm::vec4(loaded_meshes[object.lod_mesh_index].vertices[i], 1.0f);

			vertex = object.lod_transform * vertex;

			temp_lod_vertices.push_back(glm::vec3(vertex.x, vertex.y, vertex.z));
		}

		// Go through ech tri, add the vertex indices and create a per primitive material
		// We could avoid storing stuff like material per primitive if we had some sort of
		// "range hash map" that makes it easy to go from primid to object id, but i dont
		// know of any better way than just storing a 1:1 look up table so it wont save that
		// much mem anyway. Until there is a better way we do this boowomp

		// Div the number by 3 because we want to iterate per triangle
		for (int i = 0; i < loaded_meshes[object.lod_mesh_index].indices.size(); i++) {
			// When we add indeces, they should be offset by the previous number of vertices
			temp_lod_indices.push_back(loaded_meshes[object.lod_mesh_index].indices[i] + glm::ivec3(num_lod_vertices));

			// Add the material
			temp_lod_materials.push_back(loaded_materials[object.material_index]);
		}

		// These are updated after the object are processed as during the loop we assume these
		// represent the numbers of completed 
		num_lod_vertices = temp_lod_vertices.size();
		num_lod_indices = temp_lod_indices.size();
		num_lod_materials = temp_lod_materials.size();
	}

	// Multiply verts and indxs by 3 since we store them as vec3 and ivec3
	num_lod_vertices *= 3;
	num_lod_indices *= 3;

	// Convert the vectors to c arrays
	all_lod_vertices = new float[num_lod_vertices];
	all_lod_indices = new int[num_lod_indices];
	all_lod_materials = new Material[num_lod_materials];

	for (int i = 0; i < temp_lod_vertices.size(); i++) {
		all_lod_vertices[i * 3 + 0] = temp_lod_vertices[i].x;
		all_lod_vertices[i * 3 + 1] = temp_lod_vertices[i].y;
		all_lod_vertices[i * 3 + 2] = temp_lod_vertices[i].z;
	}

	for (int i = 0; i < temp_lod_indices.size(); i++) {
		all_lod_indices[i * 3 + 0] = temp_lod_indices[i].x;
		all_lod_indices[i * 3 + 1] = temp_lod_indices[i].y;
		all_lod_indices[i * 3 + 2] = temp_lod_indices[i].z;
	}

	for (int i = 0; i < temp_lod_materials.size(); i++) {
		all_lod_materials[i] = temp_lod_materials[i];
	}

}
