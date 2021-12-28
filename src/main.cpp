#include <core/main.hpp>

    SDL_Window      *window;
    SDL_GLContext    context;
    bool             running;
    const char*      title;

    int              SCREEN_WIDTH  = 1280; 
    int              SCREEN_HEIGHT = 600;
 
int main(int argc, char** argv) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) finishError("Nao foi possivel inicializar o SDL");

    setupWindow("Marching Cubes");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 450");

    mainLoop(io);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void setupWindow(const char *title){

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );

    context = SDL_GL_CreateContext(window);
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    if (!window || !context) finishError("Nao foi possivel inicializar a Janela");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);

    printf("Versao OPENGL: %s\n", glGetString(GL_VERSION));

    SDL_GL_SetSwapInterval(0);
}

void mainLoop(ImGuiIO& io) {
    SDL_Event e;

    Parameters *param = new Parameters();

    bool useCompute      = true;
    bool useCPU          = false;
    bool useMeshShader   = false;
    bool fullscreen      = false;
    bool fixedLight      = false;
    bool lockAspectRatio = true;

    int resolutionMultiplier = param->surfaceResolution / 8.0f;

    float lightDirection[3] = { 0.2f, -0.8f, -0.6f };
    float worldBounds[3] = { param->worldBounds.x, param->worldBounds.y, param->worldBounds.z };

    Camera camera = Camera(
        glm::vec3(0.0f, 0.0f, 20.0f),
        60.0f, window
    );

    Compute compute(param);

    Program program(param);

    if (useCompute) compute.start(); else program.start();

    running = true;

    //GAME LOOP
    while(running) {
        Uint32 startFrame = SDL_GetTicks();

        // Framerate Control 
        TimeDeltaTime = timeControl();

        bool changedMesh = false;

        // Ignoring Inputs if mouse is hovering an IMGUI Element
        ImGui_ImplSDL2_ProcessEvent(&e);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // INPUT HANDLER
        while(SDL_PollEvent(&e) != 0){
            switch(e.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                //case SDL_WINDOWEVENT:
                //    if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                //        int width, height;
                //        SDL_GetWindowSize(window, &width, &height);
                //        glViewport(0, 0, width, height);
                //    }
                //    break;
                //case SDL_KEYDOWN:
                //    switch(e.key.keysym.sym) {
                //        case SDLK_LEFT:  param->noiseDisplacement.x += 0.5f;         changedMesh = true; break;
                //        case SDLK_RIGHT: param->noiseDisplacement.x -= 0.5f;         changedMesh = true; break;
                //        case SDLK_UP:    param->noiseDisplacement.z += 0.5f;         changedMesh = true; break;
                //        case SDLK_DOWN:  param->noiseDisplacement.z -= 0.5f;         changedMesh = true; break;
                //        case SDLK_i:     param->noiseScale *= 2.0f;                  changedMesh = true; break;
                //        case SDLK_o:     param->noiseScale *= 0.5f;                  changedMesh = true; break;
                //        case SDLK_q:     param->surfaceLevel -= 0.1f;                changedMesh = true; break;
                //        case SDLK_e:     param->surfaceLevel += 0.1f;                changedMesh = true; break;
                //        case SDLK_t:     param->surfaceResolution += 8.0f;           changedMesh = true; break;
                //        case SDLK_y:     param->surfaceResolution -= 8.0f;           changedMesh = true; break;
                //        case SDLK_z:     param->smoothIntersect += 0.25f;            changedMesh = true; break;
                //        case SDLK_x:     param->smoothIntersect -= 0.25f;            changedMesh = true; break;
                //        case SDLK_k:     param->smooth       = !param->smooth;       changedMesh = true; break;
                //        case SDLK_j:     param->linearInterp = !param->linearInterp; changedMesh = true; break;
                //        case SDLK_g:     param->useGPU       = !param->useGPU;       changedMesh = true; break;
                //    }
                //    break;
                default:
                    break;
            }

            if (!io.WantCaptureMouse) {
                camera.handleInputs(e);
            }

        }
    
        if (changedMesh) 
            if (useCompute) compute.start(); else program.start();

        // UPDATE
        {
            camera.update();
            if (useCompute) compute.update(); else program.update();
        }

        // DRAW
        {
            //BEGIN DRAW: Clear Screen
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (useCompute) compute.draw(); else program.draw();

            ImGui::Begin("Informações");

            std::string framerate        = "Quadros por Segundo      : " + std::to_string(getFramerate(startFrame));
            std::string trizCount        = "Quantidade de Triangulos : " + compute.meshInfo["trizCount"];
            std::string vertexCount      = "Quantidade de Vertices   : " + compute.meshInfo["vertexCount"];
            std::string indexCount       = "Quantidade de Indices    : " + compute.meshInfo["indexCount"];
            std::string totalTimeGenMesh = "Tempo de Geração (ms)    : " + compute.meshInfo["timeGeneratingMesh"];

            ImGui::Text(framerate.c_str());
            ImGui::Text(trizCount.c_str());
            ImGui::Text(vertexCount.c_str());
            ImGui::Text(indexCount.c_str());
            ImGui::Text(totalTimeGenMesh.c_str());

            ImGui::End();

            ImGui::Begin("Configurações");

            bool remesh = false;

            ImGui::Text("Implementação");
            if (ImGui::Checkbox("CPU", &useCPU)) {
                useCPU = true;
                useCompute = false;
                useMeshShader = false;
            }       
            if (ImGui::Checkbox("Compute Shader", &useCompute)) {
                useCPU = false;
                useCompute = true;
                useMeshShader = false;
            }            
            if (ImGui::Checkbox("Mesh Shader", &useMeshShader)) {
                useCPU = false;
                useCompute = false;
                useMeshShader = true;
            }

            ImGui::Separator();

            if (ImGui::CollapsingHeader("Tela")) {
                ImGui::Checkbox("Tela cheia", &fullscreen);
            }

            ImGui::Separator();

            if (ImGui::CollapsingHeader("Mundo")) {
                remesh ^= ImGui::Checkbox("Iluminação fixa", &fixedLight);
                
                if (fixedLight) {
                    ImGui::Text("Direção da iluminação");
                    remesh ^= ImGui::SliderFloat3("##Direção da iluminação", lightDirection, -90, 90, "%.3f", 1.0f);
                }

                ImGui::Text("Bordas da Simulacao");
                remesh ^= ImGui::DragFloat3("##WorldBounds", worldBounds, .01f, 0.0f, 100.0f, "%.3f", 1.0f);

                ImGui::SameLine();
                remesh ^= ImGui::Checkbox("1:1:1", &lockAspectRatio);
            }

            ImGui::Separator();

            if (ImGui::CollapsingHeader("Geometria")) {
                remesh ^= ImGui::Checkbox("Suavizar Malha", &param->smooth);
                remesh ^= ImGui::Checkbox("Interpolação Linear", &param->linearInterp);

                ImGui::Text("Intencidade da Interpolação");
                remesh ^= ImGui::DragFloat("##Inten. da Interpolação", &param->smoothIntersect, 0.1f, 0.0001f, 10.0f, "%.3f");

                ImGui::Text("Resolução da Malha");
                remesh ^= ImGui::SliderInt("##resolution", &resolutionMultiplier, 1, 20, "%d");

                ImGui::Text("Nivel da Superficie");
                remesh ^= ImGui::SliderFloat("##surfLevel", &param->surfaceLevel, -1.0, 1.0, "%.3f");
            }

            ImGui::End();

            if (remesh) {
                if (lockAspectRatio) {
                    if (param->worldBounds.x != worldBounds[0])
                        worldBounds[2] = worldBounds[1] = worldBounds[0];
                    else if (param->worldBounds.y != worldBounds[1])
                        worldBounds[2] = worldBounds[0] = worldBounds[1];
                    else
                        worldBounds[0] = worldBounds[1] = worldBounds[2];
                }
                
                param->surfaceResolution = 8 * resolutionMultiplier;
                param->worldBounds.x = worldBounds[0];
                param->worldBounds.y = worldBounds[1];
                param->worldBounds.z = worldBounds[2];

                camera.fixedLight = fixedLight;
                if (!fixedLight) {
                    camera.ligthDir.x = lightDirection[0];
                    camera.ligthDir.y = lightDirection[1];
                    camera.ligthDir.z = lightDirection[2];
                }

                if (useCompute) compute.start(); else program.start();
            }

            if (fullscreen) {
                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
            } 
            else {
                SDL_SetWindowFullscreen(window, 0);
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            //END DRAW: Swap Front Buffer and Back Buffer
            SDL_GL_SwapWindow(window);
        }
    }
}

void finishError(std::string err_msg) {
    std::cout << "Ocorreu um problema durante a execucao do programa: " << err_msg << std::endl;
    fflush(stdout);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", err_msg.c_str(), NULL);
    SDL_Quit();
}

double timeControl(){
	static int frames = 0;
	static unsigned int terminoFrame = 0, inicioFrame = 0, timerFrame = 0;
	
	frames ++;
	inicioFrame = SDL_GetTicks();
	double deltaTime = (double)(inicioFrame - terminoFrame) / 100;
	
	if (SDL_TICKS_PASSED(inicioFrame, timerFrame + 1000)){
        std::string title = "Matching Cubes (" + std::to_string(frames) + " FPS)";
        SDL_SetWindowTitle(window, title.c_str());

		frames = 0;
		timerFrame = inicioFrame;
	}
	
	terminoFrame = inicioFrame;
	
	if (deltaTime < 1000 / 400) {
		SDL_Delay(1000 / 400 - deltaTime);
	}

    return deltaTime;
}

int getFramerate(Uint32 startFrame){
	int result = 0;
    Uint32 msec = SDL_GetTicks() - startFrame;

    if (msec > 0)
        result = 1000.0 / (double) msec;

    return result;
}

void GLAPIENTRY MessageCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam )
{
    if (type == GL_DEBUG_TYPE_ERROR) {
        finishError(message);
    } else {
        //std::cout << "[WARNING] " << message << std::endl;
    }
}