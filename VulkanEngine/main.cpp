/**
* The Vienna Vulkan Engine
*
* (c) bei Helmut Hlavacs, University of Vienna
*
*/


#include "VEInclude.h"
#include "glm/ext.hpp"



namespace ve {


	uint32_t g_score = 0;				//derzeitiger Punktestand
	double g_time = 30.0;				//zeit die noch übrig ist
	bool g_gameLost = false;			//true... das Spiel wurde verloren
	bool g_gameWon = false;			//true... das Spiel wurde gewonnen
	bool g_restart = false;			//true...das Spiel soll neu gestartet werden
	int g_difficulty = 1;			//true...das Spiel soll neu gestartet werden
	std::list<int> cubeIds;

	//
	//Zeichne das GUI
	//
	class EventListenerGUI : public VEEventListener {
	protected:

		virtual void onDrawOverlay(veEvent event) {
			VESubrenderFW_Nuklear* pSubrender = (VESubrenderFW_Nuklear*)getRendererPointer()->getOverlay();
			if (pSubrender == nullptr) return;

			struct nk_context* ctx = pSubrender->getContext();

			if (!g_gameLost && !g_gameWon) {
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
			else if (g_gameWon) {
				if (nk_begin(ctx, "", nk_rect(500, 500, 200, 170), NK_WINDOW_BORDER)) {
					nk_layout_row_dynamic(ctx, 45, 1);
					nk_label(ctx, "Game Won!!!", NK_TEXT_LEFT);
					if (nk_button_label(ctx, "Next Level")) {
						g_restart = true;
					}
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
	// Überprüfen, ob der Spieler die Kiste berührt
	//
	class EventListenerCollision : public VEEventListener {
	private:
		std::list<int> deletedCubeIds;

	protected:
		virtual void onFrameStarted(veEvent event) {
			static uint32_t cubeid = 0;

			if (g_restart) {
				g_gameLost = false;
				g_gameWon = false;
				g_restart = false;
				g_time = 30;
				g_score = 0;
				PlaceCubes(getSceneManagerPointer()->getSceneNode("Level 1"), g_difficulty);
				getSceneManagerPointer()->getSceneNode("The Player Parent")->setPosition(glm::vec3(0.0f, 1.0f, 5.0f));
				getEnginePointer()->m_irrklangEngine->play2D("media/sounds/ophelia.mp3", true);
				return;
			}
			if (g_gameLost || g_gameWon) return;

			glm::vec3 positionPlayer = getSceneManagerPointer()->getSceneNode("The Player Parent")->getPosition();
			for (std::list<int>::iterator it = cubeIds.begin(); it != cubeIds.end(); ++it) {

				glm::vec3 positionCube = getSceneManagerPointer()->getSceneNode("The Cube" + std::to_string(*it) + " Parent")->getPosition();

				float distanceCube = glm::length(positionCube - positionPlayer);
				if (distanceCube < 1) {
					g_score++;
					getEnginePointer()->m_irrklangEngine->play2D("media/sounds/explosion.wav", false);
					if (g_score % 10 == 0) {
						g_time = 30;
						getEnginePointer()->m_irrklangEngine->play2D("media/sounds/bell.wav", false);
					}

					if (distanceCube < 1) {
						getSceneManagerPointer()->deleteSceneNodeAndChildren("The Cube" + std::to_string(*it));
						deletedCubeIds.push_back(*it);
						cubeIds.remove(*it);
						break;
					}
				}
			}

			g_time -= event.dt;
			if (cubeIds.size() == 0) {
				g_gameWon = true;
				g_difficulty++;
				getEnginePointer()->m_irrklangEngine->removeAllSoundSources();
				getEnginePointer()->m_irrklangEngine->play2D("media/sounds/bell.wav", false);
			}
			if (g_time <= 0) {
				g_gameLost = true;
				getEnginePointer()->m_irrklangEngine->removeAllSoundSources();
				getEnginePointer()->m_irrklangEngine->play2D("media/sounds/gameover.wav", false);
			}
		};

	public:
		///Constructor of class EventListenerCollision
		EventListenerCollision(std::string name) : VEEventListener(name) {};

		///Destructor of class EventListenerCollision
		virtual ~EventListenerCollision() {};

		void PlaceCubes(VESceneNode* pScene, int dif = 1) {
			for (int i = 0; i < dif * 10; i++) {
				VESceneNode* e1, * e1Parent;
				e1Parent = getSceneManagerPointer()->createSceneNode("The Cube" + std::to_string(i) + " Parent", pScene, glm::mat4(1.0));
				VECHECKPOINTER(e1 = getSceneManagerPointer()->loadModel("The Cube" + std::to_string(i), "media/models/test/crate0", "cube.obj"));
				for (std::list<int>::iterator it = deletedCubeIds.begin(); it != deletedCubeIds.end(); ++it) {
					if (i == *it) {
						e1Parent->multiplyTransform(glm::translate(glm::mat4(1.0f), glm::vec3(d(e), 0.0f, d(e))));
						deletedCubeIds.remove(*it);
						break;
					}
					if (*it == deletedCubeIds.size() + 1)
						e1Parent->multiplyTransform(glm::translate(glm::mat4(1.0f), glm::vec3(d(e), -1.0f, d(e))));
				}
				e1Parent->addChild(e1);
				cubeIds.push_back(i);
			}
		}
	};

	//
	//Adjust the camera to look at the player
	//
	class EventListenerCameraMovement : public VEEventListener {

	protected:
		virtual void onFrameStarted(veEvent event) {
			glm::vec3 positionPlayer = getSceneManagerPointer()->getSceneNode("The Player Parent")->getPosition();
			ve::VESceneNode* pCamera = getSceneManagerPointer()->getCamera()->getParent();

			pCamera->lookAt(
				glm::vec3(positionPlayer.x, positionPlayer.y + 30, positionPlayer.z),
				glm::vec3(positionPlayer.x, positionPlayer.y, positionPlayer.z),
				glm::vec3(0, 1, 0));
		};


	public:
		///Constructor of class EventListenerCameraMocement
		EventListenerCameraMovement(std::string name) : VEEventListener(name) { };

		///Destructor of class EventListenerCameraMocement
		virtual ~EventListenerCameraMovement() {};
	};

	//
	//Adjust the camera to look at the player
	//
	class EventListenerKeyboardBindings : public VEEventListenerGLFW {

	protected:
		virtual bool onKeyboard(veEvent event) {
			if (event.idata1 == GLFW_KEY_ESCAPE) {				//ESC pressed - end the engine
				getEnginePointer()->end();
				return true;
			}

			if (event.idata3 == GLFW_RELEASE) return false;

			if (event.idata1 == GLFW_KEY_P && event.idata3 == GLFW_PRESS) {
				m_makeScreenshot = true;
				return false;
			}
			if (event.idata1 == GLFW_KEY_O && event.idata3 == GLFW_PRESS) {
				m_makeScreenshotDepth = true;
				return false;
			}

			///create some default constants for the actions 
			float speed;
			glm::vec3 trans;
			glm::vec3  rot3;
			glm::mat4  rotate;

			glm::vec4 translate = glm::vec4(0.0, 0.0, 0.0, 1.0);	//total translation
			glm::vec4 rot4 = glm::vec4(1.0);						//total rotation around the axes, is 4d !
			float angle = 0.0;
			float rotSpeed = 2.0;
			constexpr float rotTransSpeed = glm::pi<float>();

			VESceneNode* pPlayer = getSceneManagerPointer()->getSceneNode("The PLayer");
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
			speed = 6.0f;
			trans = speed * glm::vec3(translate.x, translate.y, translate.z);
			pParent->multiplyTransform(glm::translate(glm::mat4(1.0f), (float)event.dt * trans));

			///combination of yaw and pitch, both wrt to parent space
			rot3 = glm::vec3(rot4.x, rot4.y, rot4.z);
			rotate = glm::rotate(glm::mat4(1.0), angle, rot3);
			pPlayer->multiplyTransform(rotate);

			return true;
		}


	public:
		///Constructor of class EventListenerCameraMocement
		EventListenerKeyboardBindings(std::string name) : VEEventListenerGLFW(name) { };

		///Destructor of class EventListenerCameraMocement
		virtual ~EventListenerKeyboardBindings() {};
	};


	///user defined manager class, derived from VEEngine
	class MyVulkanEngine : public VEEngine {
	public:

		MyVulkanEngine(bool debug = false) : VEEngine(debug) {};
		~MyVulkanEngine() {};


		///Register an event listener to interact with the user

		virtual void registerEventListeners() {
			registerEventListener(new EventListenerKeyboardBindings("KeyboardBindings"));
			registerEventListener(new EventListenerCameraMovement("CameraMovement"), { veEvent::VE_EVENT_FRAME_STARTED });
			registerEventListener(new EventListenerCollision("Collision"), { veEvent::VE_EVENT_FRAME_STARTED });
			registerEventListener(new EventListenerGUI("GUI"), { veEvent::VE_EVENT_DRAW_OVERLAY });
		};


		///Load the first level into the game engine
		///The engine uses Y-UP, Left-handed
		virtual void loadLevel(uint32_t numLevel = 1) {


			VEEngine::loadLevel(numLevel);

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

			VESceneNode* e2, * e2Parent;
			e2Parent = getSceneManagerPointer()->createSceneNode("The Player Parent", pScene, glm::mat4(1.0));
			VECHECKPOINTER(e2 = getSceneManagerPointer()->loadModel("The PLayer", "media/models/test/crate1", "cube.obj"));
			e2Parent->multiplyTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, -15.0f)));
			e2Parent->addChild(e2);

			PlaceCubes(pScene);

			m_irrklangEngine->play2D("media/sounds/ophelia.mp3", true);
		};

		void PlaceCubes(VESceneNode* pScene) {
			for (int i = 0; i < 10; i++) {
				VESceneNode* e1, * e1Parent;
				e1Parent = getSceneManagerPointer()->createSceneNode("The Cube" + std::to_string(i) + " Parent", pScene, glm::mat4(1.0));
				VECHECKPOINTER(e1 = getSceneManagerPointer()->loadModel("The Cube" + std::to_string(i), "media/models/test/crate0", "cube.obj"));
				e1Parent->multiplyTransform(glm::translate(glm::mat4(1.0f), glm::vec3(d(e), 1.0f, d(e))));
				e1Parent->addChild(e1);
				cubeIds.push_back(i);
			}
		}
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

