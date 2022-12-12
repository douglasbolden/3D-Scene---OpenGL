#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"

using namespace std; // Uses the standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
	const char* const WINDOW_TITLE = "3D Scene - Douglas Bolden"; // Macro for window title

	// Variables for window width and height
	const int WINDOW_WIDTH = 1200;
	const int WINDOW_HEIGHT = 800;

	// Stores the GL data relative to a given mesh
	struct GLMesh
	{
		GLuint vao;         // Handle for the vertex array object
		GLuint vbo;         // Handle for the vertex buffer object
		GLuint nVertices;    // Number of indices of the mesh
	};

	// Main GLFW window
	GLFWwindow* gWindow = nullptr;
	// Triangle mesh data
	GLMesh gCubeMesh;
	GLMesh gCylinderMesh;
	GLMesh gBackgroundPlaneMesh;
	GLMesh gPlaneMesh;
	GLMesh gPyramidMesh;
	// Texture id
	GLuint gBookBindingTextureId;
	GLuint gCigaretteBoxTextureId;
	GLuint gHeadphoneTextureId;
	GLuint gMetalTextureId;
	GLuint gPaperTextureId;
	GLuint gRugTextureId;
	GLuint gTapeTextureId;
	GLuint gWallTextureId;
	GLuint gWoodTextureId;
	// Shader program
	GLuint gSurfaceProgramId;
	GLuint gLightProgramId;
	Camera gCameraFront(glm::vec3(0.0f, 1.0f, 9.0f));
	Camera gCameraOrtho(glm::vec3(0.0f, 1.25f, 5.0f));
	Camera* g_pCurrentCamera = &gCameraFront;

	// Subject position and scale
	glm::vec3 gCubePosition(5.0f, 0.0f, 0.0f);
	glm::vec3 gCubeScale(3.0f);

	// Texture
	GLuint gTextureId;
	glm::vec2 gUVScale(2.0f, 2.0f);
	GLint gTexWrapMode = GL_REPEAT;

	// mouse positions
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// timing
	float gDeltaTime = 0.0f; // time between current frame and last frame
	float gLastFrame = 0.0f;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
/* Surface Vertex Shader Source Code*/
const GLchar* surfaceVertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 vertexPosition; // VAP position 0 for vertex position data
	layout(location = 1) in vec3 vertexNormal; // VAP position 1 for normals
	layout(location = 2) in vec2 textureCoordinate;

	out vec3 vertexFragmentNormal; // For outgoing normals to fragment shader
	out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
	out vec2 vertexTextureCoordinate;

	//Uniform / Global variables for the  transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main()
	{
		gl_Position = projection * view * model * vec4(vertexPosition, 1.0f); // Transforms vertices into clip coordinates

		vertexFragmentPos = vec3(model * vec4(vertexPosition, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

		vertexFragmentNormal = mat3(transpose(inverse(model))) * vertexNormal; // get normal vectors in world space only and exclude normal translation properties
		vertexTextureCoordinate = textureCoordinate;
	}
);
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Surface Fragment Shader Source Code*/
const GLchar* surfaceFragmentShaderSource = GLSL(440,

	in vec3 vertexFragmentNormal; // For incoming normals
	in vec3 vertexFragmentPos; // For incoming fragment position
	in vec2 vertexTextureCoordinate;

	out vec4 fragmentColor; // For outgoing cube color to the GPU

	// Uniform / Global variables for object color, light color, light position, and camera/view position
	uniform vec3 objectColor;
	uniform vec3 ambientColor;
	uniform vec3 light1Color;
	uniform vec3 light1Position;
	uniform vec3 light2Color;
	uniform vec3 light2Position;
	uniform vec3 viewPosition;
	uniform sampler2D uTexture; // Useful when working with multiple textures
	uniform vec2 uvScale;
	uniform float ambientStrength = 0.1f; // Set ambient or global lighting strength
	uniform float specularIntensity = 0.8f;
	uniform float highlightSize = 16.0f;

	void main()
	{
		/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

		//Calculate Ambient lighting
		vec3 ambient = ambientStrength * ambientColor; // Generate ambient light color

		//**Calculate Diffuse lighting**
		vec3 norm = normalize(vertexFragmentNormal); // Normalize vectors to 1 unit
		vec3 light1Direction = normalize(light1Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
		float impact1 = max(dot(norm, light1Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse1 = impact1 * light1Color; // Generate diffuse light color
		vec3 light2Direction = normalize(light2Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
		float impact2 = max(dot(norm, light2Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse2 = impact2 * light2Color; // Generate diffuse light color

		//**Calculate Specular lighting**
		vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
		vec3 reflectDir1 = reflect(-light1Direction, norm);// Calculate reflection vector
		//Calculate specular component
		float specularComponent1 = pow(max(dot(viewDir, reflectDir1), 0.0), highlightSize);
		vec3 specular1 = specularIntensity * specularComponent1 * light1Color;
		vec3 reflectDir2 = reflect(-light2Direction, norm);// Calculate reflection vector
		//Calculate specular component
		float specularComponent2 = pow(max(dot(viewDir, reflectDir2), 0.0), highlightSize);
		vec3 specular2 = specularIntensity * specularComponent2 * light2Color;

		//**Calculate phong result**
		//Texture holds the color to be used for all three components
		vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);
		vec3 phong1 = (ambient + diffuse1 + specular1) * textureColor.xyz; //objectColor;
		vec3 phong2 = (ambient + diffuse2 + specular2) * textureColor.xyz; //objectColor;

		fragmentColor = vec4(phong1 + phong2, 1.0); // Send lighting results to GPU
	}
);
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Light Object Shader Source Code*/
const GLchar* lightVertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 aPos;

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main()
	{
		gl_Position = projection * view * model * vec4(aPos, 1.0);
	}
);
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Light Object Shader Source Code*/
const GLchar* lightFragmentShaderSource = GLSL(440,
	out vec4 FragColor;

	void main()
	{
		FragColor = vec4(1.0); // set all 4 vector values to 1.0
	}
);
/////////////////////////////////////////////////////////////////////////////////////////////////////////

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char*[], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreatePlaneMesh(GLMesh& mesh);
void UCreatePyramidMesh(GLMesh& mesh);
void UCreateCubeMesh(GLMesh& mesh);
void UCreateCylinderMesh(GLMesh& mesh);
void URender();
void UDestroyMesh(GLMesh& mesh);
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);

// main function. Entry point to the OpenGL program
int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	// Create the mesh
	UCreatePlaneMesh(gPlaneMesh);
	UCreatePyramidMesh(gPyramidMesh);
	UCreateCubeMesh(gCubeMesh);
	UCreateCylinderMesh(gCylinderMesh);

	// Create the shader program
	if (!UCreateShaderProgram(surfaceVertexShaderSource, surfaceFragmentShaderSource, gSurfaceProgramId))
		return EXIT_FAILURE;

	if (!UCreateShaderProgram(lightVertexShaderSource, lightFragmentShaderSource, gLightProgramId))
		return EXIT_FAILURE;

	// Load textures

	const char* texBookBindingFilename = "resources/book_binding.jpg";
	if (!UCreateTexture(texBookBindingFilename, gBookBindingTextureId))
	{
		cout << "Failed to load texture " << texBookBindingFilename << endl;
		return EXIT_FAILURE;
	}

	const char* texCigaretteBoxFilename = "resources/cigarette.jpg";
	if (!UCreateTexture(texCigaretteBoxFilename, gCigaretteBoxTextureId))
	{
		cout << "Failed to load texture " << texCigaretteBoxFilename << endl;
		return EXIT_FAILURE;
	}

	const char* texPaperFilename = "resources/paper.jpg";
	if (!UCreateTexture(texPaperFilename, gPaperTextureId))
	{
		cout << "Failed to load texture " << texPaperFilename << endl;
		return EXIT_FAILURE;
	}

	const char* texCarpetFilename = "resources/rug.jpg";
	if (!UCreateTexture(texCarpetFilename, gRugTextureId))
	{
		cout << "Failed to load texture " << texCarpetFilename << endl;
		return EXIT_FAILURE;
	}

	const char* texWoodFilename = "resources/wood.jpg";
	if (!UCreateTexture(texWoodFilename, gWoodTextureId))
	{
		cout << "Failed to load texture " << texWoodFilename << endl;
		return EXIT_FAILURE;
	}

	const char* texMetalFilename = "resources/metal.jpg";
	if (!UCreateTexture(texMetalFilename, gMetalTextureId))
	{
		cout << "Failed to load texture " << texMetalFilename << endl;
		return EXIT_FAILURE;
	}

	const char* texHeadphoneFilename = "resources/headphone.jpg";
	if (!UCreateTexture(texHeadphoneFilename, gHeadphoneTextureId))
	{
		cout << "Failed to load texture " << texHeadphoneFilename << endl;
		return EXIT_FAILURE;
	}

	const char* texTapeFilename = "resources/tape.jpg";
	if (!UCreateTexture(texTapeFilename, gTapeTextureId))
	{
		cout << "Failed to load texture " << texTapeFilename << endl;
		return EXIT_FAILURE;
	}

	const char* texWallFilename = "resources/wall.jpg";
	if (!UCreateTexture(texWallFilename, gWallTextureId))
	{
		cout << "Failed to load texture " << texWallFilename << endl;
		return EXIT_FAILURE;
	}


	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	glUseProgram(gSurfaceProgramId);
	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gSurfaceProgramId, "uTexture"), 0);
	glUniform2f(glGetUniformLocation(gSurfaceProgramId, "uvScale"), gUVScale.x, gUVScale.y);

	// Sets the background color of the window to black (it will be implicitely used by glClear)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	// render loop
	// -----------
	while (!glfwWindowShouldClose(gWindow))
	{
		// per-frame timing
		// --------------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		// input
		// -----
		UProcessInput(gWindow);

		URender();

		glfwPollEvents();
	}

	// Release mesh data
	UDestroyMesh(gCubeMesh);
	UDestroyMesh(gCylinderMesh);
	UDestroyMesh(gPlaneMesh);
	UDestroyMesh(gPyramidMesh);

	// Release textures
	UDestroyTexture(gBookBindingTextureId);
	UDestroyTexture(gCigaretteBoxTextureId);
	UDestroyTexture(gHeadphoneTextureId);
	UDestroyTexture(gMetalTextureId);
	UDestroyTexture(gPaperTextureId);
	UDestroyTexture(gRugTextureId);
	UDestroyTexture(gTapeTextureId);
	UDestroyTexture(gWallTextureId);
	UDestroyTexture(gWoodTextureId);


	UDestroyShaderProgram(gSurfaceProgramId);
	UDestroyShaderProgram(gLightProgramId);

	exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
	// GLFW: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// GLFW: window creation
	// ---------------------
	* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMouseButtonCallback);
	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLEW: initialize
	// ----------------
	// Note: if using GLEW version 1.13 or earlier
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	// Displays GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
	const static float cameraSpeed = 3.5f;
	// To close application
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	// forward movement
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		g_pCurrentCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	// backward movement
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		g_pCurrentCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	// left movement
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		g_pCurrentCamera->ProcessKeyboard(LEFT, gDeltaTime);
	// right movement
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		g_pCurrentCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	// up movement
	if ((glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS))
		g_pCurrentCamera->ProcessKeyboard(UP, gDeltaTime);                                                    // MINECRAFT FLYING FOR THE WIN
	//down movement
	if ((glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS))
		g_pCurrentCamera->ProcessKeyboard(DOWN, gDeltaTime);

	//Hold P for Orthogonal View
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		g_pCurrentCamera = &gCameraOrtho;

	}
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
		g_pCurrentCamera = &gCameraFront;
		gCameraOrtho = glm::vec3(0.0f, 1.25f, 5.0f);
	}
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

	gLastX = xpos;
	gLastY = ypos;

	g_pCurrentCamera->ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	g_pCurrentCamera->ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		if (action == GLFW_PRESS)
			cout << "Left mouse button pressed" << endl;
		else
			cout << "Left mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
	{
		if (action == GLFW_PRESS)
			cout << "Middle mouse button pressed" << endl;
		else
			cout << "Middle mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		if (action == GLFW_PRESS)
			cout << "Right mouse button pressed" << endl;
		else
			cout << "Right mouse button released" << endl;
	}
	break;

	default:
		cout << "Unhandled mouse button event" << endl;
		break;
	}
}

void URender()
{

	GLint modelLoc;
	GLint viewLoc;
	GLint projLoc;
	GLint viewPosLoc;
	GLint ambStrLoc;
	GLint ambColLoc;
	GLint light1ColLoc;
	GLint light1PosLoc;
	GLint light2ColLoc;
	GLint light2PosLoc;
	GLint objColLoc;
	GLint specIntLoc;
	GLint highlghtSzLoc;
	glm::mat4 scale;
	glm::mat4 rotation;
	glm::mat4 translation;
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	glm::vec3 location;
	glm::vec3 loc1;
	glm::vec3 loc2;

	glEnable(GL_DEPTH_TEST);

	// Clear the background
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	view = g_pCurrentCamera->GetViewMatrix();
	projection = glm::perspective(glm::radians(g_pCurrentCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

	// Set the shader to be used
	glUseProgram(gSurfaceProgramId);

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(gSurfaceProgramId, "model");
	viewLoc = glGetUniformLocation(gSurfaceProgramId, "view");
	projLoc = glGetUniformLocation(gSurfaceProgramId, "projection");
	viewPosLoc = glGetUniformLocation(gSurfaceProgramId, "viewPosition");
	ambStrLoc = glGetUniformLocation(gSurfaceProgramId, "ambientStrength");
	ambColLoc = glGetUniformLocation(gSurfaceProgramId, "ambientColor");
	light1ColLoc = glGetUniformLocation(gSurfaceProgramId, "light1Color");
	light1PosLoc = glGetUniformLocation(gSurfaceProgramId, "light1Position");
	light2ColLoc = glGetUniformLocation(gSurfaceProgramId, "light2Color");
	light2PosLoc = glGetUniformLocation(gSurfaceProgramId, "light2Position");
	objColLoc = glGetUniformLocation(gSurfaceProgramId, "objectColor");
	specIntLoc = glGetUniformLocation(gSurfaceProgramId, "specularIntensity");
	highlghtSzLoc = glGetUniformLocation(gSurfaceProgramId, "highlightSize");

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the camera view location
	glUniform3f(viewPosLoc, g_pCurrentCamera->Position.x, g_pCurrentCamera->Position.y, g_pCurrentCamera->Position.z);
	//set ambient lighting strength
	glUniform1f(ambStrLoc, 1.0f);
	//set ambient color
	glUniform3f(ambColLoc, 0.2f, 0.2f, 0.2f);
	glUniform3f(light1ColLoc, 1.0f, 1.0f, 1.0f);	//white

	//UPDATE BOTH glUniform3f and loc1 FOR EASE OF ACCESS
	glUniform3f(light1PosLoc, -20.0f, 18.0f, 23.0f);
	loc1 = glm::vec3(-20.0f, 18.0f, 23.0f);

	glUniform3f(light2ColLoc, 0.1f, 0.2f, 0.0f);	//green-ish

	//UPDATE BOTH glUniform3f and loc2 FOR EASE OF ACCESS
	glUniform3f(light2PosLoc, 0.0f, 10.0f, -12.0f);
	loc2 = glm::vec3(0.0f, 10.0f, -12.0f);
	//set specular intensity
	glUniform1f(specIntLoc, 1.0f);
	//set specular highlight size
	glUniform1f(highlghtSzLoc, 2.0f);


	//LIGHT INDICATORS	
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(gPlaneMesh.vao);

	//White light
	scale = glm::scale(glm::vec3(0.1f, 0.1f, 0.1f));
	rotation = glm::rotate(1.0f, glm::vec3(-0.5f, 0.1f, 0.1f));
	translation = glm::translate(loc1);
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawArrays(GL_TRIANGLES, 0, gPlaneMesh.nVertices);


	//Greenish light
	scale = glm::scale(glm::vec3(0.1f, 0.1f, 0.1f));
	rotation = glm::rotate(1.0f, glm::vec3(0.0, 0.0f, -0.25f));
	translation = glm::translate(loc2);
	model = translation * rotation * scale;

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawArrays(GL_TRIANGLES, 0, gPlaneMesh.nVertices);

	glBindVertexArray(0);


	//CARPET

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gRugTextureId);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(gPlaneMesh.vao);
	scale = glm::scale(glm::vec3(10.0f, 1.0f, 10.0f));
	rotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(0.0f, -3.58f, 0.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the plane for my rug.
	glDrawArrays(GL_TRIANGLES, 0, gPlaneMesh.nVertices);

	glBindVertexArray(0);

	// BOOK
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(gCylinderMesh.vao);

	//PAPER CYLINDERS

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPaperTextureId);

	//paper curve top
	scale = glm::scale(glm::vec3(1.0f, 0.2f, 0.2f));
	rotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(0.0f, 0.0f, -0.026f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the top and bottom of the paper cylinder only
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72);
	// Draw the circumference of the paper cylinder only
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 180);


	//paper curve bottom
	rotation = glm::rotate(3.12f, glm::vec3(0.0f, 1.0f, 0.0f));
	location = glm::vec3(0.0f, 0.0f, 2.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the top and bottom of the paper cylinder
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72);
	// Draw the circumference of the paper cylinder
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 180);


	//LEATHER CYLINDERS

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBookBindingTextureId);

	//leather curve top TOP
	scale = glm::scale(glm::vec3(1.05f, 0.018f, 0.25f));
	rotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(0.0f, 0.2005f, 0.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the top and bottom of the leather cylinder
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72);
	// Draw the circumference of the leather cylinder
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 180);


	//leather curve top BOTTOM
	scale = glm::scale(glm::vec3(1.05f, 0.019f, 0.25f));
	rotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(0.0f, -0.01825f, 0.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the top and bottom of the leather cylinder
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72);
	// Draw the circumference of the leather cylinder
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 180);


	//leather curve bottom TOP
	scale = glm::scale(glm::vec3(1.05f, 0.018f, 0.25f));
	rotation = glm::rotate(3.1f, glm::vec3(0.0f, 1.0f, 0.0f));
	location = glm::vec3(0.0f, 0.2005f, 2.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the top and bottom of the leather cylinder
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72);
	// Draw the circumference of the leather cylinder
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 180);


	//leather curve bottom BOTTOM
	scale = glm::scale(glm::vec3(1.05f, 0.019f, 0.25f));
	rotation = glm::rotate(3.1f, glm::vec3(0.0f, 1.0f, 0.0f));
	location = glm::vec3(0.0f, -0.01825f, 2.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the top and bottom of the leather cylinder
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72);
	// Draw the circumference of the leather cylinder
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 180);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);



	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(gCubeMesh.vao);

	// leather book TOP FACE
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(2.05f, 0.01705f, 2.05f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(0.025f, 0.21f, 1.02f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the top face of the book
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	// leather book BOTTOM FACE
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(2.05f, 0.01875f, 2.05f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(0.025f, -0.009f, 1.02f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the bottom face of the book
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	// leather book TOP FACE @ spine
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(1.36f, 0.018f, 2.48f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(-0.38f, 0.2096f, 1.005f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the top face of the book
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	// leather book TOP BOTTOM @ spine
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(1.36f, 0.018f, 2.48f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(-0.38f, -0.009f, 1.005f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the top face of the book
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	// leather book SPINE
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(0.02f, 0.235f, 2.481f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(-1.05f, 0.1f, 1.005f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the spine of the book
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPaperTextureId);

	// book spine paper additive
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(1.0f, 0.21f, 2.4f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(-0.55f, 0.1f, 1.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the spine paper
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	// paper
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(1.0f, 0.21f, 2.05f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(0.5f, 0.1f, 1.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the paper body
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	// Table
	
	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gWoodTextureId);

	//Table TOP 1/4
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(7.0f, 0.4f, 1.4f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(0.0f, -0.2185f, 2.82f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);
	 

	//Table TOP 2/4
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(7.0f, 0.4f, 1.4f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(0.0f, -0.2185f, 1.41f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table TOP 3/4
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(7.0f, 0.4f, 1.4f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(0.0f, -0.2185f, 0.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table TOP 4/4
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(7.0f, 0.4f, 1.4f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(0.0f, -0.2185f, -1.41f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table Under-Support Left (smaller)
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(5.15f, 0.3f, 0.6f));
	rotation = glm::rotate(1.575f, glm::vec3(0.0f, 1.0f, 0.0f));
	location = glm::vec3(-1.55f, -0.57f, 0.725f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table Under-Support Right (smaller)
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(5.15f, 0.3f, 0.6f));
	rotation = glm::rotate(1.575f, glm::vec3(0.0f, 1.0f, 0.0f));
	location = glm::vec3(1.6f, -0.57f, 0.725f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table Under-Support Left (larger)
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(4.25f, 0.4f, 0.9f));
	rotation = glm::rotate(1.575f, glm::vec3(0.0f, 1.0f, 0.0f));
	location = glm::vec3(-2.7f, -0.625f, 0.725f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table Under-Support Right (larger)
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(4.25f, 0.4f, 0.9f));
	rotation = glm::rotate(1.575f, glm::vec3(0.0f, 1.0f, 0.0f));
	location = glm::vec3(2.75f, -0.625f, 0.725f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table Support VERTICAL Left 1/2
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(2.75f, 0.4f, 0.9f));
	rotation = glm::rotate(1.575f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(-2.935f, -2.2f, 2.4f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table Support VERTICAL Left 2/2
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(2.75f, 0.4f, 0.9f));
	rotation = glm::rotate(1.575f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(-2.95f, -2.2f, -0.95f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table Support VERTICAL Right 1/2
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(2.75f, 0.4f, 0.9f));
	rotation = glm::rotate(1.575f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(3.0125f, -2.2f, 2.4f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table Support VERTICAL Right 2/2
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(2.75f, 0.4f, 0.9f));
	rotation = glm::rotate(1.575f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(3.0f, -2.2f, -0.95f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table Support Right Legs Bottom Support
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(0.9f, 0.35f, 4.25f));
	rotation = glm::rotate(1.575f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(3.39f, -3.1225f, 0.7235f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table Support Left Legs Bottom Support
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(0.9f, 0.35f, 4.25f));
	rotation = glm::rotate(1.575f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(-3.31f, -3.1225f, 0.7235f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table Middle Support Front
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(5.65f, 0.9f, 0.4f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(0.05f, -2.2f, 2.4f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	//Table Middle Support Front
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(5.65f, 0.9f, 0.4f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(0.05f, -2.2f, -0.95f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the Table Piece
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(gCylinderMesh.vao);

	//HEADPHONES

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gHeadphoneTextureId);

	//Headphones 
	scale = glm::scale(glm::vec3(0.35f, 0.25f, 0.2f));
	rotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(0.0f, 0.215f, 0.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the top and bottom of the curve
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72);
	// Draw the circumference of the curve
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 180);
	

	// Medical Tape
	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTapeTextureId);

	scale = glm::scale(glm::vec3(0.15f, 0.23f, 0.15f));
	rotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(0.55f, 0.2f, 0.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the Medical Tape
	// Draw the circumference of the Tape
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 180);


	//WALL

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gWallTextureId);

	//North Wall
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(gPlaneMesh.vao);
	scale = glm::scale(glm::vec3(10.0f, 1.0f, 10.0f));
	rotation = glm::rotate(1.575f, glm::vec3(1.0f, 0.0f, 0.0f));
	location = glm::vec3(0.0f, 6.4f, -10.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the plane for the wall.
	glDrawArrays(GL_TRIANGLES, 0, gPlaneMesh.nVertices);

	//East Wall
	scale = glm::scale(glm::vec3(10.0f, 1.0f, 10.0f));
	rotation = glm::rotate(1.575f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(9.9f, 6.4f, -0.2f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the plane for the wall.
	glDrawArrays(GL_TRIANGLES, 0, gPlaneMesh.nVertices);

	//West Wall
	scale = glm::scale(glm::vec3(10.0f, 1.0f, 10.0f));
	rotation = glm::rotate(1.575f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(-9.9f, 6.4f, -0.2f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the plane for the wall.
	glDrawArrays(GL_TRIANGLES, 0, gPlaneMesh.nVertices);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	//Nail

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(gCylinderMesh.vao);

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gMetalTextureId);

	//Nail Head 
	scale = glm::scale(glm::vec3(0.02f, 0.002f, 0.02f));
	rotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(-0.5f, 0.218f, 0.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the top and bottom of the curve
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72);
	// Draw the circumference of the curve
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 180);

	glBindVertexArray(0);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(gPyramidMesh.vao);

	//Nail Body
	scale = glm::scale(glm::vec3(0.004f, 0.05f, 0.004f));
	rotation = glm::rotate(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	location = glm::vec3(-0.5f, 0.24f, 0.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the plane for the wall.
	glDrawArrays(GL_TRIANGLES, 0, gPyramidMesh.nVertices);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(gCubeMesh.vao);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gCigaretteBoxTextureId);

	// Cigarette Box
	// Manipulate the mesh matrix
	scale = glm::scale(glm::vec3(0.5f, 0.21f, 0.8f));
	rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
	location = glm::vec3(0.0f, 0.32f, 1.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the top face of the book
	glDrawArrays(GL_TRIANGLES, 0, gCubeMesh.nVertices);


	glUseProgram(0);

	// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
	glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}

// Implements the UCreateMesh function
void UCreatePlaneMesh(GLMesh& mesh)
{
	// Vertex data
	GLfloat verts[] = {
		//Positions          //Normals
		// ------------------------------------------------------
		//Face					//Negative Z Normal		//Texture Coords. 
		-1.0f,  0.0f, -1.0f,	0.0f,  1.0f,  0.0f,		0.0f, 1.0f,
		-1.0f, 0.0f, 1.0f,		0.0f,  1.0f,  0.0f,		0.0f, 0.0f,
		1.0f, 0.0f, 1.0f,		0.0f,  1.0f,  0.0f,		1.0f, 0.0f,
		-1.0f,  0.0f, -1.0f,	0.0f,  1.0f,  0.0f,		0.0f, 1.0f,
		1.0f,  0.0f, -1.0f,		0.0f,  1.0f,  0.0f,		1.0f, 1.0f,
		1.0f, 0.0f, 1.0f,		0.0f,  1.0f,  0.0f,		1.0f, 0.0f,
		-1.0f, 0.0f, 1.0f,		0.0f,  1.0f,  0.0f,		0.0f, 0.0f
	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU


	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}


void UCreatePyramidMesh(GLMesh& mesh)
{
	// Vertex data
	GLfloat verts[] = {
		// Vertex Positions
		0.0f, 0.5f, 0.0f,		0.0f, 0.0f, 0.0f,	0.5f, 1.0f,
		-0.5f, -0.5f, -0.5f,	0.0f, 0.0f, 0.0f,	0.0f, 0.0f,
		0.5f, -0.5f, -0.5f,		0.0f, 0.0f, 0.0f,	1.0f, 0.0f,

		0.0f, 0.5f, 0.0f,		0.0f, 0.0f, 0.0f,	0.5f, 1.0f,
		0.5f, -0.5f, -0.5f,		0.0f, 0.0f, 0.0f,	1.0f, 0.0f,
		0.5f,  -0.5f, 0.5f,		0.0f, 0.0f, 0.0f,	1.0f, 1.0f,

		0.0f, 0.5f, 0.0f,		0.0f, 0.0f, 0.0f,	0.5f, 1.0f,
		0.5f,  -0.5f, 0.5f,		0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
		-0.5f,  -0.5f, 0.5f,	0.0f, 0.0f, 0.0f,	0.0f, 1.0f,

		0.0f, 0.5f, 0.0f,		0.0f, 0.0f, 0.0f,	0.5f, 1.0f,
		-0.5f,  -0.5f, 0.5f,	0.0f, 0.0f, 0.0f,	0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,	0.0f, 0.0f, 0.0f,	0.0f, 0.0f,
	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}


void UCreateCubeMesh(GLMesh& mesh)
{
	// Vertex data
	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float verts[] = {
		// positions          // normals           // texture coords
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f

	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}



void UCreateCylinderMesh(GLMesh& mesh)
{
	GLfloat verts[] = {


		// cylinder bottom		// normals			// texture coords
		1.0f, 0.0f, 0.0f,		0.0f, -1.0f, 0.0f,	0.5f,0.0f,
		.98f, 0.0f, -0.17f,		0.0f, -1.0f, 0.0f,	0.6f, 0.017f,
		.94f, 0.0f, -0.34f,		0.0f, -1.0f, 0.0f,	0.68f, 0.04f,
		.87f, 0.0f, -0.5f,		0.0f, -1.0f, 0.0f,	0.77f, 0.08f,
		.77f, 0.0f, -0.64f,		0.0f, -1.0f, 0.0f,	0.83f, 0.13f,
		.64f, 0.0f, -0.77f,		0.0f, -1.0f, 0.0f,	0.88f, 0.17f,
		.5f, 0.0f, -0.87f,		0.0f, -1.0f, 0.0f,	0.93f, 0.25f,
		.34f, 0.0f, -0.94f,		0.0f, -1.0f, 0.0f,	0.97f, 0.33f,
		.17f, 0.0f, -0.98f,		0.0f, -1.0f, 0.0f,	0.99f, 0.41f,
		0.0f, 0.0f, -1.0f,		0.0f, -1.0f, 0.0f,	1.0f,0.5f,
		-.17f, 0.0f, -0.98f,	0.0f, -1.0f, 0.0f,	0.99f, 0.41f,
		-.34f, 0.0f, -0.94f,	0.0f, -1.0f, 0.0f,	0.97f, 0.33f,
		-.5f, 0.0f, -0.87f,		0.0f, -1.0f, 0.0f,	0.93f, 0.25f,
		-.64f, 0.0f, -0.77f,	0.0f, -1.0f, 0.0f,	0.88f, 0.17f,
		-.77f, 0.0f, -0.64f,	0.0f, -1.0f, 0.0f,	0.83f, 0.13f,
		-.87f, 0.0f, -0.5f,		0.0f, -1.0f, 0.0f,	0.77f, 0.08f,
		-.94f, 0.0f, -0.34f,	0.0f, -1.0f, 0.0f,	0.68f, 0.04f,
		-.98f, 0.0f, -0.17f,	0.0f, -1.0f, 0.0f,	0.6f, 0.017f,
		-1.0f, 0.0f, 0.0f,		0.0f, -1.0f, 0.0f,	0.5f, 1.0f,
		-.98f, 0.0f, 0.17f,		0.0f, -1.0f, 0.0f,	0.41f, 0.99f,
		-.94f, 0.0f, 0.34f,		0.0f, -1.0f, 0.0f,	0.33f, 0.97f,
		-.87f, 0.0f, 0.5f,		0.0f, -1.0f, 0.0f,	0.25f, 0.93f,
		-.77f, 0.0f, 0.64f,		0.0f, -1.0f, 0.0f,	0.17f, 0.88f,
		-.64f, 0.0f, 0.77f,		0.0f, -1.0f, 0.0f,	0.13f, 0.83f,
		-.5f, 0.0f, 0.87f,		0.0f, -1.0f, 0.0f,	0.08f, 0.77f,
		-.34f, 0.0f, 0.94f,		0.0f, -1.0f, 0.0f,	0.04f, 0.68f,
		-.17f, 0.0f, 0.98f,		0.0f, -1.0f, 0.0f,	0.017f, 0.6f,
		0.0f, 0.0f, 1.0f,		0.0f, -1.0f, 0.0f,	0.0f, 0.5f,
		.17f, 0.0f, 0.98f,		0.0f, -1.0f, 0.0f,	0.017f, 0.41f,
		.34f, 0.0f, 0.94f,		0.0f, -1.0f, 0.0f,	0.04f, 0.33f,
		.5f, 0.0f, 0.87f,		0.0f, -1.0f, 0.0f,	0.08f, 0.25f,
		.64f, 0.0f, 0.77f,		0.0f, -1.0f, 0.0f,	0.13f, 0.17f,
		.77f, 0.0f, 0.64f,		0.0f, -1.0f, 0.0f,	0.17f, 0.13f,
		.87f, 0.0f, 0.5f,		0.0f, -1.0f, 0.0f,	0.25f, 0.08f,
		.94f, 0.0f, 0.34f,		0.0f, -1.0f, 0.0f,	0.33f, 0.04f,
		.98f, 0.0f, 0.17f,		0.0f, -1.0f, 0.0f,	0.41f, 0.017f,

		// cylinder top			// normals			// texture coords
		1.0f, 1.0f, 0.0f,		0.0f, 1.0f, 0.0f,	0.5f,0.0f,
		.98f, 1.0f, -0.17f,		0.0f, 1.0f, 0.0f,	0.6f, 0.017f,
		.94f, 1.0f, -0.34f,		0.0f, 1.0f, 0.0f,	0.68f, 0.04f,
		.87f, 1.0f, -0.5f,		0.0f, 1.0f, 0.0f,	0.77f, 0.08f,
		.77f, 1.0f, -0.64f,		0.0f, 1.0f, 0.0f,	0.83f, 0.13f,
		.64f, 1.0f, -0.77f,		0.0f, 1.0f, 0.0f,	0.88f, 0.17f,
		.5f, 1.0f, -0.87f,		0.0f, 1.0f, 0.0f,	0.93f, 0.25f,
		.34f, 1.0f, -0.94f,		0.0f, 1.0f, 0.0f,	0.97f, 0.33f,
		.17f, 1.0f, -0.98f,		0.0f, 1.0f, 0.0f,	0.99f, 0.41f,
		0.0f, 1.0f, -1.0f,		0.0f, 1.0f, 0.0f,	1.0f,0.5f,
		-.17f, 1.0f, -0.98f,	0.0f, 1.0f, 0.0f,	0.99f, 0.41f,
		-.34f, 1.0f, -0.94f,	0.0f, 1.0f, 0.0f,	0.97f, 0.33f,
		-.5f, 1.0f, -0.87f,		0.0f, 1.0f, 0.0f,	0.93f, 0.25f,
		-.64f, 1.0f, -0.77f,	0.0f, 1.0f, 0.0f,	0.88f, 0.17f,
		-.77f, 1.0f, -0.64f,	0.0f, 1.0f, 0.0f,	0.83f, 0.13f,
		-.87f, 1.0f, -0.5f,		0.0f, 1.0f, 0.0f,	0.77f, 0.08f,
		-.94f, 1.0f, -0.34f,	0.0f, 1.0f, 0.0f,	0.68f, 0.04f,
		-.98f, 1.0f, -0.17f,	0.0f, 1.0f, 0.0f,	0.6f, 0.017f,
		-1.0f, 1.0f, 0.0f,		0.0f, 1.0f, 0.0f,	0.5f, 1.0f,
		-.98f, 1.0f, 0.17f,		0.0f, 1.0f, 0.0f,	0.41f, 0.99f,
		-.94f, 1.0f, 0.34f,		0.0f, 1.0f, 0.0f,	0.33f, 0.97f,
		-.87f, 1.0f, 0.5f,		0.0f, 1.0f, 0.0f,	0.25f, 0.93f,
		-.77f, 1.0f, 0.64f,		0.0f, 1.0f, 0.0f,	0.17f, 0.88f,
		-.64f, 1.0f, 0.77f,		0.0f, 1.0f, 0.0f,	0.13f, 0.83f,
		-.5f, 1.0f, 0.87f,		0.0f, 1.0f, 0.0f,	0.08f, 0.77f,
		-.34f, 1.0f, 0.94f,		0.0f, 1.0f, 0.0f,	0.04f, 0.68f,
		-.17f, 1.0f, 0.98f,		0.0f, 1.0f, 0.0f,	0.017f, 0.6f,
		0.0f, 1.0f, 1.0f,		0.0f, 1.0f, 0.0f,	0.0f, 0.5f,
		.17f, 1.0f, 0.98f,		0.0f, 1.0f, 0.0f,	0.017f, 0.41f,
		.34f, 1.0f, 0.94f,		0.0f, 1.0f, 0.0f,	0.04f, 0.33f,
		.5f, 1.0f, 0.87f,		0.0f, 1.0f, 0.0f,	0.08f, 0.25f,
		.64f, 1.0f, 0.77f,		0.0f, 1.0f, 0.0f,	0.13f, 0.17f,
		.77f, 1.0f, 0.64f,		0.0f, 1.0f, 0.0f,	0.17f, 0.13f,
		.87f, 1.0f, 0.5f,		0.0f, 1.0f, 0.0f,	0.25f, 0.08f,
		.94f, 1.0f, 0.34f,		0.0f, 1.0f, 0.0f,	0.33f, 0.04f,
		.98f, 1.0f, 0.17f,		0.0f, 1.0f, 0.0f,	0.41f, 0.017f,

		// cylinder body		// normals			// texture coords
		1.0f, 1.0f, 0.0f,	1.0f, 0.0f, 0.0f,	0.0,1.0,
		1.0f, 0.0f, 0.0f,	1.0f, 0.0f, 0.0f,	0.0,0.0,
		.98f, 0.0f, -0.17f,	1.0f, 0.0f, 0.0f,	0.027,0.0,
		1.0f, 1.0f, 0.0f,	0.92f, 0.0f, -0.08f,	0.0,1.0,
		.98f, 1.0f, -0.17f,	0.92f, 0.0f, -0.08f,	0.027,1.0,
		.98f, 0.0f, -0.17f,	0.92f, 0.0f, -0.08f,	0.027,0.0,
		.94f, 0.0f, -0.34f,	0.83f, 0.0f, -0.17f,	0.054,0.0,
		.98f, 1.0f, -0.17f,	0.83f, 0.0f, -0.17f,	0.027,1.0,
		.94f, 1.0f, -0.34f,	0.83f, 0.0f, -0.17f,	0.054,1.0,
		.94f, 0.0f, -0.34f,	0.75f, 0.0f, -0.25f,	0.054,0.0,
		.87f, 0.0f, -0.5f,	0.75f, 0.0f, -0.25f,	0.081,0.0,
		.94f, 1.0f, -0.34f,	0.75f, 0.0f, -0.25f,	0.054,1.0,
		.87f, 1.0f, -0.5f,	0.67f, 0.0f, -0.33f,	0.081,1.0,
		.87f, 0.0f, -0.5f,	0.67f, 0.0f, -0.33f,	0.081,0.0,
		.77f, 0.0f, -0.64f,	0.67f, 0.0f, -0.33f,	0.108,0.0,
		.87f, 1.0f, -0.5f,	0.58f, 0.0f, -0.42f,	0.081,1.0,
		.77f, 1.0f, -0.64f,	0.58f, 0.0f, -0.42f,	0.108,1.0,
		.77f, 0.0f, -0.64f,	0.58f, 0.0f, -0.42f,	0.108,0.0,
		.64f, 0.0f, -0.77f,	0.5f, 0.0f, -0.5f,	0.135,0.0,
		.77f, 1.0f, -0.64f,	0.5f, 0.0f, -0.5f,	0.108,1.0,
		.64f, 1.0f, -0.77f,	0.5f, 0.0f, -0.5f,	0.135,1.0,
		.64f, 0.0f, -0.77f,	0.42f, 0.0f, -0.58f,	0.135,0.0,
		.5f, 0.0f, -0.87f,	0.42f, 0.0f, -0.58f,	0.162,0.0,
		.64f, 1.0f, -0.77f,	0.42f, 0.0f, -0.58f,	0.135, 1.0,
		.5f, 1.0f, -0.87f,  0.33f, 0.0f, -0.67f, 0.162, 1.0,
		.5f, 0.0f, -0.87f,  0.33f, 0.0f, -0.67f, 0.162, 0.0,
		.34f, 0.0f, -0.94f, 0.33f, 0.0f, -0.67f, 0.189, 0.0,
		.5f, 1.0f, -0.87f,  0.25f, 0.0f, -0.75f, 0.162, 1.0,
		.34f, 1.0f, -0.94f, 0.25f, 0.0f, -0.75f, 0.189, 1.0,
		.34f, 0.0f, -0.94f, 0.25f, 0.0f, -0.75f, 0.189, 0.0,
		.17f, 0.0f, -0.98f, 0.17f, 0.0f, -0.83f, 0.216, 0.0,
		.34f, 1.0f, -0.94f, 0.17f, 0.0f, -0.83f, 0.189, 1.0,
		.17f, 1.0f, -0.98f, 0.17f, 0.0f, -0.83f, 0.216, 1.0,
		.17f, 0.0f, -0.98f, 0.08f, 0.0f, -0.92f, 0.216, 0.0,
		0.0f, 0.0f, -1.0f,  0.08f, 0.0f, -0.92f, 0.243, 0.0,
		.17f, 1.0f, -0.98f, 0.08f, 0.0f, -0.92f, 0.216, 1.0,
		0.0f, 1.0f, -1.0f,  0.0f, 0.0f, -1.0f, 0.243, 1.0,
		0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f, 0.243, 0.0,
		-.17f, 0.0f, -0.98f, 0.0f, 0.0f, -1.0f, 0.270, 0.0,
		0.0f, 1.0f, -1.0f, 0.08f, 0.0f, -1.08f, 0.243, 1.0,
		-.17f, 1.0f, -0.98f, -0.08f, 0.0f, -0.92f, 0.270, 1.0,
		-.17f, 0.0f, -0.98f, -0.08f, 0.0f, -0.92f, 0.270, 0.0,
		-.34f, 0.0f, -0.94f, -0.08f, 0.0f, -0.92f, 0.297, 0.0,
		-.17f, 1.0f, -0.98f, -0.08f, 0.0f, -0.92f, 0.270, 1.0,
		-.34f, 1.0f, -0.94f, -0.17f, 0.0f, -0.83f, 0.297, 1.0,
		-.34f, 0.0f, -0.94f, -0.17f, 0.0f, -0.83f, 0.297, 0.0,
		-.5f, 0.0f, -0.87f, -0.17f, 0.0f, -0.83f, 0.324, 0.0,
		-.34f, 1.0f, -0.94f, -0.25f, 0.0f, -0.75f, 0.297, 1.0,
		-.5f, 1.0f, -0.87f, -0.25f, 0.0f, -0.75f, 0.324, 1.0,
		-.5f, 0.0f, -0.87f, -0.25f, 0.0f, -0.75f, 0.324, 0.0,
		-.64f, 0.0f, -0.77f, -0.33f, 0.0f, -0.67f, 0.351, 0.0,
		-.5f, 1.0f, -0.87f, -0.33f, 0.0f, -0.67f, 0.324, 1.0,
		-.64f, 1.0f, -0.77f, -0.33f, 0.0f, -0.67f, 0.351, 1.0,
		-.64f, 0.0f, -0.77f, -0.42f, 0.0f, -0.58f, 0.351, 0.0,
		-.77f, 0.0f, -0.64f, -0.42f, 0.0f, -0.58f, 0.378, 0.0,
		-.64f, 1.0f, -0.77f, -0.42f, 0.0f, -0.58f, 0.351, 1.0,
		-.77f, 1.0f, -0.64f, -0.5f, 0.0f, -0.5f, 0.378, 1.0,
		-.77f, 0.0f, -0.64f, -0.5f, 0.0f, -0.5f, 0.378, 0.0,
		-.87f, 0.0f, -0.5f, -0.5f, 0.0f, -0.5f, 0.405, 0.0,
		-.77f, 1.0f, -0.64f, -0.58f, 0.0f, -0.42f, 0.378, 1.0,
		-.87f, 1.0f, -0.5f, -0.58f, 0.0f, -0.42f, 0.405, 1.0,
		-.87f, 0.0f, -0.5f, -0.58f, 0.0f, -0.42f, 0.405, 0.0,
		-.94f, 0.0f, -0.34f, -0.67f, 0.0f, -0.33f, 0.432, 0.0,
		-.87f, 1.0f, -0.5f, -0.67f, 0.0f, -0.33f, 0.405, 1.0,
		-.94f, 1.0f, -0.34f, -0.67f, 0.0f, -0.33f, 0.432, 1.0,
		-.94f, 0.0f, -0.34f, -0.75f, 0.0f, -0.25f, 0.432, 0.0,
		-.98f, 0.0f, -0.17f, -0.75f, 0.0f, -0.25f, 0.459, 0.0,
		-.94f, 1.0f, -0.34f, -0.75f, 0.0f, -0.25f, 0.432, 1.0,
		-.98f, 1.0f, -0.17f, -0.83f, 0.0f, -0.17f, 0.459, 1.0,
		-.98f, 0.0f, -0.17f, -0.83f, 0.0f, -0.17f, 0.459, 0.0,
		-1.0f, 0.0f, 0.0f, -0.83f, 0.0f, -0.17f, 0.486, 0.0,
		-.98f, 1.0f, -0.17f, -0.92f, 0.0f, -0.08f, 0.459, 1.0,
		-1.0f, 1.0f, 0.0f, -0.92f, 0.0f, -0.08f, 0.486, 1.0,
		-1.0f, 0.0f, 0.0f, -0.92f, 0.0f, -0.08f, 0.486, 0.0,
		-.98f, 0.0f, 0.17f, -1.0f, 0.0f, 0.0f, 0.513, 0.0,
		-1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.486, 1.0,
		-.98f, 1.0f, 0.17f, -1.0f, 0.0f, 0.0f, 0.513, 1.0,
		-.98f, 0.0f, 0.17f, -0.92f, 0.0f, 0.08f, 0.513, 0.0,
		-.94f, 0.0f, 0.34f, -0.92f, 0.0f, 0.08f, 0.54, 0.0,
		-.98f, 1.0f, 0.17f, -0.92f, 0.0f, 0.08f, 0.513, 1.0,
		-.94f, 1.0f, 0.34f, -0.83f, 0.0f, 0.17f, 0.54, 1.0,
		-.94f, 0.0f, 0.34f, -0.83f, 0.0f, 0.17f, 0.54, 0.0,
		-.87f, 0.0f, 0.5f, -0.83f, 0.0f, 0.17f, 0.567, 0.0,
		-.94f, 1.0f, 0.34f, -0.75f, 0.0f, 0.25f, 0.54, 1.0,
		-.87f, 1.0f, 0.5f, -0.75f, 0.0f, 0.25f, 0.567, 1.0,
		-.87f, 0.0f, 0.5f, -0.75f, 0.0f, 0.25f, 0.567, 0.0,
		-.77f, 0.0f, 0.64f, -0.67f, 0.0f, 0.33f, 0.594, 0.0,
		-.87f, 1.0f, 0.5f, -0.67f, 0.0f, 0.33f, 0.567, 1.0,
		-.77f, 1.0f, 0.64f, -0.67f, 0.0f, 0.33f, 0.594, 1.0,
		-.77f, 0.0f, 0.64f, -0.58f, 0.0f, 0.42f, 0.594, 0.0,
		-.64f, 0.0f, 0.77f, -0.58f, 0.0f, 0.42f, 0.621, 0.0,
		-.77f, 1.0f, 0.64f, -0.58f, 0.0f, 0.42f, 0.594, 1.0,
		-.64f, 1.0f, 0.77f, -0.5f, 0.0f, 0.5f, 0.621, 1.0,
		-.64f, 0.0f, 0.77f, -0.5f, 0.0f, 0.5f, 0.621, 0.0,
		-.5f, 0.0f, 0.87f, -0.5f, 0.0f, 0.5f, 0.648, 0.0,
		-.64f, 1.0f, 0.77f, -0.42f, 0.0f, 0.58f, 0.621, 1.0,
		-.5f, 1.0f, 0.87f, -0.42f, 0.0f, 0.58f, 0.648, 1.0,
		-.5f, 0.0f, 0.87f, -0.42f, 0.0f, 0.58f, 0.648, 0.0,
		-.34f, 0.0f, 0.94f, -0.33f, 0.0f, 0.67f, 0.675, 0.0,
		-.5f, 1.0f, 0.87f, -0.33f, 0.0f, 0.67f, 0.648, 1.0,
		-.34f, 1.0f, 0.94f, -0.33f, 0.0f, 0.67f, 0.675, 1.0,
		-.34f, 0.0f, 0.94f, -0.25f, 0.0f, 0.75f, 0.675, 0.0,
		-.17f, 0.0f, 0.98f, -0.25f, 0.0f, 0.75f, 0.702, 0.0,
		-.34f, 1.0f, 0.94f, -0.25f, 0.0f, 0.75f, 0.675, 1.0,
		-.17f, 1.0f, 0.98f, -0.17f, 0.0f, 0.83f, 0.702, 1.0,
		-.17f, 0.0f, 0.98f, -0.17f, 0.0f, 0.83f, 0.702, 0.0,
		0.0f, 0.0f, 1.0f, -0.17f, 0.0f, 0.83f, 0.729, 0.0,
		-.17f, 1.0f, 0.98f, -0.08f, 0.0f, 0.92f, 0.702, 1.0,
		0.0f, 1.0f, 1.0f, -0.08f, 0.0f, 0.92f, 0.729, 1.0,
		0.0f, 0.0f, 1.0f, -0.08f, 0.0f, 0.92f, 0.729, 0.0,
		.17f, 0.0f, 0.98f, -0.0f, 0.0f, 1.0f, 0.756, 0.0,
		0.0f, 1.0f, 1.0f, -0.0f, 0.0f, 1.0f, 0.729, 1.0,
		.17f, 1.0f, 0.98f, -0.0f, 0.0f, 1.0f, 0.756, 1.0,
		.17f, 0.0f, 0.98f, 0.08f, 0.0f, 0.92f, 0.756, 0.0,
		.34f, 0.0f, 0.94f, 0.08f, 0.0f, 0.92f, 0.783, 0.0,
		.17f, 1.0f, 0.98f, 0.08f, 0.0f, 0.92f, 0.756, 1.0,
		.34f, 1.0f, 0.94f, 0.17f, 0.0f, 0.83f, 0.783, 1.0,
		.34f, 0.0f, 0.94f, 0.17f, 0.0f, 0.83f, 0.783, 0.0,
		.5f, 0.0f, 0.87f, 0.17f, 0.0f, 0.83f, 0.810, 0.0,
		.34f, 1.0f, 0.94f, 0.25f, 0.0f, 0.75f, 0.783, 1.0,
		.5f, 1.0f, 0.87f, 0.25f, 0.0f, 0.75f, 0.810, 1.0,
		.5f, 0.0f, 0.87f, 0.25f, 0.0f, 0.75f, 0.810, 0.0,
		.64f, 0.0f, 0.77f, 0.33f, 0.0f, 0.67f, 0.837, 0.0,
		.5f, 1.0f, 0.87f, 0.33f, 0.0f, 0.67f, 0.810, 1.0,
		.64f, 1.0f, 0.77f, 0.33f, 0.0f, 0.67f, 0.837, 1.0,
		.64f, 0.0f, 0.77f, 0.42f, 0.0f, 0.58f, 0.837, 0.0,
		.77f, 0.0f, 0.64f, 0.42f, 0.0f, 0.58f, 0.864, 0.0,
		.64f, 1.0f, 0.77f, 0.42f, 0.0f, 0.58f, 0.837, 1.0,
		.77f, 1.0f, 0.64f, 0.5f, 0.0f, 0.5f, 0.864, 1.0,
		.77f, 0.0f, 0.64f, 0.5f, 0.0f, 0.5f, 0.864, 0.0,
		.87f, 0.0f, 0.5f, 0.5f, 0.0f, 0.5f, 0.891, 0.0,
		.77f, 1.0f, 0.64f, 0.58f, 0.0f, 0.42f, 0.864, 1.0,
		.87f, 1.0f, 0.5f, 0.58f, 0.0f, 0.42f, 0.891, 1.0,
		.87f, 0.0f, 0.5f, 0.58f, 0.0f, 0.42f, 0.891, 0.0,
		.94f, 0.0f, 0.34f, 0.67f, 0.0f, 0.33f, 0.918, 0.0,
		.87f, 1.0f, 0.5f, 0.67f, 0.0f, 0.33f, 0.891, 1.0,
		.94f, 1.0f, 0.34f, 0.67f, 0.0f, 0.33f, 0.918, 1.0,
		.94f, 0.0f, 0.34f, 0.75f, 0.0f, 0.25f, 0.918, 0.0,
		.98f, 0.0f, 0.17f, 0.75f, 0.0f, 0.25f, 0.945, 0.0,
		.94f, 1.0f, 0.34f, 0.75f, 0.0f, 0.25f, 0.918, 0.0,
		.98f, 1.0f, 0.17f, 0.83f, 0.0f, 0.17f, 0.945, 1.0,
		.98f, 0.0f, 0.17f, 0.83f, 0.0f, 0.17f, 0.945, 0.0,
		1.0f, 0.0f, 0.0f, 0.83f, 0.0f, 0.17f, 1.0, 0.0,
		.98f, 1.0f, 0.17f, 0.92f, 0.0f, 0.08f, 0.945, 1.0,
		1.0f, 1.0f, 0.0f, 0.92f, 0.0f, 0.08f, 1.0, 1.0,
		1.0f, 0.0f, 0.0f, 0.92f, 0.0f, 0.08f, 1.0, 0.0
	};

	// total float values per each type
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;

	// store vertex and index count
	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

	// Create VAO
	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create VBO
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);
}


void UDestroyMesh(GLMesh& mesh)
{
	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(1, &mesh.vbo);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	// Create a Shader program object.
	programId = glCreateProgram();

	// Create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Retrive the shader source
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	// Compile the vertex shader, and print compilation errors (if any)
	glCompileShader(vertexShaderId); // compile the vertex shader
	// check for shader compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glCompileShader(fragmentShaderId); // compile the fragment shader
	// check for shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	// Attached compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);   // links the shader program
	// check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);    // Uses the shader program

	return true;
}


void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}

	// Error loading the image
	return false;
}


void UDestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}