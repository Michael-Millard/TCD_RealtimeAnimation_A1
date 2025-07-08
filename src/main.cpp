#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <my_shader.h>
#include <my_plane_camera.h>
#include <my_model.h>
#include <my_skybox.h>

#include <iostream>
#include <random>
#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Callback function declarations
void frameBufferSizeCallback(GLFWwindow* window, int width, int height);
//void mouseCallback(GLFWwindow* window, double xIn, double yIn);
void scrollCallback(GLFWwindow* window, double xOff, double yOff);
void processUserInput(GLFWwindow* window);
bool imguiMouseUse = false;

// Global params
unsigned int SCREEN_WIDTH = 1920;
unsigned int SCREEN_HEIGHT = 1080;
bool firstMouse = true;
float xPrev = static_cast<float>(SCREEN_WIDTH) / 2.0f;
float yPrev = static_cast<float>(SCREEN_HEIGHT) / 2.0f;

// Timing params
float deltaTime = 0.0f;	// Time between current frame and previous frame
float prevFrame = 0.0f;

// For random placement of models
float generateRandomNumInRange(float low, float high)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(low, high);

    return dis(gen);
}

// 3D model name
// "Spitfire Mk IXe" (https://skfb.ly/6txDP) 
// by martinsifrar is licensed under 
// Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/).
#define PLANE_MODEL "models/spitfire.obj"
#define CLOUD_MODEL "models/cloud.obj"

// Camera specs (set later, can't call functions here)
const float cameraSpeed = 3.0f;
const float cameraZoom = 50.0f;
const float cameraTurnSpeed = 15.0f;
glm::vec3 planePositionInit(0.0f, 0.0f, 0.0f);
glm::vec3 firstPersonOffset(0.0f, 0.75f, -0.5f);
glm::vec3 thirdPersonOffset;
PlaneCamera planeCamera(planePositionInit, firstPersonOffset, thirdPersonOffset, false);

// Main function
int main()
{
    // glfw init and configure
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_DECORATED, NULL); // Remove title bar

    // Screen params
    //GLFWmonitor* MyMonitor = glfwGetPrimaryMonitor(); 
    //const GLFWvidmode* mode = glfwGetVideoMode(MyMonitor);
    //SCREEN_WIDTH = mode->width; SCREEN_HEIGHT = mode->height;

    // glfw window creation
    //GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Realtime Animation Assign1", glfwGetPrimaryMonitor(), nullptr);
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Realtime Animation Assign1", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Callback functions
    glfwSetFramebufferSizeCallback(window, frameBufferSizeCallback);
    //glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // Mouse capture
    if (!imguiMouseUse)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    else
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Load all OpenGL function pointers with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);        // Depth-testing
    glDepthFunc(GL_LESS);           // Smaller value as "closer" for depth-testing
    glEnable(GL_BLEND);             // Enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK); // Default
    glFrontFace(GL_CCW);

    // Build and compile shaders
    Shader planeShader("shaders/vertexShader.vs", "shaders/fragmentShader.fs");
    Shader cloudShader("shaders/cloudVertexShader.vs", "shaders/cloudFragmentShader.fs");
    Shader skyboxShader("shaders/skyboxVertexShader.vs", "shaders/skyboxFragmentShader.fs");

    // Load models
    Model planeModel(PLANE_MODEL);
    Model cloudModel(CLOUD_MODEL);

    // Fine tune planeCamera params
    planeCamera.setCameraMovementSpeed(cameraSpeed);
    planeCamera.setCameraTurnSpeed(cameraTurnSpeed);
    planeCamera.setZoom(false, 50.0f);

    // IMGUI setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Set font
    io.Fonts->Clear();
    ImFont* myFont = io.Fonts->AddFontFromFileTTF(
        "C:\\fonts\\Open_Sans\\static\\OpenSans_Condensed-Regular.ttf", 32.0f);

    // Rebuild the font atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Setup skybox VAO
    GLuint skyboxVAO = setupSkyboxVAO();

    std::vector<std::string> facesCubemap =
    {
        "skybox/right.png",    // px
        "skybox/left.png",     // nx
        "skybox/top.png",      // py
        "skybox/bottom.png",   // ny
        "skybox/front.png",    // pz
        "skybox/back.png"      // nz
    };

    GLuint cubemapTexture = loadCubemap(facesCubemap);

    // Render loop
    float elapsedTime = 0.0f;
    float rotZ = 0.0f;
    float specularExponent = 32.0f;
    float ambientFloat = 0.1f;
    float lightOffsetFloat = 25.0f;
    glm::vec3 ambientLight(ambientFloat, ambientFloat, ambientFloat);
    glm::vec3 lightOffset(lightOffsetFloat, lightOffsetFloat, lightOffsetFloat);
    float lightColour[3] = { 1.0f, 0.35f, 0.25f };
    float cloudAlpha = 0.2f;
    float cloudBlendCoeff = 0.1f;
    while (!glfwWindowShouldClose(window))
    {
        // Per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - prevFrame;
        elapsedTime += deltaTime;
        prevFrame = currentFrame;

        // User input handling
        processUserInput(window);

        // Disable depth test for skybox
        glDisable(GL_DEPTH_TEST);

        // Clear screen colour and buffers
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // IMGUI window
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Skybox
        skyboxShader.use();

        // Remove translation component from the view matrix for the skybox
        glm::mat4 view = glm::mat4(glm::mat3(planeCamera.getViewMatrix()));
        skyboxShader.setMat4("view", view);
        glm::mat4 projection = glm::perspective(glm::radians(planeCamera.zoom),
            static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 0.1f, 1000.0f);
        skyboxShader.setMat4("projection", projection);

        // Bind the skybox texture and render
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        skyboxShader.setInt("skybox", 0);

        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        // Set updated IMGUI params
        ambientLight = glm::vec3(ambientFloat, ambientFloat, ambientFloat);
        lightOffset = glm::vec3(lightOffsetFloat, lightOffsetFloat, lightOffsetFloat);

        // Enable depth test for models
        glEnable(GL_DEPTH_TEST);

        // Draw clouds
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
        cloudShader.use();
        cloudShader.setVec3("lightColour", glm::vec3(lightColour[0], lightColour[1], lightColour[2]));
        cloudShader.setVec3("viewPos", planeCamera.cameraPosition);
        cloudShader.setVec3("lightPos", lightOffset);
        cloudShader.setFloat("blendCoeff", cloudBlendCoeff);
        view = planeCamera.getViewMatrix();
        cloudShader.setMat4("view", view);
        cloudShader.setMat4("projection", projection);
        cloudShader.setFloat("alpha", cloudAlpha);
        glm::mat4 model = glm::identity<glm::mat4>();
        model = glm::translate(model, glm::vec3(0.0f, -50.0f, 0.0f));
        cloudShader.setMat4("model", model);
        cloudModel.draw(cloudShader);
        glDisable(GL_BLEND);

        // Enable shader before setting uniforms
        planeShader.use();
        planeShader.setVec3("ambient", ambientLight);
        planeShader.setFloat("specularExponent", specularExponent);
        planeShader.setVec3("lightColour", glm::vec3(lightColour[0], lightColour[1], lightColour[2]));

        // Camera position
        planeShader.setVec3("viewPos", planeCamera.cameraPosition);
        planeShader.setVec3("lightPos", lightOffset);

        // Model, View & Projection transformations, set uniforms in shader
        view = planeCamera.getViewMatrix();
        planeShader.setMat4("view", view);
        planeShader.setMat4("projection", projection);

        // Rotate the propeller around the z axis at 360 degrees per second
        rotZ += 720.0f * deltaTime;
        rotZ = fmodf(rotZ, 360.0f);

        // Model mat
        model = planeCamera.getPlaneModelMatrix();
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        planeShader.setMat4("model", model);
        planeModel.drawHierarchy(planeShader, model, rotZ);

        // IMGUI drawing
        ImGui::SetNextWindowCollapsed(!imguiMouseUse);
        ImGui::SetNextWindowSize(ImVec2(550, 400));
        ImGui::Begin("Parameter Adjustments");
        ImGui::SliderFloat("Specular Exponent", &specularExponent, 2.0f, 128.0f);
        ImGui::SliderFloat("Ambient light", &ambientFloat, 0.01f, 0.5f);
        ImGui::SliderFloat("Light Offset", &lightOffsetFloat, 1.0f, 25.0f);
        ImGui::SliderFloat("Cloud Alpha", &cloudAlpha, 0.05f, 0.8f);
        ImGui::SliderFloat("Cloud Light Blend", &cloudBlendCoeff, 0.0f, 1.0f);
        ImGui::ColorEdit3("Light Colour", lightColour);
        ImGui::Combo("Camera Type", &planeCamera.selectedCameraType, cameraOptions, IM_ARRAYSIZE(cameraOptions));
        ImGui::Checkbox("MoveFreely", &planeCamera.moveFreely);
        planeCamera.updateCameraType(planeCamera.selectedCameraType);
        ImGui::End();

        // Second window in the top-right corner
        ImVec2 windowSize(250, 360); // Set window size (adjust as needed)
        ImVec2 topRightPos(ImGui::GetIO().DisplaySize.x - windowSize.x - 50, 50); // Offset 10px from edges

        ImGui::SetNextWindowPos(topRightPos, ImGuiCond_Always); // Position window
        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always); // Set size

        ImGui::Begin("Position", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        std::string xPosCam = "x = " + std::to_string(planeCamera.cameraPosition.x);
        std::string yPosCam = "y = " + std::to_string(planeCamera.cameraPosition.y);
        std::string zPosCam = "z = " + std::to_string(planeCamera.cameraPosition.z);
        ImGui::Text("Camera Position:");
        ImGui::Text(xPosCam.c_str());
        ImGui::Text(yPosCam.c_str());
        ImGui::Text(zPosCam.c_str());
        std::string xRotPlane = "Rot X = " + std::to_string(glm::degrees(planeCamera.rotX));
        std::string yRotPlane = "Rot Y = " + std::to_string(glm::degrees(planeCamera.rotY));
        std::string zRotPlane = "Rot Z = " + std::to_string(glm::degrees(planeCamera.rotZ));
        ImGui::Text("Plane Rotation:");
        ImGui::Text(xRotPlane.c_str());
        ImGui::Text(yRotPlane.c_str());
        ImGui::Text(zRotPlane.c_str());
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Shutdown procedure
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Destroy window
    glfwDestroyWindow(window);

    // Terminate and return success
    glfwTerminate();
    return 0;
}

// Process keyboard inputs
bool FKeyReleased = true;
bool IKeyReleased = true; 
void processUserInput(GLFWwindow* window)
{
    // Escape to exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Reset parameters
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        planeCamera.resetAllParams();

    // WS to move forwards/backwards
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        planeCamera.processKeyboardInput(GLFW_KEY_W, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        planeCamera.processKeyboardInput(GLFW_KEY_S, deltaTime);

    // AD for yaw
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        planeCamera.processKeyboardInput(GLFW_KEY_A, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        planeCamera.processKeyboardInput(GLFW_KEY_D, deltaTime);

    // QE for pitch
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        planeCamera.processKeyboardInput(GLFW_KEY_Q, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        planeCamera.processKeyboardInput(GLFW_KEY_E, deltaTime);

    // ZX for roll
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        planeCamera.processKeyboardInput(GLFW_KEY_Z, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        planeCamera.processKeyboardInput(GLFW_KEY_X, deltaTime);

    // 1st/3rd person POV toggle
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && FKeyReleased)
    {
        FKeyReleased = false;
        planeCamera.changeCameraPerspective();
    }

    // Debouncer for F key
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE)
        FKeyReleased = true;

    // Change mouse control between ImGUI and OpenGL
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && IKeyReleased)
    {
        IKeyReleased = false;

        // Give control to ImGUI
        if (!imguiMouseUse)
        {
            imguiMouseUse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Show cursor
        }
        // Give control to OpenGL
        else
        {
            imguiMouseUse = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Hide cursor
        }
    }

    // Debouncer for I key
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_RELEASE)
        IKeyReleased = true;
}

// Window size change callback
void frameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Ensure viewport matches new window dimensions
    glViewport(0, 0, width, height);

    // Adjust screen width and height params that set the aspect ratio in the projection matrix
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;
}

// Mouse scroll wheel input callback - planeCamera zoom must be enabled for this to work
void scrollCallback(GLFWwindow* window, double xOff, double yOff)
{
    // Tell planeCamera to process new y-offset from mouse scroll whell
    planeCamera.processMouseScroll(static_cast<float>(yOff));
}

