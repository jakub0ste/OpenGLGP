#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"

#include <vector>
#include "imgui.h" 
#include "imgui_impl/imgui_impl_glfw.h"
#include "imgui_impl/imgui_impl_opengl3.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
//void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void generateSierpinskiTetrahedron(std::vector<float>& vertices, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int depth);

bool useColor = false;
glm::vec3 vertexColor(1.0f, 0.0f, 0.0f);

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

glm::vec3 center(0.0f, 0.0f, -0.5f); // Pozycja centrum piramidy


// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

int recursionDepth = 3;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Inicjalizacja ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glEnable(GL_DEPTH_TEST);

    Shader ourShader("res/shaders/basic.vert", "res/shaders/basic.frag"); 

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    std::vector<float> vertices;
    glm::vec3 a(-0.5f, -0.5f, 0.0f);
    glm::vec3 b(0.5f, -0.5f, 0.0f);
    glm::vec3 c(0.0f, 0.5f, -0.5f);
    glm::vec3 d(0.0f, -0.5f, -1.0f);
    int depth = 3; 
    generateSierpinskiTetrahedron(vertices, a, b, c, d, depth);

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // load and create a texture 
    // -------------------------
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture); 
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //flip texture
    stbi_set_flip_vertically_on_load(true);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    unsigned char* data = stbi_load("res/textures/stone.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);

    ourShader.use();
    ourShader.setInt("texture1", 0);

    //glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Tworzenie suwaka ImGui
        ImGui::Begin("Settings");
        ImGui::SliderInt("Recursion Depth", &recursionDepth, 0, 5);
        ImGui::End();

        // ImGui window
        ImGui::Begin("Settings");
        ImGui::Checkbox("Use Color", &useColor);
        ImGui::End();

        ImGui::Begin("Change Colour");
        ImGui::ColorEdit3("Vertex Color", (float*)&vertexColor);
        ImGui::End();

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Aktualizacja poziomu rekurencji
        vertices.clear();
        generateSierpinskiTetrahedron(vertices, a, b, c, d, recursionDepth);

        // Aktualizacja danych w VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

        // activate shader
        ourShader.use();

        ourShader.setBool("useColor", useColor);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        ourShader.setMat4("projection", projection);
        // camera/view transformation
        glm::mat4 view = camera.GetViewMatrix(center);
        ourShader.setMat4("view", view);


        ourShader.setMat4("model", model);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 8);

        // Rendering ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    //don't ask - quick fix
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime, center);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime, center);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime, center);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime, center);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void generateSierpinskiTetrahedron(std::vector<float>& vertices, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int depth) {
    if (depth == 0) {
        // Dodaj wierzcho³ki z kolorem
        vertices.insert(vertices.end(), {
            a.x, a.y, a.z, vertexColor.r, vertexColor.g, vertexColor.b, 1.0f, 0.0f,
            b.x, b.y, b.z, vertexColor.r, vertexColor.g, vertexColor.b, 0.0f, 0.0f,
            c.x, c.y, c.z, vertexColor.r, vertexColor.g, vertexColor.b, 0.5f, 1.0f,

            a.x, a.y, a.z, vertexColor.r, vertexColor.g, vertexColor.b, 1.0f, 0.0f,
            b.x, b.y, b.z, vertexColor.r, vertexColor.g, vertexColor.b, 0.0f, 0.0f,
            d.x, d.y, d.z, vertexColor.r, vertexColor.g, vertexColor.b, 0.5f, 1.0f,

            a.x, a.y, a.z, vertexColor.r, vertexColor.g, vertexColor.b, 1.0f, 0.0f,
            c.x, c.y, c.z, vertexColor.r, vertexColor.g, vertexColor.b, 0.0f, 0.0f,
            d.x, d.y, d.z, vertexColor.r, vertexColor.g, vertexColor.b, 0.5f, 1.0f,

            b.x, b.y, b.z, vertexColor.r, vertexColor.g, vertexColor.b, 1.0f, 0.0f,
            c.x, c.y, c.z, vertexColor.r, vertexColor.g, vertexColor.b, 0.0f, 0.0f,
            d.x, d.y, d.z, vertexColor.r, vertexColor.g, vertexColor.b, 0.5f, 1.0f,
            });
    }
    else {
        glm::vec3 ab = (a + b) / 2.0f;
        glm::vec3 ac = (a + c) / 2.0f;
        glm::vec3 ad = (a + d) / 2.0f;
        glm::vec3 bc = (b + c) / 2.0f;
        glm::vec3 bd = (b + d) / 2.0f;
        glm::vec3 cd = (c + d) / 2.0f;

        generateSierpinskiTetrahedron(vertices, a, ab, ac, ad, depth - 1);
        generateSierpinskiTetrahedron(vertices, ab, b, bc, bd, depth - 1);
        generateSierpinskiTetrahedron(vertices, ac, bc, c, cd, depth - 1);
        generateSierpinskiTetrahedron(vertices, ad, bd, cd, d, depth - 1);
    }
}
