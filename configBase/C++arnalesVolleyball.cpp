//Proyecto CGIH parte 1
#include <iostream>
#include <cmath>

#include <vector>

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Other Libs
#include "stb_image.h"

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//Load Models
#include "SOIL2/SOIL2.h"


// Other includes
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "SkinnedModel.h"   // animacion de huesos (FBX de Mixamo)


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


//Phys
#include <btBulletDynamicsCommon.h>


// Function prototypes
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
void Animation();
void AnimationKeyFrames();
void interpolation();
void resetElements();



GLfloat current_animation_time{};
void DoAnimation(GLfloat time, std::vector<int>& keyposes, std::vector<Model>& Models, const Shader& shader, const bool condition);


// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camera
Camera  camera(glm::vec3(0.0f, 0.0f, 3.0f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;
// Light attributes
glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
bool active;

float player_rotation{ 135.0f };
bool bIsFirstPerson{ false };
bool bIsRunning{ false };

glm::vec3 pointLightPositions[] = {
	glm::vec3(3.7f, 4.0f, 15.0f),
	glm::vec3(-3.65f, 4.0f, 15.0f),
	glm::vec3(-11.5f, 2.0f, -16.0f),
	glm::vec3(8.0f, 1.0f, -15.0f)
};

float vertices[] = {
	 -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	   -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	   -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

	   -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
	   -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
	   -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

	   -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
	   -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

	   -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
	   -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
	   -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

	   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
	   -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
	   -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

glm::vec3 Light1 = glm::vec3(0);

//Anim
float rotBall = 0.0f;
bool AnimBall = false;
bool AnimDog = false;
float rotDog = 0.0f;
int dogAnim = 0;
float FLegs = 0.0f;
float RLegs = 0.0f;
float head = 0.0f;
float tail = 0.0f;
glm::vec3 dogPos(0.0f, 0.5f, 3.0f);
float dogRot = 0.0f;
float rotate_dog = 0.0f;
bool step = false;
bool rotated{ false };

float dayCycle = 0.0f;
bool dayCycleOn = false;

bool gymLights = true;
bool gymSpotLight = false;

// Animaciones de huesos del jugador: J = salto (animacion 0), K = remate (si existe Remate.fbx)
bool animarJugador = false;
int animJugadorPedida = -1;   // animacion que pidio el teclado; -1 = ninguna


// Deltatime
GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame

GLfloat TotaldeltaTimes{ 0.0f };


// --- ESTRUCTURA DE KEYFRAMES ---
#define MAX_FRAMES 15
int i_max_steps = 50;
int i_curr_steps = 0;


int FrameIndex = 0;
bool play = false;
int playIndex = 0;



int main()
{
	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	/*glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);*/

	btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
	btBroadphaseInterface* overlappingPairCache = new btDbvtBroadphase();
	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
	btDiscreteDynamicsWorld* dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0, -9.81, 0));
	btCollisionShape* groundShape = new btBoxShape(btVector3(15.0f, 0.5f, 25.0f));
	btTransform groundTransform;
	groundTransform.setIdentity();
	groundTransform.setOrigin(btVector3(0, -1.9f, 0)); // Posici�n en Y

	btDefaultMotionState* groundMotionState = new btDefaultMotionState(groundTransform);

	btRigidBody::btRigidBodyConstructionInfo groundInfo(0.0f, groundMotionState, groundShape, btVector3(0, 0, 0));
	btRigidBody* groundBody = new btRigidBody(groundInfo);
	dynamicsWorld->addRigidBody(groundBody);

	btCollisionShape* sphereShape = new btSphereShape(0.25f); // Radio de 0.5
	btTransform startTransform;
	startTransform.setIdentity();
	startTransform.setOrigin(btVector3(-10, 20.0f, 0));

	btScalar mass = 1.0f;
	btVector3 localInertia(0, 0, 0);
	sphereShape->calculateLocalInertia(mass, localInertia); 

	btDefaultMotionState* sphereMotionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo sphereInfo(mass, sphereMotionState, sphereShape, localInertia);
	btRigidBody* sphereBody = new btRigidBody(sphereInfo);
	dynamicsWorld->addRigidBody(sphereBody);

	btCollisionShape* playerShape = new btCapsuleShape(0.5f, 1.0f);

	btScalar playerMass = 75.0f;
	btVector3 playerInertia(0, 0, 0);
	playerShape->calculateLocalInertia(playerMass, playerInertia);

	btTransform playerStartTransform;
	playerStartTransform.setIdentity();
	playerStartTransform.setOrigin(btVector3(0, 2.0f, 0));
	btDefaultMotionState* playerMotionState = new btDefaultMotionState(playerStartTransform);

	btRigidBody::btRigidBodyConstructionInfo playerInfo(playerMass, playerMotionState, playerShape, playerInertia);
	btRigidBody* playerBody = new btRigidBody(playerInfo);

	playerBody->setAngularFactor(btVector3(0.0f, 1.0f, 0.0f));
	playerBody->setFriction(0.0f);

	dynamicsWorld->addRigidBody(playerBody);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Volleyball Scene C++arnales", nullptr, nullptr);

	if (nullptr == window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();

		return EXIT_FAILURE;
	}



	glfwMakeContextCurrent(window);

	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);

	// GLFW ptions
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewExperimental = GL_TRUE;
	if (GLEW_OK != glewInit())
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return EXIT_FAILURE;
	}

	// Define the viewport dimensions
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");
	Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");

	//Model Dog((char*)"Models/RedDog.obj");
	Model Volleyball_Ball((char*)"Models/Ball/VolleyBall_Ball.obj");
	Model Piso((char*)"Models/sand.obj");
	Model PisoPlaya((char*)"Models/sand.obj");

	Model SkyBox((char*)"Models/skysphere.obj");

	//Model chamaca((char*)"Models/MeinGott.glb");
	Model chamaca((char*)"Models/Player/PlayerIdle1.obj");


	Model PalmeraTronco((char*)"Models/palmera/tronco.obj");

	std::vector<Model> HojasPalmera;
	std::vector<float> HojasOffset;


	for (int i = 0; i <= 12; i++) {
		std::string path = "Models/palmera/palm_leaves_palm_leaves_0_00" + std::to_string(i) + ".obj";

		HojasPalmera.push_back(Model((char*)path.c_str()));

		HojasOffset.push_back(i * 0.6f);
	}

	

	//Model DogBody((char*)"Models/DogBody.obj");
	//Model HeadDog((char*)"Models/HeadDog.obj");
	//Model DogTail((char*)"Models/TailDog.obj");
	//Model F_RightLeg((char*)"Models/F_RightLegDog.obj");
	//Model F_LeftLeg((char*)"Models/F_LeftLegDog.obj");
	//Model B_RightLeg((char*)"Models/B_RightLegDog.obj");
	//Model B_LeftLeg((char*)"Models/B_LeftLegDog.obj");


	Model Gradas((char*)"Models/Stands/Stands.obj");
	Model CanchaVolley((char*)"Models/Volleyball_Net/Voleyball_Court.obj");
	

	Model Silla1((char*)"Models/Beach_Chairs/chair1.obj");
	Model Silla2((char*)"Models/Beach_Chairs/chair2.obj");
	Model Sombrilla((char*)"Models/Umbrella/umbrella.obj");
	
	Model Reflector((char*)"Models/Spotlight/spotlight.obj");
	


	// Jugador con esqueleto de Mixamo (modelo de Gabriel): animacion de huesos
	SkinnedModel jugadorSkin("../Blender Models/Gabriel_Yukioayax/humano/JugadorMX/Jumping.fbx");
	jugadorSkin.Reproducir(0, false);   // sin repetir: J lo dispara una vez
	// Segunda animacion de Mixamo (mismo esqueleto, descargada "Without Skin"). Si el archivo no existe, K no hace nada.
	std::ifstream existeRemate("../Blender Models/Gabriel_Yukioayax/humano/JugadorMX/Remate.fbx");
	int animRemate = existeRemate.good() ? jugadorSkin.CargarAnimacion("../Blender Models/Gabriel_Yukioayax/humano/JugadorMX/Remate.fbx") : -1;
	existeRemate.close();
	float jugadorAltura = jugadorSkin.maximo.y - jugadorSkin.minimo.y;
	float jugadorEscala = jugadorAltura > 0.0f ? 1.8f / jugadorAltura : 1.0f;   // 1.8 m de alto
	std::cout << "Jugador bbox min(" << jugadorSkin.minimo.x << "," << jugadorSkin.minimo.y << "," << jugadorSkin.minimo.z
		<< ") max(" << jugadorSkin.maximo.x << "," << jugadorSkin.maximo.y << "," << jugadorSkin.maximo.z << ") escala " << jugadorEscala << std::endl;

	Model Sea((char*)"Models/sea.obj");
	Model Mountein((char*)"Models/mounteindew.obj");

	// ---------------------------------------------------------------------------------------
	// Modelos de Carlos Parra (Blender Models/Carlos_Parra/). Cada uno lleva su posición en la playa,
	// su giro en Y y la escala que lo deja a su altura real en metros. La altura "y" ya incluye
	// la altura de la arena (sand.obj) en ese punto, para que queden apoyados en el suelo.
	// ---------------------------------------------------------------------------------------
	struct ObjetoCarlos {
		const char* nombre;
		const char* ruta;
		glm::vec3 posicion;
		float rotY;     // grados
		float escala;   // uniforme
	};
	std::vector<ObjetoCarlos> objetosCarlos = {
		// Sombrilla con sus accesorios, al este de la zona de descanso
		// Escalas aumentadas para que se vean bien en la escena (sombrilla 3.2 m, botella 0.8 m,
		// gorra 0.4 m, lentes 0.25 m de alto); "y" deja la base de cada modelo sobre la arena.
		{ "Sombrilla", "../Blender Models/Carlos_Parra/Sombrilla/Sombrilla.obj", glm::vec3(14.00f, -0.195f, -9.00f), 0.0f, 0.87190f },
		{ "BotellaPlastico", "../Blender Models/Carlos_Parra/botella_plastico/botella_plastico.obj", glm::vec3(12.80f, -1.463f, -8.00f), 0.0f, 0.14235f },
		{ "Gorra", "../Blender Models/Carlos_Parra/gorra/gorra.obj", glm::vec3(15.00f, -1.735f, -7.80f), 20.0f, 0.90910f },
		{ "LentesSol", "../Blender Models/Carlos_Parra/lentes_sol/lentes_sol.obj", glm::vec3(13.40f, -1.725f, -9.80f), 0.0f, 0.12255f },
	};
	std::vector<Model> modelosCarlos;
	modelosCarlos.reserve(objetosCarlos.size());   // evita copias de Model al crecer el vector
	for (const ObjetoCarlos& o : objetosCarlos) {
		modelosCarlos.push_back(Model((char*)o.ruta));
	}


	
	btRigidBody* PlayaCollision{ LoadRigidCollition("Models/sand.obj", glm::vec3(0.0f,0.0f,0.0f),glm::vec3(0.0f,0.0f,0.0f),glm::vec3(1.0f,1.0f,1.0f)) };
	if (PlayaCollision) { dynamicsWorld->addRigidBody(PlayaCollision); }


	btRigidBody* MounteinCollision{ LoadRigidCollition("Models/mounteindew.obj", glm::vec3(-120.0f, -1.0f, -20.0f),glm::vec3(0.0f,0.0f,0.0f),glm::vec3(1.0f,1.0f,1.0f)) };
	if (MounteinCollision) { dynamicsWorld->addRigidBody(MounteinCollision); }


	btRigidBody* SeaCollision{ LoadRigidCollition("Models/sea.obj", glm::vec3(0.0f,-3.0f,0.0f),glm::vec3(0.0f,0.0f,0.0f),glm::vec3(1.0f,1.0f,1.0f)) };
	if (SeaCollision) { dynamicsWorld->addRigidBody(SeaCollision); }

	// First, set the container's VAO (and VBO)
	GLuint VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	lightingShader.Use();
	glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.difuse"), 0);
	glUniform1i(glGetUniformLocation(lightingShader.Program, "Material.specular"), 1);

	glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.21f, 10000.0f); 

	// Game loop
	while (!glfwWindowShouldClose(window))
	{

		// Calculate deltatime of current frame
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		TotaldeltaTimes += deltaTime;
		current_animation_time += deltaTime;


		glfwPollEvents();
		DoMovement();
		Animation();
		AnimationKeyFrames();

		//MOVEMENT
		btVector3 currentVelocity = playerBody->getLinearVelocity();
		btVector3 targetVelocity(0.0f, currentVelocity.getY(), 0.0f);

		float speed = 7.0f; // Ajusta la velocidad de caminata

		glm::vec3 moveDir(0.0f);

		glm::vec3 camFront = camera.GetFront();
		camFront.y = 0.0f;
		camFront = glm::normalize(camFront);
		glm::vec3 camRight = glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f)));

		if (keys[GLFW_KEY_W]) moveDir += camFront;
		if (keys[GLFW_KEY_S]) moveDir -= camFront;
		if (keys[GLFW_KEY_A]) moveDir -= camRight;
		if (keys[GLFW_KEY_D]) moveDir += camRight;

		if (glm::length(moveDir) > 0.0f) {
			moveDir = glm::normalize(moveDir);
			targetVelocity.setX(moveDir.x * speed);
			targetVelocity.setZ(moveDir.z * speed);

			playerBody->activate(true);
		}


		btVector3 startPos = playerBody->getWorldTransform().getOrigin();
		btVector3 endPos = startPos - btVector3(0.0f, 1.4f, 0.0f);

		btCollisionWorld::ClosestRayResultCallback rayCallback(startPos, endPos);
		dynamicsWorld->rayTest(startPos, endPos, rayCallback);

		bool isFalling = !rayCallback.hasHit();

		if (keys[GLFW_KEY_SPACE] && !isFalling/*abs(currentVelocity.getY()) < 0.01f*/) {
			targetVelocity.setY(7.5f); // Fuerza/Altura del salto

			// "Consumimos" el input artificialmente para que no salga volando 
			// como un jetpack si mantienes la barra espaciadora presionada.
			keys[GLFW_KEY_SPACE] = false;
		}


		playerBody->setLinearVelocity(targetVelocity);




		dynamicsWorld->stepSimulation(deltaTime, 10);

		btTransform trans;


		sphereBody->getMotionState()->getWorldTransform(trans);

		float matrix[16];
		trans.getOpenGLMatrix(matrix);
		glm::mat4 model = glm::make_mat4(matrix);




		playerBody->getMotionState()->getWorldTransform(trans);
		btVector3 pos = trans.getOrigin();

		glm::vec3 offset = bIsFirstPerson ? glm::vec3(0.0f, 0.8f, 0.0f) : (camFront * -2.5f) + glm::vec3(0.0f, 1.0f, 0.0f);
		//glm::vec3 offset = (camFront * -2.5f) + glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 targetCameraPos = glm::vec3(pos.x(), pos.y(), pos.z()) + offset;
		camera.SetPosition(targetCameraPos);

		//----------------------------------------------------------------------------------------------------
		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions

		// Clear the colorbuffer
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// OpenGL options
		glEnable(GL_DEPTH_TEST);

		//Load Model

		// Use cooresponding shader when setting uniforms/drawing objects
		lightingShader.Use();

		glUniform1i(glGetUniformLocation(lightingShader.Program, "diffuse"), 0);
		//glUniform1i(glGetUniformLocation(lightingShader.Program, "specular"),1);

		GLint viewPosLoc = glGetUniformLocation(lightingShader.Program, "viewPos");
		glUniform3f(viewPosLoc, camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);

		dayCycle = dayCycleOn ? (sin(TotaldeltaTimes * 0.2f) * 0.5f + 0.5f) : 0.0f;
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"), 1.0f - 0.95f * dayCycle, 1.0f - 0.95f * dayCycle, 1.0f - 0.85f * dayCycle);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"), 0.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.specular"), 0.0f, 0.0f, 0.0f);

		//Point light 1
		glm::vec3 lightColor;

		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].position"), pointLightPositions[0].x, pointLightPositions[0].y, pointLightPositions[0].z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].ambient"), gymLights*0.3f, gymLights*0.3f, 0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].diffuse"), gymLights * 1.0f, gymLights * 0.9f, 0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].specular"), gymLights * 1.0f, gymLights * 1.0f, 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].linear"), 0.05f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].quadratic"), 0.075f);

		// Point light 2
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].position"), pointLightPositions[1].x, pointLightPositions[1].y, pointLightPositions[1].z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].ambient"), gymLights * 0.3f, gymLights * 0.03f, gymLights * 0.03f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].diffuse"), gymLights * 1.0f, gymLights * 0.1f, gymLights * 0.1f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].specular"), gymLights * 1.0f, gymLights * 0.2f, gymLights * 0.2f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].linear"), 0.05f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].quadratic"), 0.075f);

		// Point light 3
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].position"), pointLightPositions[2].x, pointLightPositions[2].y, pointLightPositions[2].z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].ambient"), 0.02f, 0.2f, 0.2f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].diffuse"), 0.1f, 1.0f, 0.9f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].specular"), 0.2f, 1.0f, 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].linear"), 0.045f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].quadratic"), 0.075f);

		// Point light 4
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].position"), pointLightPositions[3].x, pointLightPositions[3].y, pointLightPositions[3].z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].ambient"), 0.2f, 0.2f, 0.2f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].diffuse"), 6.1f, 0.3f, 2.3f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].specular"), 3.9f, 0.9f, 4.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].linear"), 0.6f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].quadratic"), 0.7f);

		glm::vec3 spotPos(-2.0f, 4.0f, 3.5f);	
		glm::vec3 spotDir = glm::normalize(glm::vec3(0.1f, -0.25f, 0.5f));	
	
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.position"), spotPos.x, spotPos.y, spotPos.z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.direction"), spotDir.x, spotDir.y, spotDir.z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.ambient"), gymSpotLight * 0.7f, gymSpotLight * 0.7f, gymSpotLight * 0.7f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.diffuse"), gymSpotLight * 0.7f, gymSpotLight *  0.7f, gymSpotLight * 0.7f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.specular"), gymSpotLight *  0.1f, gymSpotLight *  0.1f, gymSpotLight * 0.1f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.linear"), 0.005f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.quadratic"), 0.005f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.cutOff"), glm::cos(glm::radians(12.5f)));
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.outerCutOff"), glm::cos(glm::radians(15.0f)));


		// Set material properties
		glUniform1f(glGetUniformLocation(lightingShader.Program, "material.shininess"), 5.0f);

		// Create camera transformations
		glm::mat4 view;
		view = camera.GetViewMatrix();

		// Get the uniform locations
		GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
		GLint viewLoc = glGetUniformLocation(lightingShader.Program, "view");
		GLint projLoc = glGetUniformLocation(lightingShader.Program, "projection");

		// Pass the matrices to the shader
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		glm::mat4 modelPiso = glm::mat4(1.0f);

		modelPiso = glm::translate(modelPiso, glm::vec3(0.0f, /*-1.0f*/0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelPiso));
		Piso.Draw(lightingShader);

		SkyBox.Draw(lightingShader);


		//float matrix[16];
		trans.getOpenGLMatrix(matrix);
		glm::mat4 PlayerModel = glm::make_mat4(matrix);



		glm::vec3 PalmeraPos = glm::vec3(10.0f, -1.0f, -5.0f);

		float AmplitudTronco = 0.045f;
		float VelocidadTronco = 0.8f;

		float anguloTronco = sin(glfwGetTime() * VelocidadTronco) * AmplitudTronco;

		glm::mat4 modelTrunk = glm::mat4(1.0f);
		modelTrunk = glm::translate(modelTrunk, PalmeraPos);


		modelTrunk = glm::rotate(modelTrunk, anguloTronco, glm::vec3(0.0f, 0.0f, 1.0f));

		modelTrunk = glm::scale(modelTrunk, glm::vec3(0.15f));

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelTrunk));
		PalmeraTronco.Draw(lightingShader);

		float VelocidadViento = 2.5f;
		float AmplitudViento = 0.15f;

		for (size_t i = 0; i < HojasPalmera.size(); i++) {
			float angle = sin(glfwGetTime() * VelocidadViento + HojasOffset[i]) * AmplitudViento;

			glm::mat4 modelLeaf = modelTrunk;

			modelLeaf = glm::rotate(modelLeaf, angle, glm::vec3(0.0f, 0.5f, 0.0f));

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelLeaf));
			HojasPalmera[i].Draw(lightingShader);
		}


		PalmeraPos = glm::vec3(-8.0f, -1.0f, 8.0f);

		VelocidadTronco = 0.8f; 

		anguloTronco = sin(glfwGetTime() * VelocidadTronco) * AmplitudTronco;

		modelTrunk = glm::mat4(1.0f);
		modelTrunk = glm::translate(modelTrunk, PalmeraPos);

		modelTrunk = glm::rotate(modelTrunk, anguloTronco, glm::vec3(0.0f, 0.0f, 1.0f));
		modelTrunk = glm::scale(modelTrunk, glm::vec3(0.10f));

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelTrunk));
		PalmeraTronco.Draw(lightingShader);

		VelocidadViento = 2.8f;
		AmplitudViento = 0.17f;

		for (size_t i = 0; i < HojasPalmera.size(); i++) {
			float angle = sin(glfwGetTime() * VelocidadViento + HojasOffset[i]) * AmplitudViento;

			glm::mat4 modelLeaf = modelTrunk;

			modelLeaf = glm::rotate(modelLeaf, angle, glm::vec3(0.0f, 0.5f, 0.0f));

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelLeaf));
			HojasPalmera[i].Draw(lightingShader);
		}

		//glm::mat4 modelTemp = glm::mat4(1.0f); //Temp

		//glm::mat4 modelDog = glm::mat4(1.0f);

		//modelTemp = modelDog = glm::translate(modelDog, dogPos);
		//modelTemp = modelDog = glm::rotate(modelDog, glm::radians(dogRot), glm::vec3(0.0f, 1.0f, 0.0f));
		//glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDog));
		//DogBody.Draw(lightingShader);
		////Head
		//modelDog = modelTemp;
		//modelDog = glm::translate(modelDog, glm::vec3(0.0f, 0.093f, 0.208f));
		//modelDog = glm::rotate(modelDog, glm::radians(head), glm::vec3(0.0f, 0.0f, 1.0f));
		//glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDog));
		//HeadDog.Draw(lightingShader);
		////Tail 
		//modelDog = modelTemp;
		//modelDog = glm::translate(modelDog, glm::vec3(0.0f, 0.026f, -0.288f));
		//modelDog = glm::rotate(modelDog, glm::radians(tail), glm::vec3(0.0f, 0.0f, -1.0f));
		//glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDog));
		//DogTail.Draw(lightingShader);
		////Front Left Leg
		//modelDog = modelTemp;
		//modelDog = glm::translate(modelDog, glm::vec3(0.112f, -0.044f, 0.074f));
		//modelDog = glm::rotate(modelDog, glm::radians(FLegs), glm::vec3(-1.0f, 0.0f, 0.0f));
		//glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDog));
		//F_LeftLeg.Draw(lightingShader);
		////Front Right Leg
		//modelDog = modelTemp;
		//modelDog = glm::translate(modelDog, glm::vec3(-0.111f, -0.055f, 0.074f));
		//modelDog = glm::rotate(modelDog, glm::radians(FLegs), glm::vec3(1.0f, 0.0f, 0.0f));
		//glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDog));
		//F_RightLeg.Draw(lightingShader);
		////Back Left Leg
		//modelDog = modelTemp;
		//modelDog = glm::translate(modelDog, glm::vec3(0.082f, -0.046, -0.218));
		//modelDog = glm::rotate(modelDog, glm::radians(RLegs), glm::vec3(1.0f, 0.0f, 0.0f));
		//glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDog));
		//B_LeftLeg.Draw(lightingShader);
		////Back Right Leg
		//modelDog = modelTemp;
		//modelDog = glm::translate(modelDog, glm::vec3(-0.083f, -0.057f, -0.231f));
		//modelDog = glm::rotate(modelDog, glm::radians(RLegs), glm::vec3(-1.0f, 0.0f, 0.0f));
		//glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelDog));
		//B_RightLeg.Draw(lightingShader);
		//
		// Sea (sea.obj)
		glm::mat4 modelSea = glm::mat4(1.0f);
		modelSea = glm::translate(modelSea, glm::vec3(0.0f, -2.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelSea));
		Sea.Draw(lightingShader);


		//Mountein (mounteindew.obj)
		glm::mat4 modelMountein = glm::mat4(1.0f);
		modelMountein = glm::translate(modelMountein, glm::vec3(100.0f, -2.0f, 120.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMountein));
		Mountein.Draw(lightingShader);

		modelMountein = glm::mat4(1.0f);
		modelMountein = glm::translate(modelMountein, glm::vec3(-120.0f, -1.0f, 20.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMountein));
		Mountein.Draw(lightingShader);

		// Gradas (Stands.obj)
		glm::mat4 modelGradas = glm::mat4(1.0f);
		modelGradas = glm::translate(modelGradas, glm::vec3(7.50f, 0.05f, 15.0f));
		modelGradas = glm::rotate(modelGradas, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelGradas = glm::scale(modelGradas, glm::vec3(1.75f, 1.75f, 1.75f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelGradas));
		Gradas.Draw(lightingShader);

		// Gradas2 (Stands.obj)
		glm::mat4 modelGradas2 = glm::mat4(1.0f);
		modelGradas2 = glm::translate(modelGradas2, glm::vec3(4.5f, 0.05f, 27.5f));
		modelGradas2 = glm::rotate(modelGradas2, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelGradas2 = glm::scale(modelGradas2, glm::vec3(1.75f, 1.75f, 1.75f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelGradas2));
		Gradas.Draw(lightingShader);

		// Cancha (Voleyball_Court.obj)
		glm::mat4 modelCancha = glm::mat4(1.0f);
		modelCancha = glm::translate(modelCancha, glm::vec3(0.0f, 0.003f, 15.0f));
		modelCancha = glm::scale(modelCancha, glm::vec3(0.6f, 1.0f, 0.9f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelCancha));
		CanchaVolley.Draw(lightingShader);

	



		// Sombrilla (umbrella.obj)
		glm::mat4 modelSombrilla = glm::mat4(1.0f);
		modelSombrilla = glm::translate(modelSombrilla, glm::vec3(8.0f, -1.75f, -15.0f));
		modelSombrilla = glm::scale(modelSombrilla, glm::vec3(0.4f, 0.4f, 0.4f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelSombrilla));
		Sombrilla.Draw(lightingShader);

		// Silla1 (chair1.obj) Caf�
		glm::mat4 modelSilla1 = glm::mat4(1.0f);
		modelSilla1 = glm::translate(modelSilla1, glm::vec3(10.0f, -1.75f, -15.0f));
		modelSilla1 = glm::rotate(modelSilla1, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelSilla1 = glm::scale(modelSilla1, glm::vec3(1.1f, 1.1f, 1.1f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelSilla1));
		Silla1.Draw(lightingShader);

		// Silla2 (chair2.obj) Azul
		glm::mat4 modelSilla2 = glm::mat4(1.0f);
		modelSilla2 = glm::translate(modelSilla2, glm::vec3(6.0f, -1.50f, -15.0f));
		modelSilla2 = glm::rotate(modelSilla2, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelSilla2 = glm::scale(modelSilla2, glm::vec3(1.25f, 1.25f, 1.25f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelSilla2));
		Silla2.Draw(lightingShader);

		modelSombrilla = glm::mat4(1.0f);
		modelSombrilla = glm::translate(modelSombrilla, glm::vec3(-2.0f, -1.5f, -15.0f));
		modelSombrilla = glm::scale(modelSombrilla, glm::vec3(0.4f, 0.4f, 0.4f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelSombrilla));
		Sombrilla.Draw(lightingShader);

		modelSilla1 = glm::mat4(1.0f);
		modelSilla1 = glm::translate(modelSilla1, glm::vec3(0.0f, -1.25f, -15.0f));
		modelSilla1 = glm::rotate(modelSilla1, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelSilla1 = glm::scale(modelSilla1, glm::vec3(1.1f, 1.1f, 1.1f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelSilla1));
		Silla1.Draw(lightingShader);

		modelSilla2 = glm::mat4(1.0f);
		modelSilla2 = glm::translate(modelSilla2, glm::vec3(2.0f, -1.25f, -15.0f));
		modelSilla2 = glm::rotate(modelSilla2, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelSilla2 = glm::rotate(modelSilla2, glm::radians(05.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		modelSilla2 = glm::scale(modelSilla2, glm::vec3(1.25f, 1.25f, 1.25f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelSilla2));
		Silla2.Draw(lightingShader);

		


		// Modelos de Carlos Parra: trasladar, girar en Y y escalar
		for (size_t i = 0; i < objetosCarlos.size(); i++) {
			const ObjetoCarlos& o = objetosCarlos[i];
			glm::mat4 modelCarlos = glm::mat4(1.0f);
			modelCarlos = glm::translate(modelCarlos, o.posicion);
			modelCarlos = glm::rotate(modelCarlos, glm::radians(o.rotY), glm::vec3(0.0f, 1.0f, 0.0f));
			modelCarlos = glm::scale(modelCarlos, glm::vec3(o.escala));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelCarlos));
			modelosCarlos[i].Draw(lightingShader);
		}

		// Jugador animado con huesos: J (salto) o K (remate) se reproducen una vez y regresa a la pose inicial
		if (animJugadorPedida >= 0) {
			int pedida = animJugadorPedida == 1 ? animRemate : 0;
			animJugadorPedida = -1;
			if (pedida >= 0) { jugadorSkin.Reproducir(pedida, false); animarJugador = true; }
		}
		if (animarJugador) {
			jugadorSkin.Actualizar(deltaTime);
			if (jugadorSkin.Terminada()) {
				animarJugador = false;
				jugadorSkin.Reproducir(0, false);
				jugadorSkin.Actualizar(0.0f);
			}
		}
		glm::mat4 modelJugador = glm::mat4(1.0f);
		modelJugador = glm::translate(modelJugador, glm::vec3(2.0f, 0.01f, 10.0f)   /* sobre el piso de la cancha (y = 0.003) */);
		modelJugador = glm::scale(modelJugador, glm::vec3(jugadorEscala));
		modelJugador = glm::translate(modelJugador, glm::vec3(0.0f, -jugadorSkin.minimo.y, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelJugador));
		jugadorSkin.Draw(lightingShader);

		// Reflector (spotlight.obj)
		glm::mat4 modelReflector = glm::mat4(1.0f);
		modelReflector = glm::translate(modelReflector, spotPos);
		modelReflector = glm::scale(modelReflector, glm::vec3(1.0f, 1.0f, 1.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelReflector));
		Reflector.Draw(lightingShader);



		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 0);

		//glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		//Volleyball_Ball.Draw(lightingShader);

		//glDisable(GL_BLEND);
		//glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 0);

		lampShader.Use();
		modelLoc = glGetUniformLocation(lampShader.Program, "model");
		viewLoc = glGetUniformLocation(lampShader.Program, "view");
		projLoc = glGetUniformLocation(lampShader.Program, "projection");

		// Set matrices
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
		model = glm::mat4(1);
		model = glm::translate(model, lightPos);
		model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		// Draw the light object (using light's vertex attributes)
		for (GLuint i = 0; i < 4; i++)
		{
			model = glm::mat4(1);
			model = glm::translate(model, pointLightPositions[i]);
			model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glBindVertexArray(VAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
		glBindVertexArray(0);

		glBindVertexArray(0);

		// Swap the screen buffers
		glfwSwapBuffers(window);
	}


	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();



	return 0;
}

// Moves/alters the camera positions based on user input
void DoMovement()
{

	// Camera controls
	/*if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
	{
		camera.ProcessKeyboard(FORWARD, deltaTime);

	}

	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
	{
		camera.ProcessKeyboard(BACKWARD, deltaTime);


	}

	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
	{
		camera.ProcessKeyboard(LEFT, deltaTime);


	}

	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
	{
		camera.ProcessKeyboard(RIGHT, deltaTime);


	}*/
}

// Is called whenever a key is pressed/released via GLFW
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			keys[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			keys[key] = false;
		}
	}

	if (keys[GLFW_KEY_P])
	{
		AnimDog = !AnimDog;

	}

	if (keys[GLFW_KEY_1])
	{
		bIsFirstPerson = !bIsFirstPerson;
	}

	if (keys[GLFW_KEY_2])
	{
		dayCycleOn = !dayCycleOn;
	}

	if (keys[GLFW_KEY_3])
	{
		gymLights = !gymLights;
	}

	if (keys[GLFW_KEY_4])
	{
		gymSpotLight = !gymSpotLight;
	}

	if (key == GLFW_KEY_J && action == GLFW_PRESS)
	{
		animJugadorPedida = 0;   // salto
	}

	if (key == GLFW_KEY_K && action == GLFW_PRESS)
	{
		animJugadorPedida = 1;   // remate
	}

	if (keys[GLFW_KEY_L])
	{
		play = !play;
	}
}

void MouseCallback(GLFWwindow* window, double xPos, double yPos)
{
	if (firstMouse)
	{
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}

	GLfloat xOffset = xPos - lastX;
	GLfloat yOffset = lastY - yPos;  // Reversed since y-coordinates go from bottom to left

	lastX = xPos;
	lastY = yPos;

	camera.ProcessMouseMovement(xOffset, yOffset);
}


void Animation() {
	if (AnimBall)
	{
		rotBall += 0.4f;
		//printf("%f", rotBall);
	}

	float quantity{ 0.5f };


	if (AnimDog) {


		dogPos.z += 0.005f;


		if (!step) {
			RLegs -= quantity;
			FLegs -= quantity;
			head -= quantity;
			tail -= quantity;
			if (RLegs <= -15.0f) { step = !step; }
		}
		else {
			RLegs += quantity;
			FLegs += quantity;
			head += quantity;
			tail += quantity;
			if (RLegs >= 15.0f) { step = !step; }
		}
	}
}

void AnimationKeyFrames()
{
	if (play)
	{
		if (i_curr_steps >= i_max_steps)
		{
			playIndex++;
			if (playIndex > FrameIndex - 2)
			{
				printf("Reiniciando animacion desde cero\n");
				playIndex = 0;         // Regresa al primer par de keyframes
				resetElements();       // <--- ¡ESTO ES LO CLAVE! Forzar a que el modelo vuelva a la pose del Frame 0
			}
			i_curr_steps = 0;
			interpolation();
		}
		else
		{
			

			i_curr_steps++;
		}
	}
}



void resetElements(void)
{
	
}

void interpolation(void)
{
		}

