#include <core/main.hpp>

    SDL_Window      *window;
    SDL_GLContext    context;
 
int main(int argc, char** argv) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) finishError("Nao foi possivel inicializar o SDL");

    setupWindow("Game Window");
    mainLoop();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void setupWindow(const char *title){
    window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    context = SDL_GL_CreateContext(window);
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    if (!window || !context) finishError("Nao foi possivel inicializar a Janela");

    glEnable(GL_DEPTH_TEST);
}

void mainLoop() {
    SDL_Event e;
    bool isRunning = true;

    Shader basicShader("resources/shaders/base.shader");

    Camera mainCamera(
        glm::vec3(0.0f, 0.0f, -5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        45.0f
    );

    Cube cube = Cube::getInstance(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), &basicShader);

    float angle = 0.0f;

    unsigned int tm1 = 0, tm2 = 0, delta = 0;

    float cameraSpeed = 0.1f;
    glm::vec3 inputAxis = glm::vec3(0.0f, 0.0f, 0.0f);

    //Game Loop
    while(isRunning) {
        tm1 = SDL_GetTicks();
        delta = tm1 - tm2;


        if (delta > 1000/60.0) {
            //Handle events on queue
            while(SDL_PollEvent(&e) != 0){
                switch(e.type) {
                case SDL_QUIT:
                    isRunning = false;
                    break;

                case SDL_KEYDOWN:
                    switch(e.key.keysym.sym) {
                        case SDLK_w: inputAxis.z = -1; break;
                        case SDLK_s: inputAxis.z =  1; break;
                        case SDLK_a: inputAxis.x = -1; break;
                        case SDLK_d: inputAxis.x =  1; break;
                        default: break;
                    }
                    break;
                case SDL_KEYUP:
                    switch(e.key.keysym.sym) {
                        case SDLK_ESCAPE: isRunning = false; break;

                        case SDLK_w: if (inputAxis.z < 0) inputAxis.z =  0; break;
                        case SDLK_s: if (inputAxis.z > 0) inputAxis.z =  0; break;
                        case SDLK_a: if (inputAxis.x < 0) inputAxis.x =  0; break;
                        case SDLK_d: if (inputAxis.x > 0) inputAxis.x =  0; break;
                        default: break;
                    }
                    break;
                default:
                    break;
                }
            }

            // Move the Camera according to player's input
            if (glm::length(inputAxis) != 0) {
                mainCamera.translate(cameraSpeed * glm::normalize(inputAxis)); //TODO: Implementar metodo equivalente a Time.DeltaTime (Unity)
            }

            //Clear screen
            glClearColor(0.07f, 0.13f, 0.17f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            cube.rotate(glm::vec3(0.5f, 0.5f, 0.0f));

            if (angle++ > 360) angle = 0;

            // Draw Elements
            cube.draw(mainCamera); //TODO: Singleton MainCamera

            // Swap Front Buffer and Back Buffer
            SDL_GL_SwapWindow(window);

            tm2 = tm1;
        }
    }
}

void finishError(std::string err_msg) {
    std::cout << "Ocorreu um problema durante a execucao do programa: " << err_msg << std::endl;
    fflush(stdout);
    //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", err_msg.c_str(), NULL);
    SDL_Quit();
}