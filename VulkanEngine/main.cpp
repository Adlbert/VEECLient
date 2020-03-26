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
				getSceneManagerPointer()->getSceneNode("The Cube0 Parent")->setPosition(glm::vec3(d(e), 1.0f, d(e)));
				//getSceneManagerPointer()->getSceneNode("The Player Parent")->setPosition(glm::vec3(d(e), 1.0f, d(e)));
				//getSceneManagerPointer()->getSceneNode("StandardCameraParent")->setPosition(glm::vec3(d(e), 1.0f, d(e)));
				getEnginePointer()->m_irrklangEngine->play2D("media/sounds/ophelia.mp3", true);
				return;
			}
			if (g_gameLost) return;

			glm::vec3 positionCube = getSceneManagerPointer()->getSceneNode("The Cube0 Parent")->getPosition();
			glm::vec3 positionPlayer = getSceneManagerPointer()->getSceneNode("The Player Parent")->getPosition();

			float distanceCube = glm::length(positionCube - positionPlayer);
			if (distanceCube < 1) {
				g_score++;
				getEnginePointer()->m_irrklangEngine->play2D("media/sounds/explosion.wav", false);
				if (g_score % 10 == 0) {
					g_time = 30;
					getEnginePointer()->m_irrklangEngine->play2D("media/sounds/bell.wav", false);
				}

				if (distanceCube < 1) {
					VESceneNode* eParent = getSceneManagerPointer()->getSceneNode("The Cube0 Parent");
					eParent->setPosition(glm::vec3(d(e), 1.0f, d(e)));

					getSceneManagerPointer()->deleteSceneNodeAndChildren("The Cube" + std::to_string(cubeid));
					VECHECKPOINTER(getSceneManagerPointer()->loadModel("The Cube" + std::to_string(++cubeid), "media/models/test/crate0", "cube.obj", 0, eParent));
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

	//
	//Adjust the camera to look at the player
	//
	class EventListenerCameraMovement : public VEEventListener {

	protected:
		virtual void onFrameStarted(veEvent event) {
			glm::vec3 positionPlayer = getSceneManagerPointer()->getSceneNode("The Player Parent")->getPosition();
			ve::VESceneNode* pCamera = getSceneManagerPointer()->getCamera()->getParent();

			pCamera->lookAt(
				glm::vec3(positionPlayer.x, positionPlayer.y + 3, positionPlayer.z - 10),
				glm::vec3(positionPlayer.x, positionPlayer.y + 20, positionPlayer.z + 100),
				glm::vec3(0, 1, 0));
		};


	public:
		///Constructor of class EventListenerCameraMocement
		EventListenerCameraMovement(std::string name) : VEEventListener(name) { };

		///Destructor of class EventListenerCameraMocement
		virtual ~EventListenerCameraMovement() {};
	};


	///user defined manager class, derived from VEEngine
	class MyVulkanEngine : public VEEngine {
	public:

		MyVulkanEngine(bool debug = false) : VEEngine(debug) {};
		~MyVulkanEngine() {};


		///Register an event listener to interact with the user

		virtual void registerEventListeners() {
			VEEngine::registerEventListeners();

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

			for (int i = 0; i < 100; i++) {
				VESceneNode* e1, * e1Parent;
				float x = rand() % 500 + 500;
				float y = rand() % 20;
				float z = rand() % 500 + 500;
				e1Parent = getSceneManagerPointer()->createSceneNode("The Cube" + std::to_string(i) + " Parent", pScene, glm::mat4(1.0));
				VECHECKPOINTER(e1 = getSceneManagerPointer()->loadModel("The Cube" + std::to_string(i), "media/models/test/crate0", "cube.obj"));
				e1Parent->multiplyTransform(glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)));
				e1Parent->addChild(e1);
			}

			VESceneNode* e2, * e2Parent;
			e2Parent = getSceneManagerPointer()->createSceneNode("The Player Parent", pScene, glm::mat4(1.0));
			VECHECKPOINTER(e2 = getSceneManagerPointer()->loadModel("The PLayer0", "media/models/test/crate1", "cube.obj"));
			e2Parent->multiplyTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 5.0f)));
			e2Parent->addChild(e2);

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

