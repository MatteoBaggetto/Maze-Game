#include "modules/Starter.hpp"
#include "modules/MazeGenerator.hpp"
#include "modules/TextMaker.hpp"
#include "modules/GameObjects.hpp"

#define UNITARY_SCALE 3.0f
#define UV_PAVEMENT_SCALE 16.0f
#define PAVEMENT_SCALE 83.0f
#define PLATFORM_NUMBER 2
#define INITIAL_PLAYER_HEIGHT 2.0f
#define CENTRE_PAV_Z 23.97f

std::vector<SingleText> demoText;

// The uniform buffer object used in this example

// This contains the transform matrices for the object. In this case it is Binding 0 of Set 1
struct UniformBufferObject
{
	alignas(16) glm::mat4 mvpMat;
	alignas(16) glm::mat4 mMat;
	alignas(16) glm::mat4 nMat;
};

struct MazeUniformBufferObject
{
	alignas(16) glm::mat4 mvpMat[MAZE_SIZE][MAZE_SIZE][MAZE_HEIGHT];
	alignas(16) glm::mat4 mMat[MAZE_SIZE][MAZE_SIZE][MAZE_HEIGHT];
	alignas(16) glm::mat4 nMat[MAZE_SIZE][MAZE_SIZE][MAZE_HEIGHT];
};

struct LampUniformBufferObject
{

	UniformBufferObject ubo[WALL_LIGHTS_NUMBER];
};

struct PlatformUniformBufferObject
{

	UniformBufferObject ubo[PLATFORM_NUMBER];
};

struct KeyUniformBufferObject
{

	UniformBufferObject ubo[KEYS_NUMBER];
};

// // This contains the material parameters for the object. In this case it is Binding 2 of Set 1
// struct MaterialUniformBufferObject {
// 	alignas(16) glm::vec4 specDef;
// };

// This are the scene-wise uniforms. In this case the light parameters and the position of the camera.
// They are mapped to Binding 0 of Set 0
struct GlobalUniformBufferObject
{
	alignas(16) glm::vec3 eyePos;

	alignas(16) glm::vec3 handLightPos;
	alignas(16) glm::vec4 handLightColor;
	alignas(4) float handLightDecayFactor;

	// struct{
	//	alignas(16) glm::vec3 v;
	// } wallLampPos[5];
	// walllight
	alignas(16) glm::vec4 wallLampPos[WALL_LIGHTS_NUMBER];
	alignas(16) glm::vec4 wallLampColor;
	alignas(4) float wallLampDecayFactor;

	// spot light for cup

	alignas(16) glm::vec3 cupLightPos;
	alignas(16) glm::vec4 cupLightColor;
	alignas(16) glm::vec3 cupLightDir;
	alignas(4) float cupLightDecayFactor;
	alignas(4) float cupCIN;
	alignas(4) float cupCOUT;
};

struct PavementParametersUniformBufferObject
{
	alignas(4) float uScale;
	alignas(4) float vScale;
	alignas(4) float blinnGamma;
	alignas(4) float balanceDiffuseSpecular;
};

struct BoxParametersUniformBufferObject
{
	alignas(4) float blinnGamma;
	alignas(4) float balanceDiffuseSpecular;
	alignas(4) float ambientFactor;
};

// This is the Vertex definition
struct Vertex
{
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 UV;
};

// MAIN !
class Project : public BaseProject
{
public:
	Project()
	{
		maze = new Maze();
		maze->generateMaze();
		player.setPosition(glm::vec3(maze->getStartPoint().c * UNITARY_SCALE, INITIAL_PLAYER_HEIGHT, maze->getStartPoint().r * UNITARY_SCALE));
		player.setRotation(glm::vec2(glm::radians(180.0f), -0.3f));
	}

protected:
	// Here you list all the Vulkan objects you need:

	// Descriptor Layouts [what will be passed to the shaders]
	// In this case, DSL1 is used for the shader specific parameters, and it is mapped to Set 0
	// DSLG contains the global parameters and it is mapped to Set 1
	DescriptorSetLayout DSLPavement, DSLG, DSLBox, DSLPlatform, DSLLamp, DSLOilLamp, DSLCup, DSLKey, DSLMoon;

	// Vertex descriptor
	VertexDescriptor VD;

	// Pipelines [Shader couples]
	Pipeline PPavement, PBox, PPlatform, PLamp, POilLamp, PCup, PKey, PMoon;

	// Scenes and texts
	TextMaker txt;

	// Models, textures and Descriptor Sets (values assigned to the uniforms)
	Model MPavement;
	Texture TPavDif, TPavSpec;
	DescriptorSet DSPavement, DSG; // Even if we have just one object, since we have two DSL, we also need two sets.

	Model MBox;
	Texture TCubeDiffuse, TCubeSpecular, TCubeAmbient;
	DescriptorSet DSBox;

	Model MPlatform;
	Texture TPlatDiffuse, TPlatSpecular;
	DescriptorSet DSPlatform;

	Model MLamp;
	Texture TLampDiffuse, TLampSpecular;
	DescriptorSet DSLamp;

	Model MOilLamp;
	Texture TOilLampDiffuse, TOilLampSpecular;
	DescriptorSet DSOilLamp;

	Model MCup;
	Texture TCupDiffuse, TCupSpecular;
	DescriptorSet DSCup;

	Model MKey;
	Texture TKeyDiffuse, TKeySpecular;
	DescriptorSet DSKey;

	Model MMoon;
	Texture TMoonDiffuse;
	DescriptorSet DSMoon;

	// Uniform Buffers
	bool uniformBuffersInit = false; // Run update of static objects only once
	GlobalUniformBufferObject gubo{};
	UniformBufferObject pavUbo{};
	MazeUniformBufferObject mazeUbo;
	PlatformUniformBufferObject platUbo{};
	LampUniformBufferObject lampUbo{};
	PavementParametersUniformBufferObject pavparubo{};
	BoxParametersUniformBufferObject boxparubo{};
	UniformBufferObject cupUbo{};
	KeyUniformBufferObject keyUbo{};
	UniformBufferObject moonUbo{};
	UniformBufferObject oilLampUbo{};

	// GameObjects
	Maze* maze;
	Player player = Player(UNITARY_SCALE);

	//Display text
	int currText = 0;

	// Other application parameters
	// Current aspect ratio, used to build a correct Projection matrix
	float Ar;

	// Here you set the window parameters
	void setWindowParameters()
	{
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "Computer Graphics Project";
		windowResizable = GLFW_TRUE;
		initialBackgroundColor = {0.02f, 0.1f, 0.4f, 1.0f};

		// The initial aspect rati of the window. In this code, we assume square pixels
		Ar = 4.0f / 3.0f;
	}

	// What to do when the window changes size
	void onWindowResize(int w, int h)
	{
		std::cout << "Window resized to: " << w << " x " << h << "\n";
		// The most important thing to do when the window changes size is updating the aspect ratio.
		Ar = (float)w / (float)h;
	}

	// Here you load and setup all your Vulkan Models and Texutures.
	// Here you also create your Descriptor set layouts, vertex descriptors and load the shaders for the pipelines
	void localInit()
	{
		// Descriptor Layouts [what will be passed to the shaders]
		DSLG.init(this, {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1}});
		DSLPavement.init(this, {// first  element : the binding number
								// second element : the type of element (buffer or texture) - a Vulkan constant
								// third  element : the pipeline stage where it will be used - a Vulkan constant
								// fourth element : for uniform buffers -> the size of the data to be passed
								//                  for textures        -> the index of the texture in the array passed to the binding function
								// fifth  element : the number of elements of this type to be created. Usually 1

								{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObject), 1},
								{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
								{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},
								{3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PavementParametersUniformBufferObject), 1}

							   });

		DSLBox.init(this, {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(MazeUniformBufferObject), 1},
						   {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
						   {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},
						   {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1},
						   {4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(BoxParametersUniformBufferObject), 1}});

		DSLPlatform.init(this, {

								   {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(PlatformUniformBufferObject), 1},
								   {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
								   {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},
							});

		DSLLamp.init(this, {

							   {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(LampUniformBufferObject), 1},
							   {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
							   {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},

						   });

		DSLOilLamp.init(this, {

								  {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObject), 1},
								  {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
								  {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},

							  });

		DSLCup.init(this, {

							  {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObject), 1},
							  {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
							  {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},

						  });

		DSLKey.init(this, {

							  {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(KeyUniformBufferObject), 1},
							  {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1},
							  {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1},

						  });

		DSLMoon.init(this, {

							   {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObject), 1},
							   {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}});

		// Vertex definition (used by the models)
		VD.init(this, {// this array contains the bindings
					   // first  element : the binding number
					   // second element : the stride (size of a record) of this binging
					   // third  element : whether this parameter change per vertex or per instance
					   //                  using the corresponding Vulkan constant
					   // CURRENTLY only Single Binding scenario have been tested.
					   {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}},
				{// this array contains the location of the components of the vertex
				 // first  element : the binding number
				 // second element : the location number
				 // third  element : the offset of this element in the memory record - usually computed with the offsetof() macro.
				 // fourth element : the data type of the element
				 //                  using the corresponding Vulkan constant
				 // fifth  elmenet : the size in byte of the element - generaly obtained with a sizeof() macro
				 // sixth  element : a constant defining the element usage
				 //                   POSITION - a vec3 with the position
				 //                   NORMAL   - a vec3 with the normal vector
				 //                   UV       - a vec2 with a UV coordinate
				 //                   COLOR    - a vec4 with a RGBA color
				 //                   TANGENT  - a vec4 with the tangent vector
				 //                   OTHER    - anything else
				 //				These constnats are used when loading a model, to fill the provided vertex structure
				 //				with the data contained in the file.
				 // this array must have exactly one element per field of the Cpp structure corresponding to the vertex
				 // ***************** DOUBLE CHECK ********************
				 //    That the Vertex data structure you use in the "offsetoff" and
				 //	in the "sizeof" in the previous array, refers to the correct one,
				 //	if you have more than one vertex format!
				 // ***************************************************
				 {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos), sizeof(glm::vec3), POSITION},
				 {0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, norm), sizeof(glm::vec3), NORMAL},
				 {0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV), sizeof(glm::vec2), UV}}); // This is a classical vertex of a simple textured smooth model,
																								 // which includes the position, the normal vector and the UV coordinates.

		// Pipelines [Shader couples]		// Pipelines [Shader couples]

		// The second parameter is the pointer to the vertex definition
		// Third and fourth parameters are respectively the vertex and fragment shaders files containing the SPV code
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..

		// Pavement Pipeline
		PPavement.init(this, &VD, "shaders/PavVert.spv", "shaders/PavFrag.spv", {&DSLG, &DSLPavement});
		// Box Pipeline
		PBox.init(this, &VD, "shaders/BoxVert.spv", "shaders/BoxFrag.spv", {&DSLG, &DSLBox});

		PPlatform.init(this, &VD, "shaders/PlatVert.spv", "shaders/ToonMinimalFrag.spv", {&DSLG, &DSLPlatform});

		PLamp.init(this, &VD, "shaders/LampVert.spv", "shaders/ToonMinimalFrag.spv", {&DSLG, &DSLLamp});

		POilLamp.init(this, &VD, "shaders/SimpleVert.spv", "shaders/ToonMinimalFrag.spv", {&DSLG, &DSLOilLamp});

		PCup.init(this, &VD, "shaders/SimpleVert.spv", "shaders/CookTorranceCupFrag.spv", {&DSLG, &DSLCup});

		PKey.init(this, &VD, "shaders/KeyVert.spv", "shaders/CookTorranceKeyFrag.spv", {&DSLG, &DSLKey});

		PMoon.init(this, &VD, "shaders/SimpleVert.spv", "shaders/MoonFrag.spv", {&DSLG, &DSLMoon});
		// Models, textures and Descriptors (values assigned to the uniforms)
		// Create models
		// The second parameter is the pointer to the vertex definition for this model
		// The third parameter is the file name
		// The last is a constant specifying the file type: currently only OBJ, GLTF or the custom type MGCG

		// Pavement Model
		MPavement.init(this, &VD, "models/Pavement.obj", OBJ);

		// Box model
		MBox.init(this, &VD, "models/Cube.obj", OBJ);

		MPlatform.init(this, &VD, "models/platform.obj", OBJ);

		MLamp.init(this, &VD, "models/Lamp.obj", OBJ);

		MOilLamp.init(this, &VD, "models/Oil_lamp.obj", OBJ);

		MCup.init(this, &VD, "models/Cup.obj", OBJ);

		MKey.init(this, &VD, "models/Key.obj", OBJ);

		MMoon.init(this, &VD, "models/Moon.obj", OBJ);

		// Create the textures
		// The second parameter is the file name containing the image

		// Pavement Textures
		TPavDif.init(this, "textures/TPavDif.jpg");
		TPavSpec.init(this, "textures/TPavSpec.jpg");
		// Box Textures
		TCubeDiffuse.init(this, "textures/Cube_diffuse.jpg");
		TCubeSpecular.init(this, "textures/Cube_specular.jpg");
		TCubeAmbient.init(this, "textures/Cube_ambient.jpg");

		// plat textures
		TPlatDiffuse.init(this, "textures/platformBase.png");
		TPlatSpecular.init(this, "textures/platformSpec.png");

		// Lamp texture
		TLampDiffuse.init(this, "textures/lanternDiffuse.png");
		TLampSpecular.init(this, "textures/lanternSpecular.png");

		TOilLampDiffuse.init(this, "textures/Oil_lamp_Diffuse.png");
		TOilLampSpecular.init(this, "textures/Oil_lamp_Specular.png");

		TCupDiffuse.init(this, "textures/Cup_diffuse2.png");
		TCupSpecular.init(this, "textures/Cup_specular.png");

		TKeyDiffuse.init(this, "textures/KeyDiffuse.jpg");
		TKeySpecular.init(this, "textures/KeySpecular.jpg");

		TMoonDiffuse.init(this, "textures/MoonDiffuse.png");

		// MODIFY POOL SIZE

		// Descriptor pool sizes
		DPSZs.uniformBlocksInPool = 11; // Uniform blocks: Ubo, Gubo, PavParUbo
		DPSZs.texturesInPool = 16;		// Pavement: 3, Box: 4
		DPSZs.setsInPool = 9;			// Global set (0), Pavement set (1 in PPavement), Box set (1 in PBox)

		std::cout << "Initializing text\n";
		txt.init(this, &demoText);
	}

	// Here you create your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsInit()
	{
		// This creates a new pipeline (with the current surface), using its shaders
		PPavement.create();
		PBox.create();
		PPlatform.create();
		PLamp.create();
		POilLamp.create();
		PCup.create();
		PKey.create();
		PMoon.create();

		txt.pipelinesAndDescriptorSetsInit();

		// This creates a descriptor set, according to a given Layout. For texture bindings, the creation assigns the texture.
		// Second parameter is a pointer to the considered Descriptor Set Layout
		// Third parameter is a vector of pointer to textures. The DSL, for each texture, in its linkSize field (fourth element),
		// specifies the index of the texture in this array to pass to the shader

		DSPavement.init(this, &DSLPavement, {&TPavDif, &TPavSpec});
		DSBox.init(this, &DSLBox, {&TCubeDiffuse, &TCubeSpecular, &TCubeAmbient});
		DSPlatform.init(this, &DSLPlatform, {&TPlatDiffuse, &TPlatSpecular});
		DSLamp.init(this, &DSLLamp, {&TLampDiffuse, &TLampSpecular});
		DSOilLamp.init(this, &DSLOilLamp, {&TOilLampDiffuse, &TOilLampSpecular});
		DSCup.init(this, &DSLCup, {&TCupDiffuse, &TCupSpecular});
		DSKey.init(this, &DSLKey, {&TKeyDiffuse, &TKeySpecular});
		DSMoon.init(this, &DSLMoon, {&TMoonDiffuse});

		DSG.init(this, &DSLG, {}); // note that if a DSL has no texture, the array can be empty
	}

	// Here you destroy your pipelines and Descriptor Sets!
	void pipelinesAndDescriptorSetsCleanup()
	{
		txt.pipelinesAndDescriptorSetsCleanup();

		PPavement.cleanup();
		PBox.cleanup();
		PPlatform.cleanup();
		PLamp.cleanup();
		POilLamp.cleanup();
		PCup.cleanup();
		PKey.cleanup();
		PMoon.cleanup();

		DSPavement.cleanup();
		DSBox.cleanup();
		DSPlatform.cleanup();
		DSLamp.cleanup();
		DSOilLamp.cleanup();
		DSCup.cleanup();
		DSKey.cleanup();
		DSMoon.cleanup();
		DSG.cleanup();
	}

	// Here you destroy all the Models, Texture and Desc. Set Layouts you created!
	// You also have to destroy the pipelines
	void localCleanup()
	{
		TPavDif.cleanup();
		TPavSpec.cleanup();
		MPavement.cleanup();

		TCubeDiffuse.cleanup();
		TCubeSpecular.cleanup();
		TCubeAmbient.cleanup();
		MBox.cleanup();

		TPlatDiffuse.cleanup();
		TPlatSpecular.cleanup();
		MPlatform.cleanup();

		TLampDiffuse.cleanup();
		TLampSpecular.cleanup();
		MLamp.cleanup();

		TOilLampDiffuse.cleanup();
		TOilLampSpecular.cleanup();
		MOilLamp.cleanup();

		TCupDiffuse.cleanup();
		TCupSpecular.cleanup();
		MCup.cleanup();

		TKeyDiffuse.cleanup();
		TKeySpecular.cleanup();
		MKey.cleanup();

		TMoonDiffuse.cleanup();
		MMoon.cleanup();

		DSLPavement.cleanup();
		DSLBox.cleanup();
		DSLPlatform.cleanup();
		DSLLamp.cleanup();
		DSLOilLamp.cleanup();
		DSLCup.cleanup();
		DSLKey.cleanup();
		DSLMoon.cleanup();
		DSLG.cleanup();

		txt.localCleanup();
		PPavement.destroy();
		PBox.destroy();
		PPlatform.destroy();
		PLamp.destroy();
		POilLamp.destroy();
		PCup.destroy();
		PKey.destroy();
		PMoon.destroy();
	}

	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage)
	{
		// The resources that needs to be bound for drawing something, are:

		// The pipeline with the shader for the object

		// The models (both index and vertex buffers)

		// The descriptor sets, for each descriptor set specified in the pipeline
		// For this reason, the second parameter refers to the corresponding pipeline
		// And the third is the Set number to which the descriptor set should be bound

		txt.populateCommandBuffer(commandBuffer, currentImage, currText);

		PPavement.bind(commandBuffer);
		MPavement.bind(commandBuffer);
		DSG.bind(commandBuffer, PPavement, 0, currentImage);		// The Global Descriptor Set (Set 0)
		DSPavement.bind(commandBuffer, PPavement, 1, currentImage); // The Material and Position Descriptor Set (Set 1)
		vkCmdDrawIndexed(commandBuffer,
						 static_cast<uint32_t>(MPavement.indices.size()), 1, 0, 0, 0);

		PBox.bind(commandBuffer);
		MBox.bind(commandBuffer);
		DSG.bind(commandBuffer, PBox, 0, currentImage);	  // The Global Descriptor Set (Set 0)
		DSBox.bind(commandBuffer, PBox, 1, currentImage); // Again in set 1 since it's another pipeline
		vkCmdDrawIndexed(commandBuffer,
						 static_cast<uint32_t>(MBox.indices.size()), MAZE_SIZE * MAZE_SIZE * MAZE_HEIGHT, 0, 0, 0);

		PPlatform.bind(commandBuffer);
		MPlatform.bind(commandBuffer);
		DSG.bind(commandBuffer, PPlatform, 0, currentImage);
		DSPlatform.bind(commandBuffer, PPlatform, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
						 static_cast<uint32_t>(MPlatform.indices.size()), PLATFORM_NUMBER, 0, 0, 0);

		PLamp.bind(commandBuffer);
		MLamp.bind(commandBuffer);
		DSG.bind(commandBuffer, PLamp, 0, currentImage);
		DSLamp.bind(commandBuffer, PLamp, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
						 static_cast<uint32_t>(MLamp.indices.size()), WALL_LIGHTS_NUMBER, 0, 0, 0);

		POilLamp.bind(commandBuffer);
		MOilLamp.bind(commandBuffer);
		DSG.bind(commandBuffer, POilLamp, 0, currentImage);
		DSOilLamp.bind(commandBuffer, POilLamp, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
						 static_cast<uint32_t>(MOilLamp.indices.size()), 1, 0, 0, 0);

		PCup.bind(commandBuffer);
		MCup.bind(commandBuffer);
		DSG.bind(commandBuffer, PCup, 0, currentImage);
		DSCup.bind(commandBuffer, PCup, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
						 static_cast<uint32_t>(MCup.indices.size()), 1, 0, 0, 0);

		PKey.bind(commandBuffer);
		MKey.bind(commandBuffer);
		DSG.bind(commandBuffer, PKey, 0, currentImage);
		DSKey.bind(commandBuffer, PKey, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
						 static_cast<uint32_t>(MKey.indices.size()), KEYS_NUMBER, 0, 0, 0);

		PMoon.bind(commandBuffer);
		MMoon.bind(commandBuffer);
		DSG.bind(commandBuffer, PMoon, 0, currentImage);
		DSMoon.bind(commandBuffer, PMoon, 1, currentImage);
		vkCmdDrawIndexed(commandBuffer,
						 static_cast<uint32_t>(MMoon.indices.size()), 1, 0, 0, 0);
	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage)
	{
		static bool debounce = false;
		static int curDebounce = 0;

		float deltaT;
		glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
		bool fire = false;
		getSixAxis(deltaT, m, r, fire);
		player.move(deltaT, m, r, maze);
		

		//Close the window
		if (glfwGetKey(window, GLFW_KEY_ESCAPE)){
			glfwSetWindowShouldClose(window, GL_TRUE);
		}


		// Camera LookIn view
		glm::mat4 M = glm::perspective(glm::radians(45.0f), Ar, 0.1f, 50.0f);
		M[1][1] *= -1;
		glm::mat4 Mv = glm::rotate(glm::mat4(1.0), -player.getRotation().y, glm::vec3(1, 0, 0)) *
					   glm::rotate(glm::mat4(1.0), -player.getRotation().x, glm::vec3(0, 1, 0)) *
					   glm::translate(glm::mat4(1.0), -player.getPosition());
		glm::mat4 ViewPrj = M * Mv;

		// |||||| Global uniforms |||||||

		// hand lantern
		glm::vec4 oilLampOffset = glm::vec4(0.2f, -0.5f, -1.0f, 1.0f); // Offset in front of the camera
		glm::mat4 oilLampRotation = glm::rotate(glm::mat4(1.0f), player.getRotation().x, glm::vec3(0, 1, 0)) *
									glm::rotate(glm::mat4(1.0f), player.getRotation().y, glm::vec3(1, 0, 0));
		glm::vec3 oilLampPos = player.getPosition() + glm::vec3(oilLampRotation * oilLampOffset);

		// MaterialUniformBufferObject mubo{};

		// Gubo eye pos
		gubo.eyePos = player.getPosition();

		// Gubo hand light
		gubo.handLightPos = oilLampPos;
		if (uniformBuffersInit == false)
		{
			// Static values
			gubo.handLightColor = glm::vec4(0.6f, 0.6f, 0.428f, 3.0f);
			gubo.handLightDecayFactor = 2.1f;
		}

		// Pavement uniforms
		if (uniformBuffersInit == false)
		{
			// Static values
			pavUbo.mMat = glm::translate(glm::mat4(1.0f), glm::vec3(UNITARY_SCALE * MAZE_SIZE / 2, 0.0f, UNITARY_SCALE * MAZE_SIZE / 2 - 1.52f)) * glm::scale(glm::mat4(1.0f), glm::vec3(PAVEMENT_SCALE, 1.0f, PAVEMENT_SCALE));
			pavUbo.nMat = glm::inverse(glm::transpose(pavUbo.mMat));
			pavparubo.blinnGamma = 200.0f;
			pavparubo.balanceDiffuseSpecular = 0.8f;
		}
		pavUbo.mvpMat = ViewPrj * pavUbo.mMat;

		// Platform uniforms
		if (uniformBuffersInit == false)
		{
			// Static values
			platUbo.ubo[0].mMat = glm::translate(glm::mat4(1.0f), glm::vec3(maze->getEndPoint().c * UNITARY_SCALE, 0.0f, maze->getEndPoint().r * UNITARY_SCALE)) * glm::scale(glm::mat4(1.0f), glm::vec3(UNITARY_SCALE));
			platUbo.ubo[0].nMat = glm::inverse(glm::transpose(platUbo.ubo[0].mMat));
			platUbo.ubo[1].mMat = glm::translate(glm::mat4(1.0f), glm::vec3((float)(MAZE_SIZE)*UNITARY_SCALE / 2.0f + 6.0f, 0.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3((float)(MAZE_SIZE)*UNITARY_SCALE / 2.0f, 0.0f, CENTRE_PAV_Z)) * glm::scale(glm::mat4(1.0f), glm::vec3(UNITARY_SCALE));
			platUbo.ubo[1].nMat = glm::inverse(glm::transpose(platUbo.ubo[1].mMat));
		}
		platUbo.ubo[0].mvpMat = ViewPrj * platUbo.ubo[0].mMat;
		platUbo.ubo[1].mvpMat = ViewPrj * platUbo.ubo[1].mMat;

		// Keys uniforms
		int i = 0;
		int temp = 0;
		for (Key key : *maze->getMazeKeys())
		{
			if (!key.isTaken)
				keyUbo.ubo[i].mMat = glm::translate(glm::mat4(1.0f), glm::vec3(key.point.c * UNITARY_SCALE, 0.4, key.point.r * UNITARY_SCALE));
			else{
				temp++;
				keyUbo.ubo[i].mMat = glm::translate(glm::mat4(1.0f), glm::vec3(key.point.c * UNITARY_SCALE, -20, key.point.r * UNITARY_SCALE));
			}
			keyUbo.ubo[i].nMat = glm::inverse(glm::transpose(keyUbo.ubo[i].mMat));
			keyUbo.ubo[i].mvpMat = ViewPrj * keyUbo.ubo[i].mMat;
			i++;
		}

		//Set the text to display
		if(temp != currText && currText<=KEYS_NUMBER){
			currText = temp;
			RebuildPipeline();
		}else if(player.isTeleported()&&currText!=KEYS_NUMBER+1){
			currText = KEYS_NUMBER + 1;
			RebuildPipeline();
		}	
		

		if (uniformBuffersInit == false)
		{
			// Static values
			cupUbo.mMat = glm::translate(glm::mat4(1.0f), glm::vec3((float)(MAZE_SIZE)*UNITARY_SCALE / 2.0f + 10.0f + (float)(MAZE_SIZE)*UNITARY_SCALE / 2.0f, 0.0f, CENTRE_PAV_Z)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			cupUbo.nMat = glm::inverse(glm::transpose(cupUbo.mMat));
		}
		cupUbo.mvpMat = ViewPrj * cupUbo.mMat;

		if (uniformBuffersInit == false)
		{
			// Static values
			moonUbo.mMat = glm::translate(glm::mat4(1.0f), glm::vec3((float)(MAZE_SIZE)*UNITARY_SCALE / 2.0f + 10.0f + (float)(MAZE_SIZE)*UNITARY_SCALE / 2.0f + 20.0f, 10.0f, CENTRE_PAV_Z)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.5f, 2.5f, 2.5f));
			moonUbo.nMat = glm::inverse(glm::transpose(moonUbo.mMat));
		}
		moonUbo.mvpMat = ViewPrj * moonUbo.mMat;

		// Cup lights
		if (uniformBuffersInit == false)
		{
			// Static values
			gubo.cupLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 3.0f);
			gubo.cupLightDecayFactor = 1.3f;
			gubo.cupLightDir = glm::vec3(0.0f, -1.0f, 0.0f);
			gubo.cupLightPos = glm::vec3((float)(MAZE_SIZE)*UNITARY_SCALE / 2.0f + 10.0f + (float)(MAZE_SIZE)*UNITARY_SCALE / 2.0f, 1.5f, CENTRE_PAV_Z);
			gubo.cupCIN = glm::cos(glm::radians(1.0f));
			gubo.cupCOUT = glm::cos(glm::radians(4.0f));
		}

		// Oil lamp uniforms (nothing static here)
		oilLampUbo.mMat = glm::translate(glm::mat4(1.0f), oilLampPos) * oilLampRotation * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f));
		oilLampUbo.nMat = glm::inverse(glm::transpose(oilLampUbo.mMat));
		oilLampUbo.mvpMat = ViewPrj * oilLampUbo.mMat;

		// Place lamps on wall and lights on their long part

		if (uniformBuffersInit == false)
		{
			// Static values
			gubo.wallLampColor = glm::vec4(0.7f, 0.7f, 0.7f, 3.0f);
			gubo.wallLampDecayFactor = 2.0f;
		}

		int l = 0;
		for (Light light : maze->getMazeLights())
		{
			if (uniformBuffersInit == false)
			{
				// Lights models and light placement are static
				if (light.direction == Direction::UP)
				{
					// Lamp object
					lampUbo.ubo[l].mMat = glm::translate(glm::mat4(1.0f), glm::vec3(light.point.c * UNITARY_SCALE, UNITARY_SCALE + (UNITARY_SCALE / 2), light.point.r * UNITARY_SCALE - UNITARY_SCALE / 2 - 0.2f)) * glm::scale(glm::mat4(1.0f), glm::vec3(4.0f));
					// Light
					gubo.wallLampPos[l] = glm::vec4(light.point.c * UNITARY_SCALE, UNITARY_SCALE - 0.3f, light.point.r * UNITARY_SCALE - UNITARY_SCALE / 2 + 1.1f, 0.0f);
				}
				else if (light.direction == Direction::DOWN)
				{
					// Lamp object
					lampUbo.ubo[l].mMat = glm::translate(glm::mat4(1.0f), glm::vec3(light.point.c * UNITARY_SCALE, UNITARY_SCALE + (UNITARY_SCALE / 2), light.point.r * UNITARY_SCALE + UNITARY_SCALE / 2 + 0.2f)) * glm::scale(glm::mat4(1.0f), glm::vec3(4.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(-180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					// Light
					gubo.wallLampPos[l] = glm::vec4(light.point.c * UNITARY_SCALE, UNITARY_SCALE - 0.3f, light.point.r * UNITARY_SCALE + UNITARY_SCALE / 2 - 1.1f, 0.0f);
				}

				else if (light.direction == Direction::RIGHT)
				{
					// Lamp object
					lampUbo.ubo[l].mMat = glm::translate(glm::mat4(1.0f), glm::vec3(light.point.c * UNITARY_SCALE + UNITARY_SCALE / 2 + 0.2f, UNITARY_SCALE + (UNITARY_SCALE / 2), light.point.r * UNITARY_SCALE)) * glm::scale(glm::mat4(1.0f), glm::vec3(4.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					// Light
					gubo.wallLampPos[l] = glm::vec4(light.point.c * UNITARY_SCALE + UNITARY_SCALE / 2 - 1.1f, UNITARY_SCALE - 0.3f, light.point.r * UNITARY_SCALE, 0.0f);
				}
				else if (light.direction == Direction::LEFT)
				{
					// Lamp object
					lampUbo.ubo[l].mMat = glm::translate(glm::mat4(1.0f), glm::vec3(light.point.c * UNITARY_SCALE - UNITARY_SCALE / 2 - 0.2f, UNITARY_SCALE + (UNITARY_SCALE / 2), light.point.r * UNITARY_SCALE)) * glm::scale(glm::mat4(1.0f), glm::vec3(4.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					// Light
					gubo.wallLampPos[l] = glm::vec4(light.point.c * UNITARY_SCALE - UNITARY_SCALE / 2 + 1.1f, UNITARY_SCALE - 0.3f, light.point.r * UNITARY_SCALE, 0.0f);
				}
				lampUbo.ubo[l].nMat = glm::inverse(glm::transpose(lampUbo.ubo[l].mMat));
			}
			lampUbo.ubo[l].mvpMat = ViewPrj * lampUbo.ubo[l].mMat;
			l++;
		}

		// gubo.wallLampPos[0] = glm::vec4(0.0f, 6.5f, 0.0f, 0.0f);
		// gubo.wallLampPos[1] = glm::vec4(9.0f, 6.5f, 9.0f, 0.0f);
		// gubo.wallLampPos[2] = glm::vec4(19.0f, 6.5f, 19.0f, 0.0f);

		int row, col, h;
		for (row = 0; row < MAZE_SIZE; row++)
		{
			for (col = 0; col < MAZE_SIZE; col++)
			{
				for (h = 0; h < MAZE_HEIGHT; h++)
				{
					if (uniformBuffersInit == false)
					{
						// Maze placement in uniforms is done just once
						if (maze->getMazeMap()[row][col] == MazeWay::WALL)
						{
							mazeUbo.mMat[row][col][h] = glm::translate(glm::mat4(1.0f), glm::vec3(UNITARY_SCALE * (float)(col), UNITARY_SCALE * (float)h + (row == 0 && col == 0 ? 1.0f : 0.0f), UNITARY_SCALE * (float)(row))) * glm::scale(glm::mat4(1.0f), glm::vec3(UNITARY_SCALE));
						}
						else
						{
							mazeUbo.mMat[row][col][h] = glm::translate(glm::mat4(1.0f), glm::vec3(UNITARY_SCALE * (float)(col), -20 - 2 * (float)h, UNITARY_SCALE * (float)(row))) * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
						}
						mazeUbo.nMat[row][col][h] = glm::inverse(glm::transpose(mazeUbo.mMat[row][col][h]));
					}
					mazeUbo.mvpMat[row][col][h] = ViewPrj * mazeUbo.mMat[row][col][h];
				}
			}
		}

		// Boxes blinn parameters
		if (uniformBuffersInit == false)
		{
			boxparubo.blinnGamma = 200.0f;
			boxparubo.balanceDiffuseSpecular = 0.8f;
			boxparubo.ambientFactor = 0.05f;
		}

		// Define pavement texture parameters
		if (uniformBuffersInit == false)
		{
			pavparubo.uScale = UV_PAVEMENT_SCALE;
			pavparubo.vScale = UV_PAVEMENT_SCALE;
		}

		// Maps (transfers to the shader) the global Descriptor Set
		DSG.map(currentImage, &gubo, 0);

		// Maps (transfers to the shader) the uniform buffers of the Descriptor Set containing the
		// transform matrices (in Binding 0), and the material parameters (in Binding 2).
		DSPavement.map(currentImage, &pavUbo, 0);
		DSPavement.map(currentImage, &pavparubo, 3);

		DSBox.map(currentImage, &mazeUbo, 0);
		DSBox.map(currentImage, &boxparubo, 4);

		DSPlatform.map(currentImage, &platUbo, 0);

		DSLamp.map(currentImage, &lampUbo, 0);

		DSOilLamp.map(currentImage, &oilLampUbo, 0);

		DSCup.map(currentImage, &cupUbo, 0);

		DSKey.map(currentImage, &keyUbo, 0);

		DSMoon.map(currentImage, &moonUbo, 0);
		uniformBuffersInit = true; // Initialization completed
	}
};

int main()
{
	srand(8);
	std::cout << "Starting with maze size " << MAZE_SIZE << std::endl;
	
	//Setup Texts To Be Displayed
    std::vector<std::string> texts;
    texts.push_back("Find all the " + std::to_string(KEYS_NUMBER) + " keys!");
    demoText.push_back({2, {texts.back().c_str(), "Then go on the teleportation platform", "", ""}, 0, 0});

    for (int i = KEYS_NUMBER - 1; i > 0; i--) {
        texts.push_back("You still have to find " + std::to_string(i) + " keys!");
        demoText.push_back({1, {texts.back().c_str(), "", "", ""}, 0, 0});
    }

	demoText.push_back({2, {"You found all the keys", "Search for the teleportation platform and step on it", "", ""}, 0, 0});
	demoText.push_back({2, {"Congratulations, YOU ESCAPED", "PRESS ESC TO QUIT", "", ""}, 0, 0});

	Project app;
	try
	{
		app.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

// ubo[MAZE_SIZE][MAZE_SIZE][MAZE_HEIGHT]