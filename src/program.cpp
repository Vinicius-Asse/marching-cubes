#include<core/program.hpp>

    Cube*               wiredCube;

/***
 * Construtor do Programa
**/
Program::Program(Parameters *_param) {
    param = _param;

    onCreate();
}

/***
 * Método Executado Quando o Programa é Criado
**/
void Program::onCreate() {
    Shader* baseShader = new Shader("resources/shaders/base.glsl");

    wiredCube = Cube::getInstance(
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(10.0f, 10.0f, 10.0f),
        baseShader
    );

    start();
}

/***
 * Método Executado Quando o Programa é Iniciado
**/
void Program::start() {
    int countX = param->worldBounds.x * param->pointDencity;
    int countY = param->worldBounds.y * param->pointDencity;
    int countZ = param->worldBounds.z * param->pointDencity;

    unsigned __int64 startTime = Utils::currentTimeInMillis();
    points = instantiatePointsGPU(countX, countY, countZ);
    
    if (param->useGPU) {
        mesh = generateMeshGPU(countX, countY, countZ, new Shader("resources/shaders/base.glsl"));
    } else {
        mesh = generateMesh(countX, countY, countZ, new Shader("resources/shaders/base.glsl"));
    }

    std::cout << "Tempo para gerar mesh: " << Utils::currentTimeInMillis() - startTime << "ms. ";
    std::cout << 
        "{ " << 
                "surfaceLevel: '" << std::to_string(param->surfaceLevel)           << "', " <<
                "smooth: '"       << std::to_string(param->smooth)                 << "', " <<
                "linearInterp: '" << std::to_string(param->linearInterp)           << "', " <<
                // "distX:  '"       << std::to_string(param->noiseDisplacement.x)    << "', " <<
                // "distY: '"        << std::to_string(param->noiseDisplacement.y)    << "', " <<
                // "distZ: '"        << std::to_string(param->noiseDisplacement.z)    << "', " <<
                "GPU: '"          << std::to_string(param->useGPU)                 <<
        " }" << 
        std::endl;
}

/***
 * Método Executado no Inicio de Cada Frame
**/
void Program::input(SDL_Event* e) { }

/***
 * Método Executado Toda Frame
**/
void Program::update() { }

/***
 * Método Executado ao Fim de Toda Frame
**/
void Program::draw() {

    // Turn on wireframe mode
    glPolygonMode(GL_FRONT, GL_LINE);
    glPolygonMode(GL_BACK, GL_LINE);
    glDisable(GL_CULL_FACE);

    // Draw the box
    wiredCube->draw();

    // Turn off wireframe mode
    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);

    // Draw Generated Mesh
    mesh->draw();
}


/***
 * 
 * Métodos Privados para execução do algoritmo Marching Cubes
 * 
**/

Point*** Program::instantiatePoints(int countX, int countY, int countZ) {
    Point*** points = (Point***) malloc(sizeof(Point**) * countX);
    for (int i = 0; i < countX; i++) {
        points[i] = (Point**) malloc(sizeof(Point*) * countY);
        for (int j = 0; j < countY; j++) {
            points[i][j] = (Point*) malloc(sizeof(Point) * countZ);
            for (int k = 0; k < countZ; k++) {
                float x = Utils::remap(i, 0, countX - 1, -5.0f, 5.0f);
                float y = Utils::remap(j, 0, countY - 1, -5.0f, 5.0f);
                float z = Utils::remap(k, 0, countZ - 1, -5.0f, 5.0f);
                points[i][j][k] = {
                    x, y, z,
                    Utils::generateNoise(x, y, z, param->noiseScale, param->noiseDisplacement)
                };
            }
        }
    }

    return points;
}

Point*** Program::instantiatePointsGPU(int countX, int countY, int countZ) {

    const int buffSize = countX * countY * countZ * sizeof(Point);
    
    GLuint vertexSSbo;

    GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

    ComputeShader* compute = new ComputeShader("resources/shaders/points.compute");

    compute->enable();

    glGenBuffers(1, &vertexSSbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexSSbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, buffSize, NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertexSSbo);

    Point* ssboPoints = (Point*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, buffSize, bufMask);

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glDispatchCompute(countX, countY, countZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexSSbo);
    Point* ppt = (Point*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

    Point*** points = (Point***) malloc(sizeof(Point**) * countX); //TODO: Refatorar codigo para utilizar apenas array de pontos
    for (int i = 0; i < countX; i++) {
        points[i] = (Point**) malloc(sizeof(Point*) * countY);
        for (int j = 0; j < countY; j++) {
            points[i][j] = (Point*) malloc(sizeof(Point) * countZ);
            for (int k = 0; k < countZ; k++) {
                int currPos = k + (countY * j) + (countZ * countY * i);
                Point p = ppt[currPos];
                points[i][j][k] = p;
            }
        }
    }

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    compute->disable();

    return points;
}

Mesh* Program::generateMesh(int countX, int countY, int countZ, Shader* shader) {

    std::cout << "Comecou a gerar a Mesh. X = " << countX << " Y = " << countY << " Z = " << countZ << std::endl;

    std::vector<Triangle> triangles;
    
    for (int i = 0; i < countX - 1; i++) {
        for (int j = 0; j < countY - 1; j++) {
            for (int k = 0; k < countZ - 1; k++) {
                Point corners[8] = {
                    points[i  ][j  ][k  ],
                    points[i+1][j  ][k  ],
                    points[i+1][j  ][k+1],
                    points[i  ][j  ][k+1],
                    points[i  ][j+1][k  ],
                    points[i+1][j+1][k  ],
                    points[i+1][j+1][k+1],
                    points[i  ][j+1][k+1] 
                };

                int cubeIndex = 0;
                for (int l = 0; l < 8; l++)
                {
                    if (corners[l].value > param->surfaceLevel)
                    {
                        cubeIndex |= 1 << l;
                    }
                }

                if (cubeIndex == 0 || cubeIndex == 255) continue;

                for (int l = 0; triTable[cubeIndex][l] !=-1; l+=3)
                {
                    int idx = triTable[cubeIndex][l + 0];
                    int idy = triTable[cubeIndex][l + 1];
                    int idz = triTable[cubeIndex][l + 2];

                    int a0 = cornerTupleTable[idx][0];
                    int b0 = cornerTupleTable[idx][1];

                    int a1 = cornerTupleTable[idy][0];
                    int b1 = cornerTupleTable[idy][1];

                    int a2 = cornerTupleTable[idz][0];
                    int b2 = cornerTupleTable[idz][1];

                    glm::vec3 vec0 = interpolate(corners[a0], corners[b0]);
                    glm::vec3 vec1 = interpolate(corners[a1], corners[b1]);
                    glm::vec3 vec2 = interpolate(corners[a2], corners[b2]);
                    glm::vec3 normal = Utils::getNormalVector(vec0, vec1, vec2);

                    triangles.push_back({
                        {vec0.x, vec0.y, vec0.z, 1.0f}, 
                        {vec1.x, vec1.y, vec1.z, 1.0f},
                        {vec2.x, vec2.y, vec2.z, 1.0f},
                        {normal.x, normal.y, normal.z, 1.0f},
                    });
                }
            }
        }
    }

    if (param->smooth) {
        smoothShading(triangles);
    } else {
        flatShading(triangles);
    }

    std::cout << "Terminou de Gerar Mesh. Triangulos: " << triangles.size() << " Vertices: " << vertexBuff.size() << "  Indices: " << indicesBuff.size() << std::endl;

    return new Mesh(indicesBuff, vertexBuff, shader);
}

Mesh* Program::generateMeshGPU(int countX, int countY, int countZ, Shader* shader) {

    std::cout << "Comecou a gerar a Mesh. X = " << countX << " Y = " << countY << " Z = " << countZ << std::endl;

    std::vector<Triangle> triangles;

    const int maxTrizQnt = countX * countY * countZ * 5;
    const int totalPoints = countX * countY * countZ;

    ComputeShader* compute = new ComputeShader("resources/shaders/marching.compute");

    compute->enable();

    // Setting up Atomic Counter Buffer
    GLuint countBuff = 0;
    {
        GLuint zero = 0;
        glGenBuffers(1, &countBuff);
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, countBuff);
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, countBuff);
        glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_STATIC_COPY);
        glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
    }

    // Setting up Triangles SSBO
    GLuint trianglesSSBO;
    {
        glGenBuffers(1, &trianglesSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, trianglesSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Triangle) * maxTrizQnt, NULL, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, trianglesSSBO);

        GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
        Triangle* trianglesBuff = (Triangle*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Triangle) * maxTrizQnt, bufMask);

        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    // Setting up Points SSBO
    GLuint pointsSSBO;
    {
        glGenBuffers(1, &pointsSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointsSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Point) * totalPoints, NULL, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, pointsSSBO);

        GLint  bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
        Point* pointsBuff = (Point*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Point) * totalPoints, bufMask);

        int curr = 0;
        for(int i = 0; i < countX; i++) {
            for(int j = 0; j < countY; j++) {
                for(int k = 0; k < countZ; k++) {
                    pointsBuff[curr++] = points[i][j][k];
                    //LOG(pointsBuff[i].x << " " << pointsBuff[i].y << " " << pointsBuff[i].z << " " );
                }
            }
        }

        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    // Dispatch Compute Shader
    glDispatchCompute(countX, countY, countZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Retrieving Triangles Count
    GLuint trizCount;
    {
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, countBuff);
        GLuint* trizCountPtr = (GLuint*) glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
        trizCount = trizCountPtr[0];
        glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    }

    // Retrieving Triangles Buffer Data
    Triangle* trianglesPtr = nullptr;
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, trianglesSSBO);
        trianglesPtr = (Triangle*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        for (int i = 0; i < trizCount; i++) {
            Triangle t = trianglesPtr[i];
            //LOG("X0 = " << t.ver0.x << " Y0 = " << t.ver0.y << " Z0 = " << t.ver0.z << " X1 = " << t.ver1.x << " Y1 = " << t.ver1.y << " Z1 = " << t.ver1.z << " X2 = " << t.ver2.x << " Y2 = " << t.ver2.y << " Z2 = " << t.ver2.z);
            // triangles.push_back({ 
            //     glm::vec3(t.ver0.x, t.ver0.y, t.ver0.z), 
            //     glm::vec3(t.ver1.x, t.ver1.y, t.ver1.z), 
            //     glm::vec3(t.ver2.x, t.ver2.y, t.ver2.z), 
            //     glm::vec3(t.normal.x, t.normal.y, t.normal.z) 
            // });
            
            //triangles.push_back({ t.ver0, t.ver1, t.ver1, t.normal });

            triangles.push_back(t);
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    // Retrieving Points Buffer Data
    Point* pointsPtr = nullptr;
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointsSSBO);
        pointsPtr = (Point*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        for (int i = 0; i < trizCount; i++) {
            Point p = pointsPtr[i];
            //LOG("X0 = " << p.x << " Y0 = " << p.y << " Z0 = " << p.z << " Value = " << p.value);
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    compute->disable();

    if (param->smooth) {
        smoothShading(triangles);
    } else {
        flatShading(triangles);
    }

    std::cout << "Terminou de Gerar Mesh. Triangulos: " << triangles.size() << " Vertices: " << vertexBuff.size() << "  Indices: " << indicesBuff.size() << std::endl;

    return new Mesh(indicesBuff, vertexBuff, shader);
}

glm::vec3 Program::interpolate(Point a, Point b) {
    glm::vec3 aPos = glm::vec3(a.x, a.y, a.z);
    glm::vec3 bPos = glm::vec3(b.x, b.y, b.z);

    if (param->linearInterp) {
        float t = (param->surfaceLevel - a.value) / (b.value - a.value);
        return aPos + t * (bPos - aPos);
    } else {
        return (aPos + bPos) / 2.0f;
    }
}

void Program::smoothShading(std::vector<Triangle> triangles) {

    vertexBuff.clear();
    indicesBuff.clear();

    GLint curr = 0;
    std::unordered_map<glm::vec3, GLint> map;
    for (int i = 0; i < triangles.size(); i++) {
        Triangle t = triangles[i];

        glm::vec3 v0 = glm::vec3(t.ver0.x, t.ver0.y, t.ver0.z);
        glm::vec3 v1 = glm::vec3(t.ver1.x, t.ver1.y, t.ver1.z);
        glm::vec3 v2 = glm::vec3(t.ver2.x, t.ver2.y, t.ver2.z);
        glm::vec3 normal = glm::vec3(t.ver0.x, t.ver0.y, t.ver0.z);

        if (pushUniqueVertices(&map, v0, normal, curr)) curr++;
        if (pushUniqueVertices(&map, v1, normal, curr)) curr++;
        if (pushUniqueVertices(&map, v2, normal, curr)) curr++;
    }
}

void Program::flatShading(std::vector<Triangle> triangles) {

    vertexBuff.clear();
    indicesBuff.clear();

    for (int i = 0; i < triangles.size(); i++)
    {
        vertexBuff.push_back(Utils::createVertex(triangles[i].ver0, triangles[i].normal));
        vertexBuff.push_back(Utils::createVertex(triangles[i].ver1, triangles[i].normal));
        vertexBuff.push_back(Utils::createVertex(triangles[i].ver2, triangles[i].normal));

        indicesBuff.push_back(i * 3 + 0);
        indicesBuff.push_back(i * 3 + 1);
        indicesBuff.push_back(i * 3 + 2);
    }
}

bool Program::pushUniqueVertices(std::unordered_map<glm::vec3, GLint> *map, glm::vec3 position, glm::vec3 normal, GLint current) {
    if (map->find(position) == map->end()) {
        map->insert(std::make_pair(position, current));
        vertexBuff.push_back(Utils::createVertex({position.x, position.y, position.z, 1.0f}, {normal.x, normal.y, normal.z, 1.0f}));
        indicesBuff.push_back(current);

        return true;
    } else {
        indicesBuff.push_back(map->at(position));
        return false;
    }
}