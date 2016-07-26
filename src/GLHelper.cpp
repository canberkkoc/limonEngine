//
// Created by Engin Manap on 10.02.2016.
//

#include "GLHelper.h"


GLuint GLHelper::createShader(GLenum eShaderType, const std::string &strShaderFile) {
    GLuint shader = glCreateShader(eShaderType);
    std::string shaderCode;
    std::ifstream shaderStream(strShaderFile.c_str(), std::ios::in);

    if (shaderStream.is_open()) {
        std::string Line = "";

        while (getline(shaderStream, Line))
            shaderCode += "\n" + Line;

        shaderStream.close();
    } else {
        std::cerr << strShaderFile.c_str() <<
        " could not be read. Please ensure run directory if you used relative paths." << std::endl;
        getchar();
        return 0;
    }

    const char *shaderCodePtr = shaderCode.c_str();
    glShaderSource(shader, 1, &shaderCodePtr, NULL);

    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE) {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);
        const char *strShaderType = NULL;

        switch (eShaderType) {
            case GL_VERTEX_SHADER:
                strShaderType = "vertex";
                break;

            case GL_GEOMETRY_SHADER:
                strShaderType = "geometry";
                break;

            case GL_FRAGMENT_SHADER:
                strShaderType = "fragment";
                break;
        }

        std::cerr << strShaderType << " type shader " << strShaderFile.c_str() << " could not be compiled:\n" <<
        strInfoLog << std::endl;
        delete[] strInfoLog;

    }
    checkErrors("createShader");
    return shader;
}


GLuint GLHelper::createProgram(const std::vector<GLuint> &shaderList) {
    GLuint program = glCreateProgram();

    for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++) {
        glAttachShader(program, shaderList[iLoop]);
    }

    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE) {
        GLint infoLogLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

        GLchar *strInfoLog = new GLchar[infoLogLength + 1];
        glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
        std::cerr << "Linking failed: \n" << strInfoLog << std::endl;
        delete[] strInfoLog;
    } else {
        std::cout << "Program compiled successfully" << std::endl;
    }

    for (size_t iLoop = 0; iLoop < shaderList.size(); iLoop++) {
        glDetachShader(program, shaderList[iLoop]);
    }

    checkErrors("createProgram");
    return program;
}


GLuint GLHelper::initializeProgram(std::string vertexShaderFile, std::string fragmentShaderFile, std::map<std::string, Uniform*> &uniformMap) {
    GLuint program;
    std::vector<GLuint> shaderList;
    checkErrors("before create shaders");
    shaderList.push_back(createShader(GL_VERTEX_SHADER, vertexShaderFile));
    shaderList.push_back(createShader(GL_FRAGMENT_SHADER, fragmentShaderFile));


    program = createProgram(shaderList);
    std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);

    fillUniformMap(program, uniformMap);
    attachUBOs(program);

    checkErrors("initializeProgram");
    return program;
}

void GLHelper::fillUniformMap(const GLuint program, std::map<std::string, GLHelper::Uniform *> &uniformMap) const {
    GLint i;
    GLint count;

    GLint size; // size of the variable
    GLenum type; // type of the variable (float, vec3 or mat4, etc)

    GLint maxLength;

    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);

    GLchar name[maxLength]; // variable name in GLSL
    GLsizei length; // name length

    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
    //std::cout << "Active Uniforms:" << count << std::endl;


    for (i = 0; i < count; i++)
    {
        glGetActiveUniform(program, (GLuint)i, maxLength, &length, &size, &type, name);

        //std::cout << "Uniform " << i << " Type: " << type << " Name: " << name << std::endl;
        uniformMap[name] = new Uniform(i, name, type, size);
    }
}

void GLHelper::attachUBOs(const GLuint program) const {//Attach the light block to our UBO
    GLuint lightAttachPoint = 0, playerAttachPoint = 1;

    int uniformIndex = glGetUniformBlockIndex(program, "LightSourceBlock");
    if (uniformIndex >= 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
        glUniformBlockBinding(program, uniformIndex, lightAttachPoint);
        glBindBufferRange(GL_UNIFORM_BUFFER, lightAttachPoint, lightUBOLocation, 0,
                          (sizeof(glm::mat4) + 2 * sizeof(glm::vec4)) *
                          NR_POINT_LIGHTS); //FIXME calculating the size should not be like that
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    int uniformIndex2 = glGetUniformBlockIndex(program, "PlayerTransformBlock");
    if (uniformIndex2 >= 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, playerUBOLocation);
        glUniformBlockBinding(program, uniformIndex2, playerAttachPoint);
        glBindBufferRange(GL_UNIFORM_BUFFER, playerAttachPoint, playerUBOLocation, 0,
                          3 * sizeof(glm::mat4));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }


}


GLHelper::GLHelper() {
    GLenum rev;
    error = GL_NO_ERROR;
    glewExperimental = GL_TRUE;
    rev = glewInit();

    if (GLEW_OK != rev) {
        std::cout << "GLEW init Error: " << glewGetErrorString(rev) << std::endl;
        exit(1);
    } else {
        std::cout << "GLEW Init: Success!" << std::endl;
    }
    checkErrors("after Context creation");

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureImageUnits);

    std::cout << "Maximum number of texture image units is " << maxTextureImageUnits << std::endl;
    state = new OpenglState(maxTextureImageUnits);

    cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraMatrix = glm::lookAt(cameraPosition, glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    lightProjection = glm::ortho(lightOrthogonalProjectionValues.x,
                                 lightOrthogonalProjectionValues.y,
                                 lightOrthogonalProjectionValues.z,
                                 lightOrthogonalProjectionValues.w,
                                 lightOrthogonalProjectionNearPlane, lightOrthogonalProjectionFarPlane);


    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    // Setup
    //glDisable(GL_CULL_FACE);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDepthRange(0.0f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);



    std::printf("%s\n%s\n",
                glGetString(GL_RENDERER),  // e.g. Intel HD Graphics 3000 OpenGL Engine
                glGetString(GL_VERSION)    // e.g. 3.2 INTEL-8.0.61
    );

    printf("Supported GLSL version is %s.\n", (char *) glGetString(GL_SHADING_LANGUAGE_VERSION));



    //create the Light Uniform Buffer Object for later usage
    glGenBuffers(1, &lightUBOLocation);

    glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
    glBufferData(GL_UNIFORM_BUFFER, (sizeof(glm::mat4) + 2 * sizeof(glm::vec4)) * NR_POINT_LIGHTS, NULL,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //create player transforms uniform buffer object
    glGenBuffers(1, &playerUBOLocation);
    glBindBuffer(GL_UNIFORM_BUFFER, playerUBOLocation);
    glBufferData(GL_UNIFORM_BUFFER, 3 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //create depth buffer and texture for shadow map
    glGenFramebuffers(1, &depthOnlyFrameBuffer);

    glGenTextures(1, &depthMap);
    state->activateTextureUnit(0);
    /****************** this block is copy of createTextureArray, except it is GL_DEPTH_COMPONENT *********/
    glBindTexture(GL_TEXTURE_2D_ARRAY, depthMap);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_WIDTH, NR_POINT_LIGHTS, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    /****************** this block is copy of createTextureArray, except it is GL_DEPTH_COMPONENT *********/
    GLfloat borderColor[] = {1.0, 1.0, 1.0, 1.0};
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBuffer);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    checkErrors("Constructor");
}

GLuint GLHelper::generateBuffer(const GLuint number) {
    GLuint bufferID;
    glGenBuffers(number, &bufferID);
    bufferObjects.push_back(bufferID);

    checkErrors("generateBuffer");
    return bufferID;
}

bool GLHelper::deleteBuffer(const GLuint number, const GLuint bufferID) {
    if (glIsBuffer(bufferID)) {
        glDeleteBuffers(number, &bufferID);
        checkErrors("deleteBuffer");
        return true;
    }
    checkErrors("deleteBuffer");
    return false;
}

bool GLHelper::freeBuffer(const GLuint bufferID) {
    for (int i = 0; i < bufferObjects.size(); ++i) {
        if (bufferObjects[i] == bufferID) {
            deleteBuffer(1, bufferObjects[i]);
            bufferObjects[i] = bufferObjects[bufferObjects.size() - 1];
            bufferObjects.pop_back();
            checkErrors("freeBuffer");
            return true;
        }
    }
    checkErrors("freeBuffer");
    return false;
}

GLuint GLHelper::generateVAO(const GLuint number) {
    GLuint bufferID;
    glGenVertexArrays(number, &bufferID);
    vertexArrays.push_back(bufferID);
    checkErrors("generateVAO");
    return bufferID;
}

bool GLHelper::deleteVAO(const GLuint number, const GLuint bufferID) {
    if (glIsBuffer(bufferID)) {
        glDeleteVertexArrays(number, &bufferID);
        checkErrors("deleteVAO");
        return true;
    }
    checkErrors("deleteVAO");
    return false;
}

bool GLHelper::freeVAO(const GLuint bufferID) {
    for (int i = 0; i < vertexArrays.size(); ++i) {
        if (vertexArrays[i] == bufferID) {
            deleteBuffer(1, vertexArrays[i]);
            vertexArrays[i] = vertexArrays[vertexArrays.size() - 1];
            vertexArrays.pop_back();
            checkErrors("freeVAO");
            return true;
        }
    }
    checkErrors("freeVAO");
    return false;
}

void GLHelper::bufferVertexData(const std::vector<glm::vec3> &vertices,
                                const std::vector<glm::mediump_uvec3> &faces,
                                GLuint &vao, GLuint &vbo, const GLuint attachPointer, GLuint &ebo) {

    // Set up the element array buffer
    ebo = generateBuffer(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(glm::mediump_uvec3), faces.data(), GL_STATIC_DRAW);

    // Set up the vertex attributes
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    vbo = generateBuffer(1);
    bufferObjects.push_back(vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    //glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize, vertexData);
    glVertexAttribPointer(attachPointer, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(attachPointer);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    checkErrors("bufferVertexData");
}

void GLHelper::bufferNormalData(const std::vector<glm::vec3> &normals,
                                GLuint &vao, GLuint &vbo, const GLuint attachPointer) {
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);

    glBindVertexArray(vao);
    glVertexAttribPointer(attachPointer, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);
    checkErrors("bufferVertexColor");
}

void GLHelper::bufferVertexColor(const std::vector<glm::vec4> &colors,
                                 GLuint &vao, GLuint &vbo, const GLuint attachPointer) {
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STATIC_DRAW);

    glBindVertexArray(vao);
    glVertexAttribPointer(attachPointer, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);
    checkErrors("bufferVertexColor");
}

void GLHelper::bufferVertexTextureCoordinates(const std::vector<glm::vec2> &textureCoordinates,
                                              GLuint &vao, GLuint &vbo, const GLuint attachPointer, GLuint &ebo) {
    vbo = generateBuffer(1);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, textureCoordinates.size() * sizeof(glm::vec2), textureCoordinates.data(),
                 GL_STATIC_DRAW);

    glBindVertexArray(vao);
    glVertexAttribPointer(attachPointer, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attachPointer);
    glBindVertexArray(0);
    checkErrors("bufferVertexTextureCoordinates");
}

void GLHelper::setCamera(const glm::vec3 &cameraPosition, const glm::mat4 &cameraTransform) {
    this->cameraMatrix = cameraTransform;
}


void GLHelper::switchRenderToShadowMap(const unsigned int index) {
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFrameBuffer);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0, index);

    glClear(GL_DEPTH_BUFFER_BIT);

    glCullFace(GL_FRONT);

}

void GLHelper::switchrenderToDefault() {
    glViewport(0, 0, screenWidth, screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //we bind shadow map to last texture unit
    state->attach2DTextureArray(depthMap, maxTextureImageUnits - 1);
    glCullFace(GL_BACK);
}

void GLHelper::render(const GLuint program, const GLuint vao, const GLuint ebo, const GLuint elementCount) {
    if (program == 0) {
        std::cerr << "No program render requested." << std::endl;
        return;
    }
    state->setProgram(program);

    // Set up for a glDrawElements call
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glDrawElements(GL_TRIANGLES, elementCount, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
    //state->setProgram(0);

    checkErrors("render");
}


bool GLHelper::setUniform(const GLuint programID, const GLuint uniformID, const glm::mat4 &matrix) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniformMatrix4fv(uniformID, 1, GL_FALSE, glm::value_ptr(matrix));
        //state->setProgram(0);
        checkErrors("setUniformMatrix");
        return true;
    }

}

bool GLHelper::setUniform(const GLuint programID, const GLuint uniformID, const glm::vec3 &vector) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform3fv(uniformID, 1, glm::value_ptr(vector));
        //state->setProgram(0);
        checkErrors("setUniformVector");
        return true;
    }
}

bool GLHelper::setUniform(const GLuint programID, const GLuint uniformID, const float value) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform1f(uniformID, value);
        //state->setProgram(0);
        checkErrors("setUniformFloat");
        return true;
    }
}

bool GLHelper::setUniform(const GLuint programID, const GLuint uniformID, const int value) {
    if (!glIsProgram(programID)) {
        std::cerr << "invalid program for setting uniform." << std::endl;
        return false;
    } else {
        state->setProgram(programID);
        glUniform1i(uniformID, value);
        //state->setProgram(0);
        checkErrors("setUniformInt");
        return true;
    }
}

bool GLHelper::checkErrors(std::string callerFunc) {
    bool hasError = false;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cerr << "error found on GL context while " << callerFunc << ":" << error << std::endl;
        hasError = true;
    }
    return hasError;
}


GLHelper::~GLHelper() {
    for (int i = 0; i < bufferObjects.size(); ++i) {
        deleteBuffer(1, bufferObjects[i]);
    }

    deleteBuffer(1, lightUBOLocation);
    deleteBuffer(1, playerUBOLocation);
    deleteBuffer(1, depthMap);
    glDeleteFramebuffers(1, &depthOnlyFrameBuffer); //maybe we should wrap this up too
    //state->setProgram(0);
}

void GLHelper::reshape(unsigned int height, unsigned int width) {
    this->screenHeight = height;
    this->screenWidth = width;
    glViewport(0, 0, width, height);
    aspect = float(height) / float(width);
    perspectiveProjectionMatrix = glm::perspective(45.0f, 1.0f / aspect, 0.1f, 100.0f);
    orthogonalProjectionMatrix = glm::ortho(0.0f, (float) width, 0.0f, (float) height);
    checkErrors("reshape");
}

GLuint GLHelper::loadTexture(int height, int width, GLenum format, void *data) {
    GLuint texture;
    glGenTextures(1, &texture);
    state->activateTextureUnit(0);//this is the default working texture
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    checkErrors("loadTexture");
    return texture;
}

void GLHelper::attachTexture(unsigned int textureID, unsigned int attachPoint) {
    state->attachTexture(textureID, attachPoint);
    checkErrors("attachTexture");
}

void GLHelper::attach2DTextureArray(unsigned int textureID, unsigned int attachPoint) {
    state->attach2DTextureArray(textureID, attachPoint);
    checkErrors("attachTexture");
}


void GLHelper::attachCubeMap(unsigned int cubeMapID, unsigned int attachPoint) {
    state->attachCubemap(cubeMapID, attachPoint);
    checkErrors("attachCubeMap");
}

bool GLHelper::deleteTexture(GLuint textureID) {
    if (glIsTexture(textureID)) {
        glDeleteTextures(1, &textureID);
        checkErrors("deleteTexture");
        return true;
    } else {
        checkErrors("deleteTexture");
        return false;
    }
}

GLuint GLHelper::loadCubeMap(int height, int width, void *right, void *left, void *top, void *bottom, void *back,
                             void *front) {
    GLuint cubeMap;
    glGenTextures(1, &cubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, right);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, left);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, top);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, bottom);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, back);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, front);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    checkErrors("loadCubeMap");
    return cubeMap;
}

bool GLHelper::getUniformLocation(const GLuint programID, const std::string &uniformName, GLuint &location) {
    GLint rawLocation = glGetUniformLocation(programID, uniformName.c_str());
    if (!checkErrors("getUniformLocation")) {
        if (rawLocation >= 0) {
            location = rawLocation;
            return true;
        } else {
            std::cerr << "No error found, but uniform[" << uniformName << "] can not be located " << std::endl;

        }
    }
    return false;
}

/**
 * This is pretty low performance, but it is used only for debugging physics, so it is good enough
 */
void GLHelper::drawLine(const glm::vec3 &from, const glm::vec3 &to,
                        const glm::vec3 &fromColor, const glm::vec3 &toColor, bool willTransform) {
    static GLuint program, viewTransformU, lineInfoU, vao, vbo;
    static std::map<std::string, Uniform*> uniformMap;//FIXME That map will always be empty, maybe we should overload
    if (program == 0 || vbo == 0 || vao == 0) {
        program = initializeProgram("./Data/Shaders/Line/vertex.glsl", "./Data/Shaders/Line/fragment.glsl", uniformMap);
        lineInfoU = glGetUniformLocation(program, "lineInfo");
        viewTransformU = glGetUniformLocation(program, "cameraTransformMatrix");
        state->setProgram(program);
        vbo = generateBuffer(1);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        std::vector<GLint> indexes;
        indexes.push_back(0);
        indexes.push_back(1);
        glBufferData(GL_ARRAY_BUFFER, indexes.size() * sizeof(GLint), indexes.data(), GL_STATIC_DRAW);

        //this means the vao is not created yet.
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        //stride means the space between start of 2 elements. 0 is special, means the space is equal to size.
        glVertexAttribPointer(0, 1, GL_INT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);
    }
    state->setProgram(program);
    glm::mat4 matrix(glm::vec4(from, 1.0f),
                     glm::vec4(to, 1.0f),
                     glm::vec4(fromColor, 1.0f),
                     glm::vec4(toColor, 1.0f));

    glUniformMatrix4fv(lineInfoU, 1, GL_FALSE, glm::value_ptr(matrix));

    if (willTransform) {
        glUniformMatrix4fv(viewTransformU, 1, GL_FALSE, glm::value_ptr(getProjectionMatrix() * getCameraMatrix()));
    } else {
        glUniformMatrix4fv(viewTransformU, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0)));
    }

    glBindVertexArray(vao);

    glDrawArrays(GL_LINES, 0, 2);

    glBindVertexArray(0);
    //state->setProgram(0);
    checkErrors("drawLine");

}

void GLHelper::setLight(const Light &light, const int i) {
    glm::mat4 lightView = glm::lookAt(light.getPosition(),
                                      glm::vec3(0.0f, 0.0f, 0.0f),
                                      glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    glBindBuffer(GL_UNIFORM_BUFFER, lightUBOLocation);
    glBufferSubData(GL_UNIFORM_BUFFER, i * (sizeof(glm::mat4) + 2 * sizeof(glm::vec4)), sizeof(glm::mat4),
                    glm::value_ptr(lightSpaceMatrix));
    glBufferSubData(GL_UNIFORM_BUFFER, i * (sizeof(glm::mat4) + 2 * sizeof(glm::vec4)) + sizeof(glm::mat4),
                    sizeof(glm::vec3), &light.getPosition());
    glBufferSubData(GL_UNIFORM_BUFFER,
                    i * (sizeof(glm::mat4) + 2 * sizeof(glm::vec4)) + sizeof(glm::mat4) + sizeof(glm::vec4),
                    sizeof(glm::vec3), &light.getColor());

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkErrors("setLight");
}

void GLHelper::setPlayerMatrices() {
    glBindBuffer(GL_UNIFORM_BUFFER, playerUBOLocation);
    glBufferSubData(GL_UNIFORM_BUFFER, 0 * sizeof(glm::mat4), sizeof(glm::mat4), &cameraMatrix);
    glBufferSubData(GL_UNIFORM_BUFFER, 1 * sizeof(glm::mat4), sizeof(glm::mat4), &perspectiveProjectionMatrix);
    glm::mat4 viewMatrix = perspectiveProjectionMatrix * cameraMatrix;
    glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), sizeof(glm::mat4), &viewMatrix);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    checkErrors("setPlayerMatrices");
}

unsigned int GLHelper::createTextureArray(const unsigned int width, const unsigned int height, const unsigned int arraySize) {
    unsigned int textureID;
    state->activateTextureUnit(0);
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, width, height, arraySize, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    checkErrors("createTextureArray");
    return textureID;
}

void GLHelper::loadTextureArrayElement(const unsigned int textureID, const char index, unsigned int height,
                                       unsigned int width, int component, void *data) {
    state->activateTextureUnit(0);//this is the default working texture
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, index, width, height, 1, component, GL_UNSIGNED_BYTE, data);
    checkErrors("loadTextureArrayElement");
}


