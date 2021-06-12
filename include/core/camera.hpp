#pragma once

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/perpendicular.hpp>

class Camera {
private:
    glm::vec3 position;

    glm::vec3 up          = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 orientation = glm::vec3(0.0f, 0.0f, -1.0f);

    glm::vec2 inputAxis   = glm::vec3(0.0f, 0.0f, 0.0f);

    float moveSpeed;
    bool mouseEnabled = false;

public:

    Camera(glm::vec3 position, float fov);
    //~Camera();

    void update(SDL_Window *window);
    void handleInputs(SDL_Event e, SDL_Window *window);

    glm::mat4 getViewProjectionMatrix();
    glm::mat4 getMVPMatrix(glm::mat4 model);
};