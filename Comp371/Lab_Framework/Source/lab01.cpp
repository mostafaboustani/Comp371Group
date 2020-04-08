//
// COMP 371 Labs Framework
//
// Created by Nicolas Bergeron on 20/06/2019.
//

#include "COMP371Helper.h"

using namespace glm;
using namespace std;

int main(int argc, char*argv[])
{
	if (!initContext()) return -1;

	// Disable mouse cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Black background
	glClearColor(135.0f/255.0f, 206.0f/255.0f, 235.0f/255.0f, 1.0f);

	//---------------------------------------Textures--------------------------------------------//
	// Load Textures
#if defined(PLATFORM_OSX)
	GLuint grassTextureID = loadTexture("Textures/grass.jpg");
#else
	GLuint grassTextureID = loadTexture("../Assets/Textures/grass.jpg");
#endif

	// GL_TEXTURE0 IS RESERVED FOR SHADOW MAPPING
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, grassTextureID);

	//------------------------------------Shader Programs----------------------------------------//

	// Compile and link shaders here ...
	int shaderProgramBasic = compileAndLinkShaders("Comp371Basic.vshader", "Comp371Basic.fshader");
	int shaderProgramTexture = compileAndLinkShaders("Comp371Texture.vshader", "Comp371Texture.fshader");
	int shaderProgramShadow = compileAndLinkShaders("Comp371Shadow.vshader", "Comp371Shadow.fshader");
	int shaderProgramLightSource = compileAndLinkShaders("Comp371LightSource.vshader", "Comp371LightSource.fshader");

	//-----------------------------------------SHADOWS--------------------------------------//

	// Create framebuffer object for rendering the depth map
	GLuint depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	// Create a 2D texture that we'll use as the framebuffer's depth buffer
	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

	GLuint depthMap;
	glGenTextures(1, &depthMap);
	glActiveTexture(GL_TEXTURE0); // Texture2 for shadow rendering because Texture1 and texture2 is for ground and nose textures
	glBindTexture(GL_TEXTURE_2D, depthMap);
	setTexture(shaderProgramBasic, "shadowMap", 0);
	setTexture(shaderProgramTexture, "shadowMap", 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Attach the generated depth texture as the framebuffer's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//--------------------------------------------------------------------------------------//

	//----------------------------------------Camera------------------------------------------//

	// Camera parameters for view transform
	vec3 cameraPosition(0.6f, 1.0f, 1.0f);
	vec3 cameraLookAt(0.0f, 0.0f, -1.0f);
	vec3 cameraUp(0.0f, 1.0f, 0.0f);
	vec3 cameraTarget = vec3(0.0f, 0.0f, 0.0f);
	vec3 cameraDirection = normalize(cameraPosition - cameraTarget);

	// THE FOLLOWING IS IMPORTANT FOR THE LIGHTING
	setVec3(shaderProgramBasic, "viewPos", cameraPosition);
	setVec3(shaderProgramTexture, "viewPos", cameraPosition);

	// Other camera parameters
	float cameraSpeed = 1.0f;
	float cameraFastSpeed = 2 * cameraSpeed;
	float cameraHorizontalAngle = 90.0f;
	float cameraVerticalAngle = 0.0f;
	bool  cameraFirstPerson = true; // press 1 or 2 to toggle this variable

									// Spinning cube at camera position
	float spinningCubeAngle = 0.0f;

	//-------------------------------------ProjectionMatrix---------------------------------------//

	// Set projection matrix
	mat4 projectionMatrix = glm::perspective(70.0f,            // field of view in degrees
		800.0f / 600.0f,  // aspect ratio
		0.01f, 800.0f);   // near and far (near > 0)


	setMat4(shaderProgramBasic, "projectionMatrix", projectionMatrix);
	setMat4(shaderProgramTexture, "projectionMatrix", projectionMatrix);
	setMat4(shaderProgramLightSource, "projectionMatrix", projectionMatrix);

	//---------------------------------------ViewMatrix-------------------------------------------//

	// Set initial view matrix
	mat4 viewMatrix = lookAt(cameraPosition,  // eye
		 cameraLookAt,  // center
		cameraUp); // up

	setMat4(shaderProgramBasic, "viewMatrix", viewMatrix);
	setMat4(shaderProgramTexture, "viewMatrix", viewMatrix);
	setMat4(shaderProgramLightSource, "viewMatrix", viewMatrix);

	//-----------------------------------------VAOs--------------------------------------------//

	// Define and upload geometry to the GPU here ...
	int vaoCube = createVertexArrayObjectCube();
	int vaoGround = createVertexArrayObjectGround();

	//----------------------------------------------------------------------------------------//
	// For frame time
	float lastFrameTime = glfwGetTime();
	int lastMouseLeftState = GLFW_RELEASE;
	double lastMousePosX, lastMousePosY;
	glfwGetCursorPos(window, &lastMousePosX, &lastMousePosY);

	//-----------------------------------------Settings?-----------------------------------------//

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	//---------------------------------------Miscellaneous------------------------------------//

	// Put future variables here

	//----------------------------------------------------------------------------------------//

	// Entering Game Loop
	while (!glfwWindowShouldClose(window))
	{
		// Frame time calculation
		float dt = glfwGetTime() - lastFrameTime;
		lastFrameTime += dt;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//-----------------------------------------SHADOWS--------------------------------------//

		vec3 lightPos = vec3(0.001f, 150.0f, -300.0f);
		mat4 lightProjectionMatrix = ortho(-200.0f, 200.0f, -200.0f, 200.0f, 1.0f, 400.0f);
		mat4 lightViewMatrix = lookAt(lightPos, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
		mat4 lightSpaceMatrix = lightProjectionMatrix * lightViewMatrix;

		setVec3(shaderProgramBasic, "lightPos", lightPos);
		setVec3(shaderProgramTexture, "lightPos", lightPos);
		setMat4(shaderProgramBasic, "lightSpaceMatrix", lightSpaceMatrix);
		setMat4(shaderProgramTexture, "lightSpaceMatrix", lightSpaceMatrix);

		glCullFace(GL_FRONT);

		// 1. first render to depth map
		setMat4(shaderProgramShadow, "lightSpaceMatrix", lightSpaceMatrix);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);

		// Draw ground
		glBindVertexArray(vaoGround);
		glUseProgram(shaderProgramShadow);

		mat4 groundWorldMatrix = translate(mat4(1.0f), vec3(0.0f, 0.0f, 0.0f)) *
			scale(mat4(1.0f), vec3(50.0f, 0.0f, 50.0f)); // 100 * 100 grid now
		setMat4(shaderProgramShadow, "worldMatrix", groundWorldMatrix);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//--------------------------------Now do actual rendering-------------------------------//

		const unsigned int SCR_WIDTH = 1024, SCR_HEIGHT = 768;
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//----------------------------------Draw light source-----------------------------------//
		glBindVertexArray(vaoCube);
		glUseProgram(shaderProgramLightSource);
		mat4 lightSourceMatrix = translate(mat4(1.0), lightPos) * scale(mat4(1.0), vec3(30.0f, 30.0f, 30.0f));
		setMat4(shaderProgramLightSource, "worldMatrix", lightSourceMatrix);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		//----------------------------------Draw ground-----------------------------------------//
		glBindVertexArray(vaoGround);
		glUseProgram(shaderProgramTexture);

		groundWorldMatrix = translate(mat4(1.0f), vec3(0.0f, 0.0f, 0.0f)) * 
			scale(mat4(1.0f), vec3(50.0f, 0.0f, 50.0f)); // 100 * 100 grid now
		setMat4(shaderProgramTexture, "worldMatrix", groundWorldMatrix);
		setVec3(shaderProgramTexture, "aColor", vec3(1.0f, 1.0f, 1.0f));

		// The following is to make the grass texture repeat so that it doesn't become blurry
		setFloat(shaderProgramTexture, "uvMultiplier", 12.0f);
		// Activate texture1 where the grass texture is located
		glActiveTexture(GL_TEXTURE0 + 1);
		GLuint textureLocation = glGetUniformLocation(shaderProgramTexture, "textureSampler");
		glUniform1i(textureLocation, 1);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		setFloat(shaderProgramTexture, "uvMultiplier", 1.0f);
		setVec3(shaderProgramTexture, "aColor", vec3(1.0f, 1.0f, 1.0f));

		//----------------------------------------------------------------------------------------//


		glBindVertexArray(0);

		// End Frame
		glfwSwapBuffers(window);
		glfwPollEvents();

		//--------------------------------------Handle Inputs-------------------------------------//

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		bool fastCam = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
		float currentCameraSpeed = (fastCam) ? cameraFastSpeed : cameraSpeed;
		
		double mousePosX, mousePosY;
		glfwGetCursorPos(window, &mousePosX, &mousePosY);

		double dx = mousePosX - lastMousePosX;
		double dy = mousePosY - lastMousePosY;

		lastMousePosX = mousePosX;
		lastMousePosY = mousePosY;

		// Convert to spherical coordinates
		const float cameraAngularSpeed = 60.0f;
		cameraHorizontalAngle -= dx * cameraAngularSpeed * dt;
		cameraVerticalAngle   -= dy * cameraAngularSpeed * dt;

		// Clamp vertical angle to [-85, 85] degrees
		//cameraVerticalAngle = std::max(-85.0f, std::min(85.0f, cameraVerticalAngle));
		if (cameraHorizontalAngle > 360)
		{
		cameraHorizontalAngle -= 360;
		}
		else if (cameraHorizontalAngle < -360)
		{
		cameraHorizontalAngle += 360;
		}

		float theta = radians(cameraHorizontalAngle);
		float phi = radians(cameraVerticalAngle);

		cameraLookAt = vec3(cosf(phi)*cosf(theta), sinf(phi), -cosf(phi)*sinf(theta));
		vec3 cameraSideVector = glm::cross(cameraLookAt, vec3(0.0f, 1.0f, 0.0f));

		glm::normalize(cameraSideVector);
		

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // move camera to the left
		{
			cameraPosition -= cameraSideVector * currentCameraSpeed * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // move camera to the right
		{
			cameraPosition += cameraSideVector * currentCameraSpeed * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // move camera up
		{
			cameraPosition -= cameraLookAt * currentCameraSpeed * dt;
		}

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // move camera down
		{
			cameraPosition += cameraLookAt * currentCameraSpeed * dt;
		}
		setVec3(shaderProgramBasic, "viewPos", cameraPosition);
		setVec3(shaderProgramTexture, "viewPos", cameraPosition);

		mat4 viewMatrix = lookAt(cameraPosition, cameraPosition + cameraLookAt, cameraUp);

		setMat4(shaderProgramBasic, "viewMatrix", viewMatrix);
		setMat4(shaderProgramTexture, "viewMatrix", viewMatrix);
		setMat4(shaderProgramLightSource, "viewMatrix", viewMatrix);
		glUseProgram(shaderProgramBasic);

	}


	// Shutdown GLFW
	glfwTerminate();

	return 0;
}

