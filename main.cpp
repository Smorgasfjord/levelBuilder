/*
 *  CPE 471 Final Program - aMAZEing
 *  Generates a random maze for the user to navigate.
 *
 *  Created by Taylor Woods on 2/20/14
 *
 *****************************************************************************/
#ifdef __APPLE__
#include <OPENGL/gl.h>
#endif
#ifdef __unix__
#include <GL/glut.h>
#endif

#define GLFW_INCLUDE_GLU
#include "glfw3.h"
#include "CMeshLoaderSimple.h"
#include "Platform.h"
#include "Mountain.h"
#include "GameObject.h"
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <streambuf>
#include "GLSL_helper.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp" //perspective, trans etc
#include "glm/gtc/type_ptr.hpp" //value_ptr

#define NUM_PLANES 10
#define INIT_WIDTH 800
#define INIT_HEIGHT 600
#define pi 3.14159
#define PLANE_HEIGHT 1.25
#define WALL_COLLISION_SIZE .63

//Material selection constants
#define GROUND_MAT 3

//This needs to be forward declared for GLFW
void move(glm::vec3 delta);

using namespace std;
vector<Platform> importLevel(std::string const & fileName);

//GL basics
int ShadeProg;
static float g_width, g_height;

//Handles to the shader data
GLHandles handles;

//Models
vector<GameModel> Models;

//Ground
GameModel grndMod;
vector<glm::vec3> groundTiles;
static const float g_groundY = 0;
static const float g_groundSize = 60.0;

//Platform
GameModel platMod;
vector<Platform> platforms;
Platform placing;

//Mountain
GameModel mountMod;
Mountain mount;

//Placement flags
bool placeMode = false;

//Light
glm::vec3 lightPos;

//Camera
float firstPersonHeight = 10;
glm::vec3 eye = glm::vec3(g_groundSize / 2, firstPersonHeight, -5);
glm::vec3 lookAtVec = glm::vec3(g_groundSize / 2 + 1, firstPersonHeight, g_groundSize / 2 + 1);
glm::vec3 upV = glm::vec3(0, 1, 0);
float pitch = 0;
float yaw = pi/2;

//User interaction
glm::vec2 prevMouseLoc;

/* projection matrix */
void SetProjectionMatrix(bool drawText) {
   glm::mat4 Projection;
   if(!drawText)
      Projection = glm::perspective(80.0f, (float)g_width/g_height, 0.1f, 100.f);
   else
      Projection = glm::ortho(0.0f, (float)g_width / 2,(float)g_height / 2,0.0f, 0.1f, 100.0f);
   safe_glUniformMatrix4fv(handles.uProjMatrix, glm::value_ptr(Projection));
}

/* camera controls */
void SetView() {
   glm::mat4 view = glm::lookAt(eye, lookAtVec, upV);
   safe_glUniformMatrix4fv(handles.uViewMatrix, glm::value_ptr(view));
}

//Generates a random float within the range min-max
float randomFloat(float min, float max)
{
   return (max - min) * (rand() / (double) RAND_MAX) + min;
}

GameModel init_Ground(float g_groundY)
{
   float GrndPos[] = {
      0, g_groundY, 0,
      0, g_groundY, 1,
      1, g_groundY, 1,
      1, g_groundY, 0,
   };
   
   unsigned short idx[] =
   {
      2, 1, 0,
      3, 2, 0,
   };
   
   float grndNorm[] =
   {
      0, 1, 0,
      0, 1, 0,
      0, 1, 0,
      0, 1, 0,
      0, 1, 0,
      0, 1, 0,
   };
   
   static GLfloat GrndTex[] = {
      0, 0,
      0, 1,
      1, 0,
      1, 1
   };
   
   GameModel mod;
   ModelMesh mesh;
   
   mod = GameModel(Model(), 1, "ground");
   mesh = ModelMesh(0,0,0,0, sizeof(idx)/sizeof(float));
   mesh.numFaces = 6;
   glGenBuffers(1, &mesh.posBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, mesh.posBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);
   
   glGenBuffers(1, &mesh.idxBuffObj);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.idxBuffObj);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
   
   glGenBuffers(1, &mesh.normBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, mesh.normBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(grndNorm), grndNorm, GL_STATIC_DRAW);
   
   glGenBuffers(1, &mesh.uvBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, mesh.uvBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);
   
   mod.bounds = SBoundingBox(0, -0.001, 1);
   mod.bounds.update(0, 0.001, 1);
   mod.meshes.push_back(mesh);
   return mod;
}

GameModel init_Platform()
{
   float Pos[] = {
      -1, -0.25, 0, //0
      -1, -0.25, 1, //1
      1, -0.25, 1,  //2
      1, -0.25, 0,  //3
      -1, 0.25, 0,  //4
      -1, 0.25, 1,  //5
      1, 0.25, 1,   //6
      1, 0.25, 0,   //7
   };
   
   unsigned short idx[] =
   {
      2, 1, 0, //Bottom face
      3, 2, 0,
      4, 5, 6, //top face
      7, 4, 6,
      4, 0, 1, //Left
      5, 4, 1,
      6, 3, 2, //Right
      7, 3, 6,
      5, 2, 1, //Front
      6, 2, 5,
      4, 3, 0, //Back
      7, 3, 4
   };
   
   float Norm[] =
   {
      0, 1, 0,
      0, 1, 0,
      1, 0, 0,
      1, 0, 0,
      0, 0, 1,
      0, 0, 1, //here
      0, 1, 0,
      0, 1, 0,
      1, 0, 0,
      1, 0, 0,
      0, 0, 1,
      0, 0, 1,
   };
   
   static GLfloat Tex[] = {
      0, 0,
      0, 1,
      1, 0,
      1, 1
   };
   
   GameModel mod;
   ModelMesh mesh;
   
   mod = GameModel(Model(), 1, "platform");
   mesh = ModelMesh(0,0,0,0, sizeof(idx)/sizeof(float));
   mesh.numFaces = 36;

   glGenBuffers(1, &mesh.posBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, mesh.posBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(Pos), Pos, GL_STATIC_DRAW);
   
   glGenBuffers(1, &mesh.idxBuffObj);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.idxBuffObj);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
   
   glGenBuffers(1, &mesh.normBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, mesh.normBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(Norm), Norm, GL_STATIC_DRAW);
   
   glGenBuffers(1, &mesh.uvBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, mesh.uvBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(Tex), Tex, GL_STATIC_DRAW);
   
   mod.bounds = SBoundingBox(-1, -0.5, 1);
   mod.bounds.update(-1, -.5, 1);
   mod.meshes.push_back(mesh);
   return mod;
}

GameModel init_Mountain()
{
   float Pos[] = {
      -30, 0, -30, //0
      -30, 0, 30,  //1
      30, 0, 30,   //2
      30, 0, -30,  //3
      0, 30, 0     //4
   };
   
   unsigned short idx[] =
   {
      2, 1, 0, //Bottom face
      3, 2, 0,
      4, 1, 0, //Left
      4, 2, 1, //Front
      4, 3, 2, //Right
      4, 0, 3 //Back
   };
   
   float Norm[] =
   {
      0, -1, 0,
      0, -1, 0,
      -1, 1, 0,
      0, 1, 1,
      1, 1, 0,
      0, 1, -1
   };
   
   static GLfloat Tex[] = {
      0, 0,
      0, 1,
      1, 0,
      1, 1
   };
   
   GameModel mod;
   ModelMesh mesh;
   
   mod = GameModel(Model(), 1, "mountain");
   mesh = ModelMesh(0,0,0,0, sizeof(idx)/sizeof(float));
   mesh.numFaces = 36;
   
   glGenBuffers(1, &mesh.posBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, mesh.posBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(Pos), Pos, GL_STATIC_DRAW);
   
   glGenBuffers(1, &mesh.idxBuffObj);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.idxBuffObj);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
   
   glGenBuffers(1, &mesh.normBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, mesh.normBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(Norm), Norm, GL_STATIC_DRAW);
   
   glGenBuffers(1, &mesh.uvBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, mesh.uvBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(Tex), Tex, GL_STATIC_DRAW);
   
   mod.bounds = SBoundingBox(-30, 30, 30);
   mod.bounds.update(-30, 30, 30);
   mod.meshes.push_back(mesh);
   return mod;
}

/* Initialization of objects in the world */
void setWorld()
{
   groundTiles.clear();
   for(int i = 0; i < g_groundSize; i++)
   {
      for(int j = 0; j < g_groundSize; j++)
         groundTiles.push_back(glm::vec3(i, g_groundY, j));
   }
   
   //Add mountain
   mount = Mountain(glm::vec3(g_groundSize / 2, 0, g_groundSize / 2), handles, mountMod);
   
   lightPos= glm::vec3(5, 5, 0);
   
   //Send light data to shader
   safe_glUniform3f(handles.uLightColor, lightPos.x, lightPos.y, lightPos.z);
   safe_glUniform3f(handles.uLightColor, 1, 1, 1);
   
   //Read in a level if one is available, we probably want to make
   //this a more complex system of loading/saving
   platforms = Platform::importLevel("mountain.lvl", handles, platMod);
}

/* Set up matrices to place model in the world */
void SetModel(glm::vec3 loc, glm::vec3 size, float rotation) {
   glm::mat4 Scale = glm::scale(glm::mat4(1.0f), size);
   glm::mat4 Trans = glm::translate(glm::mat4(1.0f), loc);
   glm::mat4 Rotate = glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0, 1, 0));
   
   glm::mat4 final = Trans * Rotate * Scale;
   safe_glUniformMatrix4fv(handles.uModelMatrix, glm::value_ptr(final));
   safe_glUniformMatrix4fv(handles.uNormMatrix, glm::value_ptr(glm::vec4(1.0f)));
}

/* Set up matrices for ground plane */
void setGround(glm::vec3 loc)
{
   glm::mat4 ctm = glm::translate(glm::mat4(1.0f), loc);
   safe_glUniformMatrix4fv(handles.uModelMatrix, glm::value_ptr(ctm));
   safe_glUniformMatrix4fv(handles.uNormMatrix, glm::value_ptr(glm::mat4(1.0f)));
}

//Export the current platform layout to a .lvl file
void exportLevel()
{
   string fileName = "mountain.lvl";
   ofstream myfile;
   
   myfile.open (fileName);
   for (std::vector<Platform>::iterator it = platforms.begin(); it != platforms.end(); ++ it) {
      myfile << it->toString();
   }
   myfile.close();
}

/* Initialize the geometry */
void InitGeom() {
   grndMod = init_Ground(g_groundY);
   platMod = init_Platform();
   mountMod = init_Mountain();
}

/*function to help load the shaders (both vertex and fragment */
int InstallShader(const GLchar *vShaderName, const GLchar *fShaderName) {
   GLuint VS; //handles to shader object
   GLuint FS; //handles to frag shader object
   GLint vCompiled, fCompiled, linked; //status of shader
   
   VS = glCreateShader(GL_VERTEX_SHADER);
   FS = glCreateShader(GL_FRAGMENT_SHADER);
   
   //load the source
   glShaderSource(VS, 1, &vShaderName, NULL);
   glShaderSource(FS, 1, &fShaderName, NULL);
   
   //compile shader and print log
   glCompileShader(VS);
   /* check shader status requires helper functions */
   printOpenGLError();
   glGetShaderiv(VS, GL_COMPILE_STATUS, &vCompiled);
   printShaderInfoLog(VS);
   
   //compile shader and print log
   glCompileShader(FS);
   /* check shader status requires helper functions */
   printOpenGLError();
   glGetShaderiv(FS, GL_COMPILE_STATUS, &fCompiled);
   printShaderInfoLog(FS);
   
   if (!vCompiled || !fCompiled) {
      printf("Error compiling either shader %s or %s", vShaderName, fShaderName);
      return 0;
   }
   
   //create a program object and attach the compiled shader
   ShadeProg = glCreateProgram();
   glAttachShader(ShadeProg, VS);
   glAttachShader(ShadeProg, FS);
   
   glLinkProgram(ShadeProg);
   /* check shader status requires helper functions */
   printOpenGLError();
   glGetProgramiv(ShadeProg, GL_LINK_STATUS, &linked);
   printProgramInfoLog(ShadeProg);
   
   glUseProgram(ShadeProg);
   
   /* get handles to attribute and uniform data in shader */
   handles.aPosition = safe_glGetAttribLocation(ShadeProg, "aPosition");
   handles.aNormal = safe_glGetAttribLocation(ShadeProg,	"aNormal");
   handles.uProjMatrix = safe_glGetUniformLocation(ShadeProg, "uProjMatrix");
   handles.uViewMatrix = safe_glGetUniformLocation(ShadeProg, "uViewMatrix");
   handles.uModelMatrix = safe_glGetUniformLocation(ShadeProg, "uModelMatrix");
   handles.uNormMatrix = safe_glGetUniformLocation(ShadeProg, "uNormalMatrix");
   handles.uLightPos = safe_glGetUniformLocation(ShadeProg, "uLightPos");
   handles.uLightColor = safe_glGetUniformLocation(ShadeProg, "uLColor");
   handles.uEyePos = safe_glGetUniformLocation(ShadeProg, "uEyePos");
   handles.uMatAmb = safe_glGetUniformLocation(ShadeProg, "uMat.aColor");
   handles.uMatDif = safe_glGetUniformLocation(ShadeProg, "uMat.dColor");
   handles.uMatSpec = safe_glGetUniformLocation(ShadeProg, "uMat.sColor");
   handles.uMatShine = safe_glGetUniformLocation(ShadeProg, "uMat.shine");
   
   printf("sucessfully installed shader %d\n", ShadeProg);
   return 1;
}

/* helper function to set up material for shading */
void SetMaterial(int i) {
   
   glUseProgram(ShadeProg);
   switch (i) {
      case 0:
         safe_glUniform3f(handles.uMatAmb, 0.2, 0.2, 0.2);
         safe_glUniform3f(handles.uMatDif, 0.4, 0.4, 0.4);
         safe_glUniform3f(handles.uMatSpec, 0.2, 0.2, 0.2);
         safe_glUniform1f(handles.uMatShine, .2);
         break;
      case GROUND_MAT:
         safe_glUniform3f(handles.uMatAmb, 0.1, 0.3, 0.1);
         safe_glUniform3f(handles.uMatDif, 0.1, 0.3, 0.1);
         safe_glUniform3f(handles.uMatSpec, 0.3, 0.3, 0.4);
         safe_glUniform1f(handles.uMatShine, 1.0);
         break;
   }
}

/* Some OpenGL initialization */
void Initialize ()
{
	// Start Of User Initialization
	glClearColor (1.0f, 1.0f, 1.0f, 1.0f);
	// Black Background
 	glClearDepth (1.0f);	// Depth Buffer Setup
 	glDepthFunc (GL_LEQUAL);	// The Type Of Depth Testing
	glEnable (GL_DEPTH_TEST);// Enable Depth Testing
}

/* Main display function */
void Draw (void)
{
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//Start our shader
 	glUseProgram(ShadeProg);
   
   /* set up the projection and camera - do not change */
   SetProjectionMatrix(false);
   SetView();
   
   safe_glUniform3f(handles.uEyePos, eye.x, eye.y, eye.z);
   
   //-------------------------------Ground Plane --------------------------
   safe_glEnableVertexAttribArray(handles.aPosition);
   safe_glEnableVertexAttribArray(handles.aNormal);
   SetMaterial(GROUND_MAT);
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   for (std::vector<glm::vec3>::iterator it = groundTiles.begin(); it != groundTiles.end(); ++ it) {
      setGround(glm::vec3(it->x, it->y, it->z));
      
      glBindBuffer(GL_ARRAY_BUFFER, grndMod.meshes[0].posBuffObj);
      safe_glVertexAttribPointer(handles.aPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grndMod.meshes[0].idxBuffObj);
      
      safe_glEnableVertexAttribArray(handles.aNormal);
      glBindBuffer(GL_ARRAY_BUFFER, grndMod.meshes[0].normBuffObj);
      safe_glVertexAttribPointer(handles.aNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
      
      glDrawElements(GL_TRIANGLES, grndMod.meshes[0].numFaces, GL_UNSIGNED_SHORT, 0);
   }
   
   mount.draw();
   
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   if(placeMode)
   {
      SetMaterial(2);
      placing.draw();
   }
   
   SetMaterial(0);
   for (std::vector<Platform>::iterator it = platforms.begin(); it != platforms.end(); ++ it) {
      it->draw();
   }
   
   //clean up
	safe_glDisableVertexAttribArray(handles.aPosition);
	safe_glDisableVertexAttribArray(handles.aNormal);
   
	//Disable the shader
	glUseProgram(0);
}

/* Reshape - note no scaling as perspective viewing*/
void ReshapeGL (GLFWwindow* window, int width, int height)
{
	g_width = (float)width;
	g_height = (float)height;
	glViewport (0, 0, (GLsizei)(width), (GLsizei)(height));
}

/* Convert pixel x coordinates into world space */
float p2wx(int in_x) {
   //fill in with the correct return value
   float l, r;
   float c, d;
   
   if(g_width > g_height)
   {
      l = -1 * (g_width) / (g_height);
      r = g_width / g_height;
   }
   else{
      l = -1;
      r = 1;
   }
   
   c = (1 - g_width) / (l - r);
   d = ((g_width - 1) / (l - r)) * l;
   
   return (in_x - d) / c;
}

/* Convert pixel y coordinates into world space */
float p2wy(int in_y) {
   float b, t;
   float e, f;
   //flip glut y
   in_y = g_height - in_y;
   
   if(g_width > g_height)
   {
      b = -1;
      t = 1;
   }
   else{
      b = -1 * (g_height) / (g_width);
      t = g_height / g_width;
   }
   e = (1 - g_height) / (b - t);
   f = ((g_height - 1) / (b - t)) * b;
   
   return (in_y - f) / e;
}

/* Tracks mouse movement for the camera */
void mouse(GLFWwindow* window, double x, double y)
{
   glm::vec2 currentPos = glm::vec2(x, y);
   glm::vec2 delta = currentPos - prevMouseLoc;
   
   pitch += delta.y * (pi / g_height);
   yaw += delta.x * (pi / g_width);
   
   if(pitch > (4*pi/9))
      pitch = 4*pi/9;
   else if(pitch < -(4*pi/9))
      pitch = -4*pi/9;
   
   lookAtVec.x = cos(pitch) * cos(yaw) + eye.x;
   lookAtVec.y = sin(pitch) + eye.y;
   lookAtVec.z = cos(pitch)*(cos((pi/2) - yaw)) + eye.z;
   prevMouseLoc = currentPos;
}

static void error_callback(int error, const char* description)
{
   fputs(description, stderr);
}

static void mouseClick(GLFWwindow* window, int button, int action, int mods)
{
   double x, y;
   if(action == GLFW_PRESS)
   {
      if(button == GLFW_MOUSE_BUTTON_1)
      {
         //NOT FULLY IMPLEMENTED
         for (std::vector<Platform>::iterator it = platforms.begin(); it < platforms.end(); ++ it) {
            if(it->detectCollision(lookAtVec))
            {
               placeMode = true;
               placing = *it;
               platforms.erase(it);
               //Need to remove from platforms
            }
         }
      }
   }
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   glm::vec3 delta;
   
   if(action == GLFW_PRESS || action == GLFW_REPEAT)
   {
      switch( key ) {
         //Arrows control platform
         case GLFW_KEY_UP:
            if(placeMode)
               placing.moveUp();
            break;
         case GLFW_KEY_DOWN:
            if(placeMode)
               placing.moveDown();
            break;
         case GLFW_KEY_LEFT:
            if(placeMode)
               placing.moveLeft();
            break;
         case GLFW_KEY_RIGHT:
            if(placeMode)
               placing.moveRight();
            break;
         //Comma & period tilt platforms
         case GLFW_KEY_COMMA:
            placing.setRot(placing.getRot() - 1);
            break;
         case GLFW_KEY_PERIOD:
            placing.setRot(placing.getRot() + 1);
            break;
         //Grow and shrink the platform with + -
         case GLFW_KEY_EQUAL:
            placing.stretch();
            break;
         case GLFW_KEY_MINUS:
            placing.shrink();
            break;
         //WASD controls camera
         case GLFW_KEY_W:
            delta = upV;
            move(delta);
            break;
         case GLFW_KEY_S:
            delta = -upV;
            move(delta);
            break;
         case GLFW_KEY_D:
            delta = glm::normalize(glm::cross(upV, -(lookAtVec - eye))) * 3.0f;
            move(delta);
            break;
         case GLFW_KEY_A:
            delta = glm::normalize(glm::cross(upV, (lookAtVec - eye))) * 3.0f;
            move(delta);
            break;
         //Zoom in with e, out with q
         case GLFW_KEY_E:
            delta = glm::normalize(lookAtVec - eye);
            move(delta);
            break;
         case GLFW_KEY_Q:
            delta = glm::normalize(-(lookAtVec - eye));
            move(delta);
            break;
         case GLFW_KEY_R:
            setWorld();
            break;
         case GLFW_KEY_1:
            if (!placeMode) {
               placing = Platform(lookAtVec, handles, platMod);
               placeMode = true;
            }
            else
               placeMode = false;
            break;
         case GLFW_KEY_ENTER:
            if (placeMode) {
               platforms.push_back(placing);
               placeMode = false;
            }
            break;
         case GLFW_KEY_SPACE:
            exportLevel();
            break;
         case GLFW_KEY_ESCAPE:
            exit( EXIT_SUCCESS );
            break;
      }
   }
}

void move(glm::vec3 delta)
{
   eye += (.1f) * delta;
   lookAtVec += (.1f) * delta;
}

int main( int argc, char *argv[] )
{
   GLFWwindow* window;
   
   g_width = INIT_WIDTH;
   g_height = INIT_HEIGHT;
   
   glfwSetErrorCallback(error_callback);
   
   if (!glfwInit())
      exit(EXIT_FAILURE);
   
   window = glfwCreateWindow(g_width, g_height, "Level Builder", NULL, NULL);
   if (!window)
   {
      glfwTerminate();
      exit(EXIT_FAILURE);
   }
   
   glfwMakeContextCurrent(window);
   glfwSetKeyCallback(window, key_callback);
   glfwSetMouseButtonCallback(window, mouseClick);
   glfwSetCursorPosCallback(window, mouse);
   glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
   glfwSetWindowSizeCallback(window, ReshapeGL);
   
   //test the openGL version
   getGLversion();
   //install the shader
   if (!InstallShader(textFileRead((char *)"Phong_vert.glsl"), textFileRead((char *)"Phong_frag.glsl"))) {
      printf("Error installing shader!\n");
      return 0;
   }
   
   Initialize();
   InitGeom();
   setWorld();
   
   while (!glfwWindowShouldClose(window))
   {
      Draw();
      glfwSwapBuffers(window);
      glfwPollEvents();
   }
   
   glfwDestroyWindow(window);
   
   glfwTerminate();
   exit(EXIT_SUCCESS);
   return 0;
}
