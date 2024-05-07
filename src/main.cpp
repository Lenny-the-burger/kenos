// Windows must be included before glad
#include <windows.h> // error message box

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "shader.h"
#include "shader_compute.h"

#include "loader.h"

int WINDOW_WIDTH = 1200;
int WINDOW_HEIGHT = 900;

float PI = 3.14159265359f;

#define NUM_LIGHTS_PER_PRIMITIVE 5

// this can be large because shadows are just stored as single ints
#define NUM_SHADOWS_PER_PRIMITIVE 10

float aspect_ratio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
bool should_update_aspect_ratio = true; // optimization to avoid updating aspect ratio every frame

const char* WINDOW_TITLE = "helo tringl";

std::string SCENE_FILE = "assets/cornell_box.json";

#pragma region IMGUI_VALS
static float updown = -0.2f;
static float FOV = 45.0f;

static int debug_id = 0;
static int debug_id_max = 512;
static float debug_grid_size = 0.05f;

static int convolution_samples = 4; // this should be multiple of 2
static float convolution_distance_mult = 0.8f;
static float convolution_smaple_scale = 0.5f;

static float test_brightness = 0.1f;

static int shadow_test_max = 100;

#pragma endregion

Loader scene_loader = Loader();

// Camera position
static glm::vec3 camera_pos = glm::vec3(0.0f, 1.0f, -5.0f);
static glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
static glm::vec3 camera_lookat = glm::vec3(0.0f, 0.0f, 4.0f);

// Lightmap struct
struct Lightmap {
    int numLights;
    int lightIndex;

    int padding[2];
};

struct Light {
	int casterIndex;
	glm::vec3 color;
	float brightness;

    int numShadows;
    int shadowIndex;

    int padding;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    aspect_ratio = (float)width / (float)height;
    should_update_aspect_ratio = true;
}

void processInput(GLFWwindow* window) {
    // input handling
}

void draw_ui() {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

#ifdef IMGUI_DEBUG
	ImGui::ShowDemoWindow(); // Show demo window! :)
    return;

#else

    bool* p_open = NULL;
    ImGuiWindowFlags window_flags = 0;

    // Main body of the Demo window starts here.
    if (!ImGui::Begin("Options", p_open, window_flags))
    {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }
    ImGui::PushItemWidth(ImGui::GetFontSize() * -12);
    
	// increase default spacing for better readability
	//ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

#pragma region UI
    // UI starts here

    // Debugging

    if (ImGui::CollapsingHeader("Debug")) {
		ImGui::Text("Debugging options");
        ImGui::SliderInt("Debug id", &debug_id, 0, debug_id_max);
        ImGui::SliderFloat("Debug grid size", &debug_grid_size, 0.0f, 1.0f);

        // spacing for better readability
        ImGui::Spacing();
	}

    ImGui::SliderFloat("slider updown", &updown, -5.0f, 5.0f);
    ImGui::SliderFloat("slider FOV", &FOV, 1.0f, 180.0f);

	ImGui::Spacing();

	// Convolution
	ImGui::Text("Convolution options");
	ImGui::SliderInt("Samples", &convolution_samples, 2, 10);
	ImGui::SliderFloat("Distance mult", &convolution_distance_mult, 0.0f, 1.0f);
	ImGui::SliderFloat("Sample scale", &convolution_smaple_scale, 0.0f, 5.0f);

	ImGui::SliderFloat("Test brightness", &test_brightness, 0.0f, 1.0f);

	ImGui::SliderInt("Shadow test max", &shadow_test_max, 0, 968);
    


#pragma endregion
    // End
    ImGui::PopItemWidth();
    ImGui::End();

#endif
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set vsync off
    glfwSwapInterval(0);

    // Load OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();


    // Load the scene
    scene_loader.load_scene(SCENE_FILE);

    Scene_information scene_info = scene_loader.get_scene_info();
    // set the fov
    FOV = scene_info.camera_fov;

    // set debug id max (amount of primitives)
    debug_id_max = (scene_loader.get_num_indices() / 3) - 1;


    // Set the viewport
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    std::vector<std::string> shader_includes = {
        "shaders/common.glsl"
    };

    // Create shaders
    Shader raster_shader("shaders/vertex.vert", "shaders/fragment.frag", shader_includes, 460);
    ComputeShader compute_shader_frame("shaders/compute_frame.comp", shader_includes, 460);

    /* WHAT EACH BUFFER IS USED FOR:
    * 0: Monolithic vertex buffer
    *  This buffer is indexed into by the index buffer and is equal to # of verices * 3 in the 
    *  scene. This buffer is bound normally as a VBO, but also as a SSBO to be acessible in later
    *  stages.
    * 
    * 1: Monolithic index buffer
    *  This buffer stores indeces into the vertex buffer. This buffer is bound normally as a EBO,
    *  but also as a SSBO to be acessible in later stages.
    * 
    * 2: Material buffer
    *  This buffer stores material information for each primitive in the scene. This buffer is
    *  only written to once by the cpu and is then read by several stages. Stores all the information
    *  about the material of the primitive, such as color, reflectivity, etc. This should eventually be
    *  changed to a per-object buffer to save space.
    * 
    * 3: Lightmap buffer
    *   This buffer stores the lightmap for each primitive in the scene. This buffer is written
    *   exclusevly to by the compute shader, so we dont need to initialize it here. This buffer stores
    *   the lighmap of each primitve in the scene, and contains the number of lights on the primitive, 
    *   the shadows, and the index into the lights and shadows buffers.
    * 
    * 4: Lights buffer
    *   This buffer stores all the lights in the scene. This buffer is written exclusevly to by the 
    *   compute shader, so we dont need to initialize it here. This buffer holds light structs which 
    *   describe every bounce of light in the scene. It is not symmetrical and size variable so lightmaps
    *   need to store an integer index and number of lights.
    * 
    * 5: Shadows buffer
    *   This buffer is identical in function to the lights buffer, except that the index information is
    *   stored by lights. This buffer also only holds ints to save space and we dont need that much more
    *   for shadows.
    * 
	* 6: Lod vertex buffer
	*    This buffer stores the vertex data for the lod mesh. This buffer is set up the same way as buffer
    *    0, but is not bound as a VBO.
    * 
	* 7: Lod index buffer
	*    This buffer stores the index data for the lod mesh. This buffer is set up the same way as buffer
	*    1, but is not bound as a EBO.
    */


    // ==================== VERTEX AND INDEX BUFFER ====================

    int num_vertices = scene_loader.get_num_vertices();
    int num_indices = scene_loader.get_num_indices();

    float* vertices = new float[num_vertices];
    int* indices = new int[num_indices];

    vertices = scene_loader.get_vertices();
    indices = scene_loader.get_indices();

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then 
    // configure vertex attributes(s).
    glBindVertexArray(VAO);

    // Bind VBO to GL_ARRAY_BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * num_vertices, vertices, GL_STATIC_DRAW);

    // rebind vertex buffer to be accessed by compute shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, VBO);

    // Copy our index array in a element buffer for OpenGL to use
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * num_indices, indices, GL_STATIC_DRAW);

    // rebind index buffer to be accessed by compute shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, EBO);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the 
    // vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this 
    // VAO, but this rarely happens. Modifying other VAOs requires a call to glBindVertexArray
    // anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // ==================== MATERIAL BUFFER ====================

    // set up material buffer ssbo
    Material* materials;

    materials = scene_loader.get_materials();
    int num_materials = scene_loader.get_num_materials();

    unsigned int material_ssbo;
    glGenBuffers(1, &material_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, material_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Material) * num_materials, materials, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, material_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, material_ssbo); // for now all geometry is static

    // ==================== LIGHTMAP BUFFER ====================

    // We dont need to initilize lightmaps since we will write to them in the compute shader

    int num_lightmaps = scene_loader.get_num_primitives();

    unsigned int lightmap_ssbo;
    glGenBuffers(1, &lightmap_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightmap_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Lightmap) * num_lightmaps, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, lightmap_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightmap_ssbo);

    // ==================== LIGHTS BUFFER ====================

	// We dont need to initilize lights since we will write to them in the compute shader

	int num_lights = scene_loader.get_num_primitives() * NUM_LIGHTS_PER_PRIMITIVE;

	unsigned int lights_ssbo;   
	glGenBuffers(1, &lights_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lights_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Light) * num_lights, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, lights_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lights_ssbo);

	// ==================== SHADOWS BUFFER ====================

	// We dont need to initilize shadows since we will write to them in the compute shader

    // The number of shadows is pretty random in a scene but will generally depend on
    // the number of primitives. 10 times the number of prims doesnt mean that each
	// prim will have 10 shadows, but that overall there will be 10 shadows per prim
	int num_shadows = scene_loader.get_num_primitives() * NUM_SHADOWS_PER_PRIMITIVE;

	unsigned int shadows_ssbo;
	glGenBuffers(1, &shadows_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, shadows_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int)* num_shadows, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, shadows_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, shadows_ssbo);

	// ==================== LOD VERTEX AND INDEX BUFFER ====================
    // I shoud really bind these to locations 2 and 3 but i dont want to change numbers in 3 different
	// places

	int num_lod_vertices = scene_loader.get_num_lod_vertices();
	int num_lod_indices = scene_loader.get_num_lod_indices();

	float* lod_vertices = new float[num_lod_vertices];
	int* lod_indices = new int[num_lod_indices];

	lod_vertices = scene_loader.get_lod_vertices();
	lod_indices = scene_loader.get_lod_indices();

	// Dont need any vbos and ebos just bind them stright to ssbos
	unsigned int lod_verts_ssbo, lod_indices_ssbo;

	glGenBuffers(1, &lod_verts_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lod_verts_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)* num_lod_vertices, lod_vertices, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, lod_verts_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lod_verts_ssbo);

	glGenBuffers(1, &lod_indices_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lod_indices_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int)* num_lod_indices, lod_indices, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, lod_indices_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lod_indices_ssbo);

    // ==================== END BUFFERS SECTION ====================

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // We have to compute the scene first
#pragma region COMPUTE

        int workGroupSize = 64; // ! THIS HAS TO MATCH THE WORK GROUP SIZE IN THE COMPUTE SHADER !

        int num_primitives = scene_loader.get_num_primitives();
        
        compute_shader_frame.use();

        int numWorkGroups = (num_primitives + workGroupSize - 1) / workGroupSize;
        glDispatchCompute(numWorkGroups, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


#pragma endregion

        // Switch from computing to rendering

#pragma region RENDER

        processInput(window);
        draw_ui();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw our first triangle
        raster_shader.use();

        {   // Set the model matrix
            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, glm::vec3(0.0f, updown, 0.0f));

            // rotate around the y axis 180 because i messed up the model
            transform = glm::rotate(transform, PI, glm::vec3(0.0f, 1.0f, 0.0f));

            unsigned int transformLoc = glGetUniformLocation(raster_shader.ID, "model");
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
        }

        {   // Set the view matrix
            glm::mat4 view = glm::mat4(1.0f);
            view = glm::lookAt(camera_pos, camera_lookat, camera_up);

            unsigned int viewLoc = glGetUniformLocation(raster_shader.ID, "view");
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        }

        {   // Set the projection matrix
            glm::mat4 projection = glm::mat4(1.0f);
            projection = glm::perspective(glm::radians(FOV), aspect_ratio, 0.1f, 100.0f);

            unsigned int projectionLoc = glGetUniformLocation(raster_shader.ID, "projection");
            glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        }

        // Check if aspect ratio has changed and update it
        if (should_update_aspect_ratio) {
			// This is handled by the projection matrix so won't be needed until i do
            // optimize that since we set it every frame right now
			should_update_aspect_ratio = false;
		}

        {   // Set the debug id
			unsigned int debugLoc = glGetUniformLocation(raster_shader.ID, "debug_id");
			glUniform1i(debugLoc, debug_id);

            unsigned int debugGridSizeLoc = glGetUniformLocation(raster_shader.ID, "debug_grid_size_uniform");
            glUniform1f(debugGridSizeLoc, debug_grid_size);
        }

        {   // Set misc ui controlled uniforms
			unsigned int convolutionSamplesLoc = glGetUniformLocation(raster_shader.ID, "convolution_samples");
			glUniform1i(convolutionSamplesLoc, convolution_samples);

			unsigned int convolutionDistanceMultLoc = glGetUniformLocation(raster_shader.ID, "convolution_distance_mult");
			glUniform1f(convolutionDistanceMultLoc, convolution_distance_mult);

			unsigned int convolutionSampleScaleLoc = glGetUniformLocation(raster_shader.ID, "convolution_smaple_scale");
			glUniform1f(convolutionSampleScaleLoc, convolution_smaple_scale);

			unsigned int testBrightnessLoc = glGetUniformLocation(raster_shader.ID, "test_brightness");
			glUniform1f(testBrightnessLoc, test_brightness);

			unsigned int shadowTestMaxLoc = glGetUniformLocation(raster_shader.ID, "shadow_test_max"); 
			glUniform1i(shadowTestMaxLoc, shadow_test_max); 
        }

#pragma endregion

        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);
        // glBindVertexArray(0); // no need to unbind it every time 


        // Draw Dear ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}