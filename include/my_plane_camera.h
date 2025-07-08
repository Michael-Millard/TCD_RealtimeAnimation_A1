#ifndef MY_PLANE_CAMERA_H
#define MY_PLANE_CAMERA_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <vector>

// Constraints on pitch and zoom
const float MIN_PITCH = -89.0f;
const float MAX_PITCH = 89.0f;
const float MIN_ZOOM = 1.0f;
const float MAX_ZOOM = 60.0f;

// Default camera values
const float CAMERA_SPEED = 2.5f;
const float TURN_SPEED = 5.0f;
const float ZOOM = 50.0f; // FOV

// World axes orientation vecs
const glm::vec3 WORLD_ORIGIN(0.0f, 0.0f, 0.0f);
const glm::vec3 WORLD_FRONT(0.0f, 0.0f, -1.0f);
const glm::vec3 WORLD_UP(0.0f, 1.0f, 0.0f);
const glm::vec3 WORLD_RIGHT(1.0f, 0.0f, 0.0f);
const glm::quat QUAT_IDENTITY(1.0f, 0.0f, 0.0f, 0.0f);

// Camera types
const char* cameraOptions[] =
{ 
    "Euler", 
    "F/R/U Vectors", 
    "Quaternions"
};

// Types of rotations
enum
{
    EulerAngles = 0,
    FrontRightUpVecs = 1,
    Quaternions = 2
};

class PlaneCamera
{
public:
    // Camera attributes
    glm::vec3 planePosition;
    glm::vec3 cameraPosition;
    glm::vec3 cameraOffset;
    glm::vec3 firstPersonOffset;
    glm::vec3 thirdPersonOffset;

    // For front, right and up vecs approach
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;

    // For Euler angles (IN RADIANS)
    float rotX;
    float rotY;
    float rotZ;

    // For quaternions
    glm::quat planeOrientation;

    // Selected camera
    int selectedCameraType;

    // Camera params
    float movementSpeed;
    float zoom;
    float turnSpeed;
    bool zoomEnabled;
    bool firstPerson;
    bool moveFreely;

    // Constructor
    PlaneCamera(glm::vec3 planePosition = WORLD_ORIGIN,
        glm::vec3 firstPersonOffset = glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3 thirdPersonOffset = glm::vec3(0.0f, 1.0f, 10.0f),
        bool zoomEnable = true)
        : front(WORLD_FRONT)
        , up(WORLD_UP)
        , movementSpeed(CAMERA_SPEED)
        , turnSpeed(TURN_SPEED)
        , zoom(ZOOM)
        , firstPerson(false)
        , moveFreely(true)
        , selectedCameraType(FrontRightUpVecs)
    {
        this->planePosition = planePosition;
        this->firstPersonOffset = firstPersonOffset;
        this->thirdPersonOffset = thirdPersonOffset;
        this->zoomEnabled = zoomEnable;
        this->right = glm::cross(front, up);
        this->rotX = this->rotY = this->rotZ = 0.0f;

        if (firstPerson)
            this->cameraOffset = firstPersonOffset;
        else
            this->cameraOffset = thirdPersonOffset;

        this->cameraPosition = planePosition + cameraOffset;
        this->planeOrientation = QUAT_IDENTITY;
    }

    void updateCameraType(int camType)
    {
        // Need to set plane orientation for new camera type
        switch (selectedCameraType)
        {
        // If previous type was Euler angles, compute front, right, and up and planeOrientation
        case EulerAngles:
            // Vectors
            front = getRotatedOffset(WORLD_FRONT);
            right = getRotatedOffset(WORLD_RIGHT);
            up = getRotatedOffset(WORLD_UP);

            // Convert each Euler angle into a quaternion rotation
            glm::quat qPitch = glm::angleAxis(rotX, WORLD_RIGHT);
            glm::quat qYaw = glm::angleAxis(rotY, WORLD_UP); 
            glm::quat qRoll = glm::angleAxis(rotZ, WORLD_FRONT); 
            planeOrientation = glm::normalize(qYaw * qPitch * qRoll);
            break;

        // If previous type was front, right, and up, compute Euler angles and planeOrientation
        case FrontRightUpVecs:
            // Euler angles
            rotX = asin(front.y);
            rotY = -atan2(front.x, -front.z);
            rotZ = atan2(right.y, up.y);

            // Convert vecs into a rotation matrix and convert to a quat
            glm::mat4 rotationMatrix = glm::mat4(1.0f);     
            rotationMatrix[0] = glm::vec4(right, 0.0f);     
            rotationMatrix[1] = glm::vec4(up, 0.0f);        
            rotationMatrix[2] = glm::vec4(-front, 0.0f);    
            planeOrientation = glm::toQuat(rotationMatrix);
            break;

        // If previous type quaternions, compute Euler angles and front, right and up vecs
        case Quaternions:
            // Euler angles
            glm::vec3 eulerAngles = glm::eulerAngles(planeOrientation);
            rotX = eulerAngles.x; 
            rotY = eulerAngles.y; 
            rotZ = eulerAngles.z;

            // Vecs
            front = planeOrientation * WORLD_FRONT;
            right = planeOrientation * WORLD_RIGHT;
            up = planeOrientation * WORLD_UP;
            break;

        default:
            break;
        }

        // Set new camera type
        selectedCameraType = camType;
        updateCameraPosition();
    }

    // Set camera movement speed
    void setCameraMovementSpeed(const float newSpeed)
    {
        movementSpeed = newSpeed;
    }

    // Enable/Disable zoom
    void setZoom(const bool enableZoom, const float zoom)
    {
        this->zoomEnabled = enableZoom;
        this->zoom = zoom;
    }

    void setCameraTurnSpeed(const float turnSpeed)
    {
        this->turnSpeed = turnSpeed;
    }

    void resetAllParams()
    {
        rotX = rotY = rotZ = 0.0f;
        planePosition = WORLD_ORIGIN;
        front = WORLD_FRONT;
        up = WORLD_UP;
        right = glm::cross(front, up);
        cameraPosition = planePosition + cameraOffset;
        planeOrientation = QUAT_IDENTITY;
    }
    
    // For Euler angles
    glm::vec3 getRotatedOffset(glm::vec3 offset)
    {
        glm::mat4 rotation = glm::mat4(1.0f);
        rotation = glm::rotate(rotation, rotZ, WORLD_FRONT);  // Roll
        rotation = glm::rotate(rotation, rotY, WORLD_UP);     // Yaw
        rotation = glm::rotate(rotation, rotX, WORLD_RIGHT);  // Pitch
        return glm::vec3(rotation * glm::vec4(offset, 1.0f));
    }

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 getViewMatrix()
    {
        glm::mat4 viewMat = glm::mat4(1);
        if (moveFreely)
        {
            switch (selectedCameraType)
            {
            case EulerAngles:
                viewMat = glm::lookAt(cameraPosition,
                    cameraPosition + getRotatedOffset(WORLD_FRONT),
                    getRotatedOffset(WORLD_UP));
                break;

            case FrontRightUpVecs:
                viewMat = glm::lookAt(cameraPosition,
                    cameraPosition + front,
                    up);
                break;

            case Quaternions:
                viewMat = glm::lookAt(cameraPosition,
                    cameraPosition + planeOrientation * WORLD_FRONT,
                    planeOrientation * WORLD_UP);
                break;

            default:
                break;
            }
        }
        else
            viewMat = glm::lookAt(cameraPosition, planePosition, WORLD_UP);

        return viewMat;
    }

    // Update camera position
    void updateCameraPosition()
    {
        switch (selectedCameraType)
        {
        case EulerAngles:
            if (moveFreely)
            {
                // Recalculate camera position based on rotated offset
                cameraPosition = planePosition + getRotatedOffset(cameraOffset);
            }
            else
            {
                // Else just use 3rd person offset
                cameraPosition = planePosition + thirdPersonOffset;
            }
            break;

        case FrontRightUpVecs:
            if (moveFreely)
            {
                // Recalculate camera position based on front, right, and up vectors
                cameraPosition = planePosition
                    - (front * cameraOffset.z)  // Move camera backward
                    + (up * cameraOffset.y);    // Move camera upward
            }
            else
            {
                // Else just use 3rd person offset
                cameraPosition = planePosition + thirdPersonOffset;
            }
            break;

        case Quaternions:
            if (moveFreely)
            {
                // Recalculate camera position based on the plane’s orientation
                cameraPosition = planePosition + planeOrientation * cameraOffset;
            }
            else
            {
                // Else just use 3rd person offset
                cameraPosition = planePosition + thirdPersonOffset;
            }
            break;

        default:
            break;
        }
    }

    // Change persective (1st/3rd person)
    void changeCameraPerspective()
    {
        // Going to 3rd person
        if (firstPerson)
        {
            cameraOffset = thirdPersonOffset;
            firstPerson = false;
        }
        // Going to 1st person
        else
        {
            cameraOffset = firstPersonOffset;
            firstPerson = true;
        }

        updateCameraPosition();
    }

    // Processes input received from keyboard
    void processKeyboardInput(int key, float deltaTime)
    {
        float velocity = movementSpeed * deltaTime;
        float rotationSpeed = turnSpeed * deltaTime; // Adjust turn speed
        float angle;

        switch (selectedCameraType)
        {
        case EulerAngles:
            switch (key)
            {
            // Yaw Rotation (Left/Right)
            case GLFW_KEY_A:
                rotY += glm::radians(rotationSpeed);
                break;

            case GLFW_KEY_D:
                rotY -= glm::radians(rotationSpeed);
                break;

            // Pitch Rotation (Up/Down)
            case GLFW_KEY_Q:
                rotX += glm::radians(rotationSpeed);
                break;
            case GLFW_KEY_E:
                rotX -= glm::radians(rotationSpeed);
                break;

            // Roll Rotation (CCW/CW)
            case GLFW_KEY_Z:
                rotZ -= glm::radians(rotationSpeed);
                break;
            case GLFW_KEY_X:
                rotZ += glm::radians(rotationSpeed);
                break;

            default:
                break;
            }

            if (moveFreely)
            {
                // Update the camera position based on the rotated camera offset
                cameraPosition = planePosition + getRotatedOffset(cameraOffset);
            }
            break;

        case FrontRightUpVecs:
            switch (key)
            {
            // Forward/Backward
            case GLFW_KEY_W:
                planePosition += front * velocity;
                break;
            case GLFW_KEY_S:
                planePosition -= front * velocity;
                break;

            // Yaw Rotation (Left/Right)
            case GLFW_KEY_A:
                angle = glm::radians(rotationSpeed);
                front = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, up)) * front;
                right = glm::cross(front, up); 
                break;
            case GLFW_KEY_D:
                angle = glm::radians(-rotationSpeed);
                front = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, up)) * front;
                right = glm::cross(front, up);
                break;

            // Pitch Rotation (Up/Down)
            case GLFW_KEY_Q:
                angle = glm::radians(rotationSpeed);
                front = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, right)) * front;
                up = glm::cross(right, front); 
                break;
            case GLFW_KEY_E:
                angle = glm::radians(-rotationSpeed);
                front = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, right)) * front;
                up = glm::cross(right, front);
                break;

            // Roll Rotation (CCW/CW)
            case GLFW_KEY_Z:
                angle = glm::radians(-rotationSpeed);
                up = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, front)) * up;
                right = glm::cross(front, up); 
                break;
            case GLFW_KEY_X:
                angle = glm::radians(rotationSpeed);
                up = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, front)) * up;
                right = glm::cross(front, up); 
                break;

            default:
                break;
            }

            if (moveFreely)
            {
                // Update the camera position based on the plane’s orientation vectors
                cameraPosition = planePosition
                    - (front * cameraOffset.z)
                    + (up * cameraOffset.y);
            }
            break;

        case Quaternions:
            // Create rotation quaternion
            glm::quat rotationQuat;

            switch (key)
            {
            // Move Forward/Backward
            case GLFW_KEY_W:
                planePosition += planeOrientation * WORLD_FRONT * velocity; // Move along forward direction
                break;
            case GLFW_KEY_S:
                planePosition -= planeOrientation * WORLD_FRONT * velocity;
                break;

            // Yaw Rotation (A/D) - Rotate Around Up Axis
            case GLFW_KEY_A:
                angle = glm::radians(rotationSpeed);
                rotationQuat = glm::angleAxis(angle, WORLD_UP); // Rotate around Y (up)
                planeOrientation = glm::normalize(rotationQuat * planeOrientation);
                break;
            case GLFW_KEY_D:
                angle = glm::radians(-rotationSpeed);
                rotationQuat = glm::angleAxis(angle, WORLD_UP); // Rotate around Y (up)
                planeOrientation = glm::normalize(rotationQuat * planeOrientation);
                break;

            // Pitch Rotation (Q/E) - Rotate Around Right Axis
            case GLFW_KEY_Q:
                angle = glm::radians(rotationSpeed);
                rotationQuat = glm::angleAxis(angle, planeOrientation * WORLD_RIGHT); // Rotate around Right
                planeOrientation = glm::normalize(rotationQuat * planeOrientation);
                break;
            case GLFW_KEY_E:
                angle = glm::radians(-rotationSpeed);
                rotationQuat = glm::angleAxis(angle, planeOrientation * WORLD_RIGHT); // Rotate around Right
                planeOrientation = glm::normalize(rotationQuat * planeOrientation);
                break;

            // Roll Rotation (Z/X) - Rotate Around Front Axis
            case GLFW_KEY_Z:
                angle = glm::radians(-rotationSpeed);
                rotationQuat = glm::angleAxis(angle, planeOrientation * WORLD_FRONT); // Rotate around Front
                planeOrientation = glm::normalize(rotationQuat * planeOrientation);
                break;
            case GLFW_KEY_X:
                angle = glm::radians(rotationSpeed);
                rotationQuat = glm::angleAxis(angle, planeOrientation * WORLD_FRONT); // Rotate around Front
                planeOrientation = glm::normalize(rotationQuat * planeOrientation);
                break;

            default:
                break;
            }

            if (moveFreely)
            {
                // Update the camera position based on the plane’s orientation
                cameraPosition = planePosition + planeOrientation * cameraOffset;
            }
            break;

        default:
            break;
        }
    }

    // Processes input received from mouse scroll-wheel
    void processMouseScroll(float yOff)
    {
        // Only if zoom is enabled
        if (zoomEnabled)
        {
            zoom -= yOff;
            // Constrain zoom
            if (zoom < MIN_ZOOM)
                zoom = MIN_ZOOM;
            if (zoom > MAX_ZOOM)
                zoom = MAX_ZOOM;
        }
    }

    // Function to Get the Plane's Model Matrix (For Rendering)
    glm::mat4 getPlaneModelMatrix()
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, planePosition); // Move the plane to its position

        glm::mat4 rotation = glm::mat4(1.0f);
        switch (selectedCameraType)
        {
        case EulerAngles:
            rotation = glm::rotate(rotation, rotZ, WORLD_FRONT);
            rotation = glm::rotate(rotation, rotY, WORLD_UP);
            rotation = glm::rotate(rotation, rotX, WORLD_RIGHT);
            break;

        case FrontRightUpVecs:
            // Align plane’s front direction
            rotation[0] = glm::vec4(right, 0.0f);  // Right
            rotation[1] = glm::vec4(up, 0.0f);     // Up
            rotation[2] = glm::vec4(-front, 0.0f); // Forward
            break;

        case Quaternions:
            rotation = glm::toMat4(planeOrientation); // Convert quaternion to matrix
            break;

        default:
            break;
        }

        // Apply rotation
        model *= rotation; 
        return model;
    }
};

#endif // MY_PLANE_CAMERA_H