/**
* The Vienna Vulkan Engine
*
* (c) bei Helmut Hlavacs, University of Vienna
*
*/


#include "VEInclude.h"



namespace ve {


	uint32_t g_score = 0;				//derzeitiger Punktestand
	double g_time = 10.0;				//zeit die noch übrig ist
	bool g_gameLost = false;			//true... das Spiel wurde verloren
	bool g_restart = false;			//true...das Spiel soll neu gestartet werden

	//
	//Zeichne das GUI
	//
	class EventListenerGUI : public VEEventListener {
	protected:

		virtual void onDrawOverlay(veEvent event) {
			VESubrenderFW_Nuklear* pSubrender = (VESubrenderFW_Nuklear*)getRendererPointer()->getOverlay();
			if (pSubrender == nullptr) return;

			struct nk_context* ctx = pSubrender->getContext();

			if (!g_gameLost) {
				if (nk_begin(ctx, "", nk_rect(0, 0, 200, 170), NK_WINDOW_BORDER)) {
					char outbuffer[100];
					nk_layout_row_dynamic(ctx, 45, 1);
					sprintf(outbuffer, "Score: %03d", g_score);
					nk_label(ctx, outbuffer, NK_TEXT_LEFT);

					nk_layout_row_dynamic(ctx, 45, 1);
					sprintf(outbuffer, "Time: %004.1lf", g_time);
					nk_label(ctx, outbuffer, NK_TEXT_LEFT);
				}
			}
			else {
				if (nk_begin(ctx, "", nk_rect(500, 500, 200, 170), NK_WINDOW_BORDER)) {
					nk_layout_row_dynamic(ctx, 45, 1);
					nk_label(ctx, "Game Over", NK_TEXT_LEFT);
					if (nk_button_label(ctx, "Restart")) {
						g_restart = true;
					}
				}

			};

			nk_end(ctx);
		}

	public:
		///Constructor of class EventListenerGUI
		EventListenerGUI(std::string name) : VEEventListener(name) { };

		///Destructor of class EventListenerGUI
		virtual ~EventListenerGUI() {};
	};


	static std::default_random_engine e{ 12345 };					//Für Zufallszahlen
	static std::uniform_real_distribution<> d{ -10.0f, 10.0f };		//Für Zufallszahlen

	//
	// Überprüfen, ob die Kamera die Kiste berührt
	//
	class EventListenerCollision : public VEEventListener {
	protected:
		virtual void onFrameStarted(veEvent event) {
			static uint32_t cubeid = 0;

			if (g_restart) {
				g_gameLost = false;
				g_restart = false;
				g_time = 30;
				g_score = 0;
				getSceneManagerPointer()->getSceneNode("The Cube Parent")->setPosition(glm::vec3(d(e), 1.0f, d(e)));
				getEnginePointer()->m_irrklangEngine->play2D("media/sounds/ophelia.mp3", true);
				return;
			}
			if (g_gameLost) return;

			glm::vec3 positionCube = getSceneManagerPointer()->getSceneNode("The Cube Parent")->getPosition();
			glm::vec3 positionPlayer = getSceneManagerPointer()->getSceneNode("The Player Parent")->getPosition();
			glm::vec3 positionCamera = getSceneManagerPointer()->getSceneNode("StandardCameraParent")->getPosition();

			float distanceCube = glm::length(positionCube - positionCamera);
			float distancePlayer = glm::length(positionPlayer - positionCamera);
			if (distanceCube < 1 || distancePlayer < 1) {
				g_score++;
				getEnginePointer()->m_irrklangEngine->play2D("media/sounds/explosion.wav", false);
				if (g_score % 10 == 0) {
					g_time = 30;
					getEnginePointer()->m_irrklangEngine->play2D("media/sounds/bell.wav", false);
				}

				if (distanceCube < 1) {
					VESceneNode* eParent = getSceneManagerPointer()->getSceneNode("The Cube Parent");
					eParent->setPosition(glm::vec3(d(e), 1.0f, d(e)));

					getSceneManagerPointer()->deleteSceneNodeAndChildren("The Cube" + std::to_string(cubeid));
					VECHECKPOINTER(getSceneManagerPointer()->loadModel("The Cube" + std::to_string(++cubeid), "media/models/test/crate0", "cube.obj", 0, eParent));
				}
				if (distancePlayer < 1) {
					VESceneNode* eParent = getSceneManagerPointer()->getSceneNode("The Player Parent");
					eParent->setPosition(glm::vec3(d(e), 1.0f, d(e)));

					getSceneManagerPointer()->deleteSceneNodeAndChildren("The Player" + std::to_string(cubeid));
					VECHECKPOINTER(getSceneManagerPointer()->loadModel("The Player" + std::to_string(++cubeid), "media/models/test/crate1", "cube.obj", 0, eParent));
				}
			}

			g_time -= event.dt;
			if (g_time <= 0) {
				g_gameLost = true;
				getEnginePointer()->m_irrklangEngine->removeAllSoundSources();
				getEnginePointer()->m_irrklangEngine->play2D("media/sounds/gameover.wav", false);
			}
		};

	public:
		///Constructor of class EventListenerCollision
		EventListenerCollision(std::string name) : VEEventListener(name) { };

		///Destructor of class EventListenerCollision
		virtual ~EventListenerCollision() {};
	};

	class EventListenerMovement : public VEEventListenerGLFW {
	protected:
		bool onKeyboard(veEvent event) {
			VEEventListenerGLFW::onKeyboard(event);

			glm::vec4 translate = glm::vec4(0.0, 0.0, 0.0, 1.0);	//total translation
			glm::vec4 rot4 = glm::vec4(1.0);						//total rotation around the axes, is 4d !
			float angle = 0.0;
			float rotSpeed = 2.0;

			VESceneNode* pPlayer = getSceneManagerPointer()->getSceneNode("The PLayer0");
			VESceneNode* pParent = pPlayer->getParent();

			switch (event.idata1) {
			case GLFW_KEY_A:
				translate = pPlayer->getTransform() * glm::vec4(-1.0, 0.0, 0.0, 1.0);	//left
				break;
			case GLFW_KEY_D:
				translate = pPlayer->getTransform() * glm::vec4(1.0, 0.0, 0.0, 1.0); //right
				break;
			case GLFW_KEY_W:
				translate = pPlayer->getTransform() * glm::vec4(0.0, 0.0, 1.0, 1.0); //forward
				translate.y = 0.0f;
				break;
			case GLFW_KEY_S:
				translate = pPlayer->getTransform() * glm::vec4(0.0, 0.0, -1.0, 1.0); //back
				translate.y = 0.0f;
				break;
			case GLFW_KEY_Q:
				translate = glm::vec4(0.0, -1.0, 0.0, 1.0); //down
				break;
			case GLFW_KEY_E:
				translate = glm::vec4(0.0, 1.0, 0.0, 1.0);  //up
				break;
			case GLFW_KEY_LEFT:							//yaw rotation is already in parent space
				angle = rotSpeed * (float)event.dt * -1.0f;
				rot4 = glm::vec4(0.0, 1.0, 0.0, 1.0);
				break;
			case GLFW_KEY_RIGHT:						//yaw rotation is already in parent space
				angle = rotSpeed * (float)event.dt * 1.0f;
				rot4 = glm::vec4(0.0, 1.0, 0.0, 1.0);
				break;
			case GLFW_KEY_UP:							//pitch rotation is in cam/local space
				angle = rotSpeed * (float)event.dt * 1.0f;			//pitch angle
				rot4 = pPlayer->getTransform() * glm::vec4(1.0, 0.0, 0.0, 1.0); //x axis from local to parent space!
				break;
			case GLFW_KEY_DOWN:							//pitch rotation is in cam/local space
				angle = rotSpeed * (float)event.dt * -1.0f;		//pitch angle
				rot4 = pPlayer->getTransform() * glm::vec4(1.0, 0.0, 0.0, 1.0); //x axis from local to parent space!
				break;

			default:
				return false;
			};

			if (pParent == nullptr) {
				pParent = pPlayer;
			}

			///add the new translation vector to the previous one
			float speed = 6.0f;
			glm::vec3 trans = speed * glm::vec3(translate.x, translate.y, translate.z);
			pParent->multiplyTransform(glm::translate(glm::mat4(1.0f), (float)event.dt * trans));

			///combination of yaw and pitch, both wrt to parent space
			glm::vec3  rot3 = glm::vec3(rot4.x, rot4.y, rot4.z);
			glm::mat4  rotate = glm::rotate(glm::mat4(1.0), angle, rot3);
			pPlayer->multiplyTransform(rotate);

			return true;
		}

	public:
		///Constructor of class EventListenerCollision
		EventListenerMovement(std::string name) : VEEventListenerGLFW(name) { };

		///Destructor of class EventListenerCollision
		virtual ~EventListenerMovement() {};
	};

	///user defined manager class, derived from VEEngine
	class MyVulkanEngine : public VEEngine {
	public:

		MyVulkanEngine(bool debug = false) : VEEngine(debug) {};
		~MyVulkanEngine() {};


		///Register an event listener to interact with the user

		virtual void registerEventListeners() {
			//VEEngine::registerEventListeners();

			registerEventListener(new EventListenerMovement("Movement"));
			registerEventListener(new EventListenerCollision("Collision"), { veEvent::VE_EVENT_FRAME_STARTED });
			registerEventListener(new EventListenerGUI("GUI"), { veEvent::VE_EVENT_DRAW_OVERLAY });
		};


		///Load the first level into the game engine
		///The engine uses Y-UP, Left-handed
		virtual void loadLevel(uint32_t numLevel = 1) {

			VESceneNode* cameraParent = getSceneManagerPointer()->createSceneNode("StandardCameraParent", getRoot(),
				glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

			//camera can only do yaw (parent y-axis) and pitch (local x-axis) rotations
			VkExtent2D extent = getWindowPointer()->getExtent();
			VECameraProjective* camera = (VECameraProjective*)getSceneManagerPointer()->createCamera("StandardCamera", VECamera::VE_CAMERA_TYPE_PROJECTIVE, cameraParent);
			camera->m_nearPlane = 0.1f;
			camera->m_farPlane = 500.1f;
			camera->m_aspectRatio = extent.width / (float)extent.height;
			camera->m_fov = 45.0f;
			camera->lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			getSceneManagerPointer()->setCamera(camera);

			VELight* light4 = (VESpotLight*)getSceneManagerPointer()->createLight("StandardAmbientLight", VELight::VE_LIGHT_TYPE_AMBIENT, camera);
			light4->m_col_ambient = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);

			//use one light source
			VELight* light1 = (VEDirectionalLight*)getSceneManagerPointer()->createLight("StandardDirLight", VELight::VE_LIGHT_TYPE_DIRECTIONAL, getRoot());     //new VEDirectionalLight("StandardDirLight");
			light1->lookAt(glm::vec3(0.0f, 20.0f, -20.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			light1->m_col_diffuse = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
			light1->m_col_specular = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);

			registerEventListeners();

			VESceneNode* pScene;
			VECHECKPOINTER(pScene = getSceneManagerPointer()->createSceneNode("Level 1", getRoot()));

			//scene models

			VESceneNode* sp1;
			VECHECKPOINTER(sp1 = getSceneManagerPointer()->createSkybox("The Sky", "media/models/test/sky/cloudy",
				{ "yellowcloud_ft.jpg", "yellowcloud_bk.jpg", "yellowcloud_up.jpg",
					"yellowcloud_dn.jpg", "yellowcloud_rt.jpg", "yellowcloud_lf.jpg" }, pScene));

			VESceneNode* e4;
			VECHECKPOINTER(e4 = getSceneManagerPointer()->loadModel("The Plane", "media/models/test", "plane_t_n_s.obj", 0, pScene));
			e4->setTransform(glm::scale(glm::mat4(1.0f), glm::vec3(1000.0f, 1.0f, 1000.0f)));

			VEEntity* pE4;
			VECHECKPOINTER(pE4 = (VEEntity*)getSceneManagerPointer()->getSceneNode("The Plane/plane_t_n_s.obj/plane/Entity_0"));
			pE4->setParam(glm::vec4(1000.0f, 1000.0f, 0.0f, 0.0f));

			VESceneNode* e1, * eParent;
			eParent = getSceneManagerPointer()->createSceneNode("The Cube Parent", pScene, glm::mat4(1.0));
			VECHECKPOINTER(e1 = getSceneManagerPointer()->loadModel("The Cube0", "media/models/test/crate0", "cube.obj"));
			eParent->multiplyTransform(glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, 1.0f, 10.0f)));
			eParent->addChild(e1);

			VESceneNode* e2;
			eParent = getSceneManagerPointer()->createSceneNode("The Player Parent", pScene, glm::mat4(1.0));
			VECHECKPOINTER(e2 = getSceneManagerPointer()->loadModel("The PLayer0", "media/models/test/crate1", "cube.obj"));
			eParent->multiplyTransform(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)));
			eParent->addChild(e2);


			m_irrklangEngine->play2D("media/sounds/ophelia.mp3", true);

		};
	};


}

using namespace ve;

int main() {

	bool debug = true;

	MyVulkanEngine mve(debug);	//enable or disable debugging (=callback, validation layers)

	mve.initEngine();
	mve.loadLevel(1);
	mve.run();

	return 0;
}

