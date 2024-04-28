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

float aspect_ratio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
bool should_update_aspect_ratio = true; // optimization to avoid updating aspect ratio every frame

const char* WINDOW_TITLE = "helo tringl";

std::string SCENE_FILE = "assets/cornell_box.json";

#pragma region IMGUI_VALS
static float updown = -0.2f;
static float FOV = 45.0f;
static int debug_id = 0;
static int debug_id_max = 512;

#pragma endregion

Loader scene_loader = Loader();

// Camera position
static glm::vec3 camera_pos = glm::vec3(0.0f, 1.0f, -5.0f);
static glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
static glm::vec3 camera_lookat = glm::vec3(0.0f, 0.0f, 4.0f);

// Lightmap struct
struct Lightmap {
    glm::vec4 ambientColor;
    glm::vec4 diffuseColor;
    glm::vec4 specularColor;
    // Add other properties as needed
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

#pragma region UI
    // UI starts here

    ImGui::SliderFloat("slider updown", &updown, -5.0f, 5.0f);
    ImGui::SliderFloat("slider FOV", &FOV, 1.0f, 180.0f);
    ImGui::SliderInt("Debug id", &debug_id, 0, debug_id_max);


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
    debug_id_max = scene_loader.get_num_indices() / 3;


    // Set the viewport
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    std::vector<std::string> shader_includes = {
        "shaders/common.glsl"
    };

    // Create shaders
    Shader raster_shader("shaders/vertex.vert", "shaders/fragment.frag", shader_includes, 460);
    ComputeShader compute_shader_frame("shaders/compute_frame.comp", shader_includes, 460);

    // set up index and vertex buffers
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


    // We dont need to initilize lightmaps since we will write to them in the compute shader

    int num_lightmaps = scene_loader.get_num_primitives();

    unsigned int lightmap_ssbo;
    glGenBuffers(1, &lightmap_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightmap_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Lightmap) * num_lightmaps, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, lightmap_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightmap_ssbo); // for now all geometry is static

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // We have to compute the scene first
#pragma region COMPUTE

        int workGroupSize = 64; // !! THIS HAS TO MATCH THE WORK GROUP SIZE IN THE COMPUTE SHADER !!

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