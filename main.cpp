// Volume rendering project

#include "GLSLProgram.h"
#include "VTKReader.h"
#include "Volume.h"
#include "VolumeBase.h"
#include "Texture2D.h"
#include "Texture3D.h"
#include "FBO.h"
#include "Trackball.h"
#include "Camera.h"

#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <memory>
#include <AntTweakBar.h>
//#include <Algorithm>

#define WINDOW_TITLE "Volume renderer"

// Struct for global resources
struct Globals {
    int width;
    int height;
    int windowID;
    bool enableTrackball;
    GLuint quadVBO;
    GLuint cubeVBO;
    cgtk::GLSLProgram boundingGeometryProgram;
    cgtk::GLSLProgram rayCastingProgram;
    cgtk::GLSLProgram blitProgram;
    std::shared_ptr<cgtk::VolumeBase> volume;
    std::shared_ptr<cgtk::Texture3D> volumeTexture;
    cgtk::FBO frontFaceFBO;
    cgtk::FBO backFaceFBO;
    cgtk::Trackball trackball;
    cgtk::Camera camera;
    glm::fquat rotation;
    TwBar *bar;
    int steps;
    glm::vec4 transferColor1;
    glm::vec4 transferColor2;
    glm::vec4 transferColor3;
    glm::vec4 transferColor4;
    glm::vec4 backgroundColor;
    glm::vec4 mipColor;
    float transferPoint1;
    float transferPoint2;
    float transferPoint3;
    float transferPoint4;
    bool invert;
    bool MIP;
};

Globals globals;

void setUpAntTweakBar()
{
    TwInit(TW_OPENGL, NULL);
    globals.bar = TwNewBar("TweakBar");
    TwDefine(" TweakBar size='200 450',GLOBAL fontsize=3"); // resize bar

    TwAddVarRW(globals.bar, "Orientation", TW_TYPE_QUAT4F, &globals.rotation, "opened=true readonly=true");
    TwAddVarRW(globals.bar, "Step Size", TW_TYPE_INT32, &globals.steps, "step=10,max=100 ");

    TwAddSeparator(globals.bar, NULL, NULL);
    TwAddVarRW(globals.bar, "Transfer_Color_1", TW_TYPE_COLOR4F, &globals.transferColor1[0], "colormode=rgb");
    TwAddVarRW(globals.bar, "Transfer_Point_1", TW_TYPE_FLOAT, &globals.transferPoint1, "step=0.1,max=1.0");
    TwAddVarRW(globals.bar, "Transfer_Color_2", TW_TYPE_COLOR4F, &globals.transferColor2[0], "colormode=rgb");
    TwAddVarRW(globals.bar, "Transfer_Point_2", TW_TYPE_FLOAT, &globals.transferPoint2, "step=0.1,max=1.0 ");
    TwAddVarRW(globals.bar, "Transfer_Color_3", TW_TYPE_COLOR4F, &globals.transferColor3[0], "colormode=rgb");
    TwAddVarRW(globals.bar, "Transfer_Point_3", TW_TYPE_FLOAT, &globals.transferPoint3, "step=0.1,max=1.0 ");
    TwAddVarRW(globals.bar, "Transfer_Color_4", TW_TYPE_COLOR4F, &globals.transferColor4[0], "colormode=rgb");
    TwAddVarRW(globals.bar, "Transfer_Point_4", TW_TYPE_FLOAT, &globals.transferPoint4, "step=0.1,max=1.0 ");

    TwAddSeparator(globals.bar, NULL, NULL);
    TwAddVarRW(globals.bar, "Background Color", TW_TYPE_COLOR4F, &globals.backgroundColor[0], "colormode=rgb");

    TwAddVarRW(globals.bar,"Invert", TW_TYPE_BOOLCPP, &globals.invert, "");
    TwAddVarRW(globals.bar,"MIP", TW_TYPE_BOOLCPP, &globals.MIP, "");
    TwAddVarRW(globals.bar, "MIP_Color", TW_TYPE_COLOR4F, &globals.mipColor[0], "colormode=rgb");
    TwDefine(" TweakBar/MIP_Color visible=false");

}

void initializeGlobals(){
    globals.steps = 10;
    globals.transferColor1 = glm::vec4(0.0,0.0,0.0,0.0);
    globals.transferColor2 = glm::vec4(68.0/100.0,61.0/255.0,60.0/255.0,0.5);
    globals.transferColor3 = glm::vec4(1.0,1.0,0.0,1.0);
    globals.transferColor4 = glm::vec4(1.0,1.0,1.0,1.0);
    globals.transferPoint1 = 0.077;
    globals.transferPoint2 = 0.285;
    globals.transferPoint3 = 0.685;
    globals.transferPoint4 = 1.00;
    globals.backgroundColor = glm::vec4(0.0,0.0,0.0,0.0);
    globals.mipColor = glm::vec4(1.0,0.0,0.0,1.0);
}

void setUniformVariables(cgtk::GLSLProgram &program)
{
    program.setUniform1i("u_steps", globals.steps);
    program.setUniform4f("u_transferColor1", globals.transferColor1);
    program.setUniform4f("u_transferColor2", globals.transferColor2);
    program.setUniform4f("u_transferColor3", globals.transferColor3);
    program.setUniform4f("u_transferColor4", globals.transferColor4);
    program.setUniform1f("u_transferPoint1", globals.transferPoint1);
    program.setUniform1f("u_transferPoint2", globals.transferPoint2);
    program.setUniform1f("u_transferPoint3", globals.transferPoint3);
    program.setUniform1f("u_transferPoint4", globals.transferPoint4);
    program.setUniform1i("u_invert", globals.invert);
    program.setUniform1i("u_MIP", globals.MIP);
    if(globals.MIP){
        TwDefine(" TweakBar/MIP_Color visible=true");
        TwDefine(" TweakBar/Transfer_Point_1 visible=false");
        TwDefine(" TweakBar/Transfer_Point_2 visible=false");
        TwDefine(" TweakBar/Transfer_Point_3 visible=false");
        TwDefine(" TweakBar/Transfer_Point_4 visible=false");
        TwDefine(" TweakBar/Transfer_Color_1 visible=false");
        TwDefine(" TweakBar/Transfer_Color_2 visible=false");
        TwDefine(" TweakBar/Transfer_Color_3 visible=false");
        TwDefine(" TweakBar/Transfer_Color_4 visible=false");
    }
    else
    {
        TwDefine(" TweakBar/MIP_Color visible=false");
        TwDefine(" TweakBar/Transfer_Point_1 visible=true");
        TwDefine(" TweakBar/Transfer_Point_2 visible=true");
        TwDefine(" TweakBar/Transfer_Point_3 visible=true");
        TwDefine(" TweakBar/Transfer_Point_4 visible=true");
        TwDefine(" TweakBar/Transfer_Color_1 visible=true");
        TwDefine(" TweakBar/Transfer_Color_2 visible=true");
        TwDefine(" TweakBar/Transfer_Color_3 visible=true");
        TwDefine(" TweakBar/Transfer_Color_4 visible=true");
    }

    program.setUniform4f("u_MIP_Color", globals.mipColor);
}

// Returns the value of the environment variable whose name is
// specified by the argument
std::string getEnvVar(const std::string &name)
{
    char *value = getenv(name.c_str());
    if (value == NULL) {
        return std::string();
    }
    else {
        return std::string(value);
    }
}

// Returns the absolute path to the shader directory
std::string shaderDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT4_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT4_ROOT is not set." << std::endl;
        exit(EXIT_FAILURE);
    }
    return rootDir + "/src/shaders/";
}

// Returns the absolute path to the volume data directory
std::string dataDir(void)
{
    std::string rootDir = getEnvVar("ASSIGNMENT4_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: ASSIGNMENT4_ROOT is not set." << std::endl;
        exit(EXIT_FAILURE);
    }
    return rootDir + "/data/";
}

void initGLEW(void)
{
    GLenum status = glewInit();
    if (status != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(status) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void displayOpenGLVersion(void)
{
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
}

std::shared_ptr<cgtk::VolumeBase> loadVolume(const std::string &filename)
{
    cgtk::VTKReader vtkReader;
    if (!vtkReader.read(filename)) {
        std::cerr << "Error: Could not read volume " << filename << std::endl;
        exit(EXIT_FAILURE);
    }
    return vtkReader.getVolume();
}

std::shared_ptr<cgtk::Texture3D> createVolumeTexture(std::shared_ptr<cgtk::VolumeBase> &volume)
{
    std::shared_ptr<cgtk::Texture3D> volumeTexture(new cgtk::Texture3D());
    if (volume->getDataType() == "uint8") {
        glm::ivec3 dimensions = volume->getDimensions();
        volumeTexture->setWidth(dimensions[0]);
        volumeTexture->setHeight(dimensions[1]);
        volumeTexture->setDepth(dimensions[2]);
        volumeTexture->setInternalFormat(GL_R8);
        volumeTexture->setFormat(GL_RED);
        volumeTexture->setType(GL_UNSIGNED_BYTE);
        volumeTexture->setWrapModeS(GL_CLAMP_TO_BORDER);
        volumeTexture->setWrapModeT(GL_CLAMP_TO_BORDER);
        volumeTexture->setWrapModeR(GL_CLAMP_TO_BORDER);
        volumeTexture->setMinFilter(GL_LINEAR);
        volumeTexture->setMagFilter(GL_LINEAR);
        volumeTexture->update();

        std::shared_ptr<cgtk::VolumeUInt8> tmp = std::static_pointer_cast<cgtk::VolumeUInt8>(volume);
        volumeTexture->write(tmp->getImageData().data());
    }
    return volumeTexture;
}

// Returns the ID of a VBO holding a fullscreen quad (i.e., two
// triangles that together fill up the entire screen or window)
GLuint createQuadVBO()
{
    const GLfloat vertices[] = {
        -1.0f, -1.0f,  0.0f,
         1.0f, -1.0f,  0.0f,
         1.0f,  1.0f,  0.0f,
        -1.0f, -1.0f,  0.0f,
         1.0f,  1.0f,  0.0f,
        -1.0f,  1.0f,  0.0f,
    };

    GLuint quadVBO;
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return quadVBO;
}

// Returns the ID of a VBO holding a 2-unit cube centered at origin
GLuint createCubeVBO(void)
{
    const GLfloat vertices[] = {
        // back
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        // front
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        // right
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        // left
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        // top
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
        // bottom
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f
    };

    GLuint cubeVBO;
    glGenBuffers(1, &cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return cubeVBO;
}

void setUpCamera(void)
{
    globals.camera.setFOV(45.0f);
    globals.camera.setAspectRatio((float)globals.width / (float)globals.height);
    globals.camera.setZNear(1.0f);
    globals.camera.setZFar(1000.0f);

    globals.camera.setEye(glm::vec3(0.0f, 0.0f, 400.0f));
    globals.camera.setCenter(glm::vec3(0.0f, 0.0f, 0.0f));
    globals.camera.setUp(glm::vec3(0.0f, 1.0f, 0.0f));
}

void setUpTrackball(void)
{
    double radius = double(std::min(globals.width, globals.height)) / 2.0;
    globals.trackball.setRadius(radius);
    glm::vec2 center = glm::vec2(globals.width, globals.height) / 2.0f;
    globals.trackball.setCenter(center);
}


void init(void)
{
    glClearColor(globals.backgroundColor.x,
                 globals.backgroundColor.y,
                 globals.backgroundColor.z, 
                 globals.backgroundColor.a);

    // Load volume data
    globals.volume = loadVolume(dataDir() + "bonsai.vtk");

    // Create volume texture
    globals.volumeTexture = createVolumeTexture(globals.volume);

    // Create geometry
    globals.quadVBO = createQuadVBO();
    globals.cubeVBO = createCubeVBO();

    // Create shader programs
    globals.boundingGeometryProgram.setShaderSource(GL_VERTEX_SHADER,
        cgtk::readGLSLSource(shaderDir() + "boundingGeometry.vert"));
    globals.boundingGeometryProgram.setShaderSource(GL_FRAGMENT_SHADER,
        cgtk::readGLSLSource(shaderDir() + "boundingGeometry.frag"));
    globals.boundingGeometryProgram.setAttributeLocation("a_position", 0);
    globals.boundingGeometryProgram.update();

    globals.rayCastingProgram.setShaderSource(GL_VERTEX_SHADER,
        cgtk::readGLSLSource(shaderDir() + "rayCaster.vert"));
    globals.rayCastingProgram.setShaderSource(GL_FRAGMENT_SHADER,
        cgtk::readGLSLSource(shaderDir() + "rayCaster.frag"));
    globals.rayCastingProgram.setAttributeLocation("a_position", 0);
    globals.rayCastingProgram.update();

    globals.blitProgram.setShaderSource(GL_VERTEX_SHADER,
        cgtk::readGLSLSource(shaderDir() + "blit.vert"));
    globals.blitProgram.setShaderSource(GL_FRAGMENT_SHADER,
        cgtk::readGLSLSource(shaderDir() + "blit.frag"));
    globals.blitProgram.setAttributeLocation("a_position", 0);
    globals.blitProgram.update();

    // Create frame buffer objects (FBOs)
    std::shared_ptr<cgtk::Texture2D> frontFaceColorTexture(new cgtk::Texture2D());
    frontFaceColorTexture->setWidth(globals.width);
    frontFaceColorTexture->setHeight(globals.height);
    frontFaceColorTexture->setInternalFormat(GL_RGBA16);
    frontFaceColorTexture->setFormat(GL_RGBA);
    frontFaceColorTexture->setType(GL_UNSIGNED_SHORT);
    frontFaceColorTexture->update();
    globals.frontFaceFBO.setAttachment(GL_COLOR_ATTACHMENT0, frontFaceColorTexture);

    std::shared_ptr<cgtk::Texture2D> frontFaceDepthTexture(new cgtk::Texture2D());
    frontFaceDepthTexture->setWidth(globals.width);
    frontFaceDepthTexture->setHeight(globals.height);
    frontFaceDepthTexture->setInternalFormat(GL_DEPTH_COMPONENT24);
    frontFaceDepthTexture->setFormat(GL_DEPTH_COMPONENT);
    frontFaceDepthTexture->setType(GL_FLOAT);
    frontFaceDepthTexture->update();
    globals.frontFaceFBO.setAttachment(GL_DEPTH_ATTACHMENT, frontFaceDepthTexture);

    globals.frontFaceFBO.update();

    std::shared_ptr<cgtk::Texture2D> backFaceColorTexture(new cgtk::Texture2D());
    backFaceColorTexture->setWidth(globals.width);
    backFaceColorTexture->setHeight(globals.height);
    backFaceColorTexture->setInternalFormat(GL_RGBA16);
    backFaceColorTexture->setFormat(GL_RGBA);
    backFaceColorTexture->setType(GL_UNSIGNED_SHORT);
    backFaceColorTexture->update();
    globals.backFaceFBO.setAttachment(GL_COLOR_ATTACHMENT0, backFaceColorTexture);

    std::shared_ptr<cgtk::Texture2D> backFaceDepthTexture(new cgtk::Texture2D());
    backFaceDepthTexture->setWidth(globals.width);
    backFaceDepthTexture->setHeight(globals.height);
    backFaceDepthTexture->setInternalFormat(GL_DEPTH_COMPONENT24);
    backFaceDepthTexture->setFormat(GL_DEPTH_COMPONENT);
    backFaceDepthTexture->setType(GL_FLOAT);
    backFaceDepthTexture->update();
    globals.backFaceFBO.setAttachment(GL_DEPTH_ATTACHMENT, backFaceDepthTexture);

    globals.backFaceFBO.update();

    // Set up the rest of the scene
    setUpCamera();
    setUpTrackball();

    // Set up the AntTweakBar
    setUpAntTweakBar();
    initializeGlobals();
}


// Used to draw the front- or backfaces of the volume bounding
// box. You don't need to modify this function, just the
// boundingGeometry.vert and boundingGeometry.frag shaders.
void drawBoundingGeometry(cgtk::GLSLProgram &program, const GLuint cubeVBO)
{
    program.enable();

    glm::mat4 model = globals.volume->getModelMatrix();
    model = globals.trackball.getRotationMatrix() * model;

    // AntTweakBar
    globals.rotation = glm::quat_cast(globals.trackball.getRotationMatrix());

    glm::mat4 projection = globals.camera.getProjectionMatrix();
    glm::mat4 view = globals.camera.getViewMatrix();
    glm::mat4 mvp = projection * view * model;

    program.setUniformMatrix4f("u_mvp", mvp);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glEnableVertexAttribArray(program.getAttributeLocation("a_position"));
    glVertexAttribPointer(program.getAttributeLocation("a_position"), 3,
                          GL_FLOAT, GL_FALSE, 0, NULL);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    program.disable();
}


// MODIFY THIS FUNCTION and the rayCaster.vert and rayCaster.frag to
// perform ray-casting
void renderVolume(cgtk::GLSLProgram &program, const GLuint quadVBO,
                  const GLuint frontFaceTexture, const GLuint backFaceTexture,
                  const GLuint volumeTexture)
{
    program.enable();


    // Set uniforms and bind textures here

    program.setUniform1i("u_frontFaceTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, frontFaceTexture);

    program.setUniform1i("u_backFaceTexture", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, backFaceTexture);

    program.setUniform1i("u_volumeTexture", 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, volumeTexture);
    
    setUniformVariables(program);

    //float diffuseIntensity = 0.50;
    //float ambientIntensity = 0.50;
    //float specularPower = 60;
    //float specularIntensity = 0.05;
    //glm::vec3 diffuseColor = glm::vec3(1.0,1.0,1.0);
    //glm::vec3 specularColor = glm::vec3(1.0,1.0,1.0);
    //glm::vec3 ambientColor = glm::vec3(1.0,1.0,1.0);
    //glm::vec3 lightColor = glm::vec3(1.0,1.0,1.0);
    //glm::vec3 lightPosition = glm::vec3(0.0,4.0,-1.0);
    //program.setUniform3f("u_light_position",lightPosition);
    //program.setUniform3f("u_light_color",lightColor);
    //program.setUniform3f("K_a",glm::vec3(ambientIntensity,ambientIntensity,ambientIntensity));
    //program.setUniform3f("K_d",glm::vec3(diffuseIntensity,diffuseIntensity,diffuseIntensity));
    //program.setUniform3f("K_s",glm::vec3(specularIntensity,specularIntensity,specularIntensity));
    //program.setUniform3f("K_d_color",diffuseColor);
    //program.setUniform3f("K_a_color",ambientColor);
    //program.setUniform3f("K_s_color",specularColor);
    //program.setUniform1f("specular_power",specularPower);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glEnableVertexAttribArray(program.getAttributeLocation("a_position"));
    glVertexAttribPointer(program.getAttributeLocation("a_position"), 3,
                          GL_FLOAT, GL_FALSE, 0, NULL);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    program.disable();
}

// This function can be used to render ("blit") a 2D texture to the
// screen or some other render target
void blit(cgtk::GLSLProgram &program, const GLuint quadVBO, const GLuint texture)
{


    program.setUniform1i("u_texture", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glEnableVertexAttribArray(program.getAttributeLocation("a_position"));
    glVertexAttribPointer(program.getAttributeLocation("a_position"), 3,
                          GL_FLOAT, GL_FALSE, 0, NULL);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    program.disable();
}

// MODIFY THIS FUNCTION
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the front faces of the volume bounding box to a texture
    // via the frontFaceFBO
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glCullFace(GL_BACK);
    globals.frontFaceFBO.bind();
    globals.frontFaceFBO.clear();
    drawBoundingGeometry(globals.boundingGeometryProgram, globals.cubeVBO);
    globals.frontFaceFBO.unbind();
    //

    // Render the back faces of the volume bounding box to a texture
    // via the backFaceFBO
    // ...
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    globals.backFaceFBO.bind();
    globals.backFaceFBO.clear();
    drawBoundingGeometry(globals.boundingGeometryProgram, globals.cubeVBO);
    globals.backFaceFBO.unbind();

    glDisable(GL_CULL_FACE);
    // ...

    // Perform ray-casting
    // ...
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    renderVolume(globals.rayCastingProgram, globals.quadVBO,
                  globals.frontFaceFBO.getTexture(GL_COLOR_ATTACHMENT0)->getHandle(),
                  globals.backFaceFBO.getTexture(GL_COLOR_ATTACHMENT0)->getHandle(),
                  globals.volumeTexture->getHandle());
    // ...

    // AntTweakDraw
    TwDraw();

    //blit(globals.blitProgram, globals.quadVBO,
         //globals.frontFaceFBO.getTexture(GL_COLOR_ATTACHMENT0)->getHandle());

    glClearColor(globals.backgroundColor.x,
                 globals.backgroundColor.y,
                 globals.backgroundColor.z, 
                 globals.backgroundColor.a);
    

    glutSwapBuffers();
}

void reshape(int width, int height)
{
    globals.width = width;
    globals.height = height;
    globals.camera.setAspectRatio((float)globals.width / (float)globals.height);
    globals.trackball.setRadius(double(std::min(width, height)) / 2.0);
    globals.trackball.setCenter(glm::vec2(width, height) / 2.0f);
    globals.frontFaceFBO.resize(width, height);
    globals.backFaceFBO.resize(width, height);
    glViewport(0, 0, globals.width, globals.height);
    TwWindowSize(width, height);
}

void zoom(int button)
{
    float zoomStep = 0.05;
    float currentZoom = globals.camera.getZoom();
    if (button == 3) {
        globals.camera.setZoom(std::max(zoomStep, currentZoom - zoomStep));
    }
    else {
        globals.camera.setZoom(currentZoom + zoomStep);
    }
}

void mouseButtonPressed(int button, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
        globals.trackball.setCenter(glm::vec2(x, y));
        globals.trackball.startTracking(glm::vec2(x, y));
        if (!TwEventMouseMotionGLUT(x,y)){
            globals.enableTrackball = true;
        }
        else{
            globals.enableTrackball = false;
        }

    }
    else if (button == 3 || button == 4) {
        zoom(button);
    }
}

void mouseButtonReleased(int button, int /*x*/, int /*y*/)
{
    if (button == GLUT_LEFT_BUTTON) {
        globals.trackball.stopTracking();
    }
}

void mouse(int button, int state, int x, int y)
{

    if( !TwEventMouseMotionGLUT(x,y) ){
        if (state == GLUT_DOWN) {
            mouseButtonPressed(button, x, y);
        }
        else {
            mouseButtonReleased(button, x, y);
        }
    }
    else
    {
        TwEventMouseButtonGLUT(button,state,x,y);
    }
}

void moveTrackball(int x, int y)
{
    if (globals.trackball.tracking()) {
        globals.trackball.move(glm::vec2(x, y));
    }
}

void motion(int x, int y)
{
    if(globals.enableTrackball){
        moveTrackball(x, y);
    }
}

void idle(void)
{
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    //globals.width = 1680;
    //globals.height = 1050;
    globals.width = 800;
    globals.height = 600;
    glutInitWindowSize(globals.width, globals.height);
    glutInitWindowPosition(550, 250);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    globals.windowID = glutCreateWindow(WINDOW_TITLE);
    initGLEW();
    displayOpenGLVersion();
    init();
    glutDisplayFunc(&display);
    glutReshapeFunc(&reshape);

    // AntTweakBar
    glutPassiveMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT); // same as MouseMotion
    glutKeyboardFunc((GLUTkeyboardfun)TwEventKeyboardGLUT);
    glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);

    // send the ''glutGetModifers'' function pointer to AntTweakBar
    TwGLUTModifiersFunc(glutGetModifiers);

    glutMouseFunc(&mouse);
    glutMotionFunc(&motion);
    glutIdleFunc(&idle);
    glutMainLoop();

    return EXIT_SUCCESS;
}
