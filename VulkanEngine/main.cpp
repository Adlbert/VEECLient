/**
* The Vienna Vulkan Engine
*
* (c) bei Helmut Hlavacs, University of Vienna
*
*/


#include "VEInclude.h"
#include "glm/ext.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>



extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/common.h"
#include "libavutil/mathematics.h"
}



namespace ve {


	uint32_t g_score = 0;				//derzeitiger Punktestand
	double g_time = 5.0;				//zeit die noch übrig ist
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

	//
	//Adjust the camera to look at the player
	//
	class EventListenerEncodeFrame : public VEEventListenerGLFW {

	private:

		const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
		AVCodecContext* c = avcodec_alloc_context3(codec);
		const char* filename = "D:\\Projects\\ViennaVulkanEngine\\delta.mp4";
		FILE* f;

		static void encode(AVCodecContext* enc_ctx, AVFrame* frame, AVPacket* pkt, FILE* outfile) {
			int ret;

			// send the frame to the encoder */
			ret = avcodec_send_frame(enc_ctx, frame);
			if (ret < 0) {
				fprintf(stderr, "error sending a frame for encoding\n");
				exit(1);
			}

			while (ret >= 0) {
				int ret = avcodec_receive_packet(enc_ctx, pkt);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
					return;
				else if (ret < 0) {
					fprintf(stderr, "error during encoding\n");
					exit(1);
				}

				printf("encoded frame %lld (size=%5d)\n", pkt->pts, pkt->size);
				fwrite(pkt->data, 1, pkt->size, outfile);
				av_packet_unref(pkt);
			}
		}
	protected:
		virtual void onFrameEnded(veEvent event) {
			//Method from VEEventListenerGLFW
			VkExtent2D extent = getWindowPointer()->getExtent();
			uint32_t imageSize = extent.width * extent.height * 4;
			VkImage image = getRendererPointer()->getSwapChainImage();

			uint8_t* dataImage = new uint8_t[imageSize];

			vh::vhBufCopySwapChainImageToHost(getRendererPointer()->getDevice(),
				getRendererPointer()->getVmaAllocator(),
				getRendererPointer()->getGraphicsQueue(),
				getRendererPointer()->getCommandPool(),
				image, VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				dataImage, extent.width, extent.height, imageSize);

			m_numScreenshot++;

			//std::string name("media/screenshots/screenshot" + std::to_string(m_numScreenshot - 1) + ".jpg");
			//stbi_write_jpg(name.c_str(), extent.width, extent.height, 4, dataImage, 4 * extent.width);
			//
			uint8_t endcode[] = { 0, 0, 1, 0xb7 };

			AVFrame* picture = av_frame_alloc();

			AVPacket* pkt = av_packet_alloc();
			if (!pkt) {
				fprintf(stderr, "Cannot alloc packet\n");
				exit(1);
			}

			picture->format = c->pix_fmt;
			picture->width = c->width;
			picture->height = c->height;

			int ret = av_frame_get_buffer(picture, 32);
			if (ret < 0) {
				fprintf(stderr, "could not alloc the frame data\n");
				exit(1);
			}

			// encode 1 frame of video
			for (int i = 0; i < 1; i++) {
				fflush(stdout);

				// make sure the frame data is writable
				ret = av_frame_make_writable(picture);
				if (ret < 0) {
					fprintf(stderr, "Cannot make frame writeable\n");
					exit(1);
				}

				// prepare image
				// Y
				for (int y = 0; y < c->height; y++) {
					for (int x = 0; x < c->width; x++) {
						picture->data[0][y * picture->linesize[0] + x] = dataImage[i * 3];
					}
				}

				// Cb and Cr
				for (int y = 0; y < c->height / 2; y++) {
					for (int x = 0; x < c->width / 2; x++) {
						picture->data[1][y * picture->linesize[1] + x] = dataImage[i * 3 + 1];
						picture->data[2][y * picture->linesize[2] + x] = dataImage[i * 3 + 2];
					}
				}

				picture->pts = m_numScreenshot;

				// encode the image
				encode(c, picture, pkt, f);
			}

			if (g_gameLost || g_gameWon) {
				// flush the encoder
				encode(c, NULL, pkt, f);

				// add sequence end code to have a real MPEG file
				fwrite(endcode, 1, sizeof(endcode), f);
				fclose(f);

				avcodec_free_context(&c);
				av_frame_free(&picture);
				av_packet_free(&pkt);
				exit(0);
			}
		};


	public:
		///Constructor of class EventListenerCameraMocement
		EventListenerEncodeFrame(std::string name) : VEEventListenerGLFW(name) {
			c->bit_rate = 400000;

			// resolution must be a multiple of two
			//c->width = extent.width;
			//c->height = extent.height;
			c->width = 352;
			c->height = 288;
			// frames per second
			c->time_base.num = 1;
			c->time_base.den = 25;
			c->framerate.num = 25;
			c->framerate.den = 1;

			c->gop_size = 10; // emit one intra frame every ten frames
			c->max_b_frames = 1;
			c->pix_fmt = AV_PIX_FMT_YUV420P;

			f = fopen(filename, "wb");
			if (!f) {
				fprintf(stderr, "could not open %s\n", filename);
				exit(1);
			}

			// open it
			if (avcodec_open2(c, codec, NULL) < 0) {
				fprintf(stderr, "could not open codec\n");
				exit(1);
			}
		};

		///Destructor of class EventListenerCameraMocement
		virtual ~EventListenerEncodeFrame() {};
	};


	///user defined manager class, derived from VEEngine
	class MyVulkanEngine : public VEEngine {
	public:

		MyVulkanEngine(bool debug = false) : VEEngine(debug) {};
		~MyVulkanEngine() {};


		///Register an event listener to interact with the user

		virtual void registerEventListeners() {
			registerEventListener(new EventListenerKeyboardBindings("KeyboardBindings"));
			registerEventListener(new EventListenerEncodeFrame("EncodeFrame"), { veEvent::VE_EVENT_FRAME_ENDED });
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

#define INBUF_SIZE 4096

static void encode(AVCodecContext* enc_ctx, AVFrame* frame, AVPacket* pkt, FILE* outfile) {
	int ret;

	// send the frame to the encoder */
	ret = avcodec_send_frame(enc_ctx, frame);
	if (ret < 0) {
		fprintf(stderr, "error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0) {
		int ret = avcodec_receive_packet(enc_ctx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			fprintf(stderr, "error during encoding\n");
			exit(1);
		}

		printf("encoded frame %lld (size=%5d)\n", pkt->pts, pkt->size);
		fwrite(pkt->data, 1, pkt->size, outfile);
		av_packet_unref(pkt);
	}
}

static void pgm_save(unsigned char* buf, int wrap, int xsize, int ysize, char* filename) {
	FILE* f = fopen(filename, "w");
	fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
	for (int i = 0; i < ysize; i++)
		fwrite(buf + i * wrap, 1, xsize, f);
	fclose(f);
}

static void decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt, const char* filename) {
	char buf[1024];

	int ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0) {
		fprintf(stderr, "Error sending a packet for decoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			fprintf(stderr, "Error during decoding\n");
			exit(1);
		}

		printf("saving frame %3d\n", dec_ctx->frame_number);
		fflush(stdout);

		// the picture is allocated by the decoder. no need to
		//      free it
		snprintf(buf, sizeof(buf), filename, dec_ctx->frame_number);
		pgm_save(frame->data[0], frame->linesize[0],
			frame->width, frame->height, buf);
	}
}

static int testEncode(const char* filename, const AVCodec* codec) {
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };

	avcodec_register_all();

	AVCodecContext* c = avcodec_alloc_context3(codec);
	AVFrame* picture = av_frame_alloc();

	AVPacket* pkt = av_packet_alloc();
	if (!pkt) {
		fprintf(stderr, "Cannot alloc packet\n");
		exit(1);
	}

	c->bit_rate = 400000;

	// resolution must be a multiple of two
	c->width = 352;
	c->height = 288;
	// frames per second
	c->time_base.num = 1;
	c->time_base.den = 25;
	c->framerate.num = 25;
	c->framerate.den = 1;

	c->gop_size = 10; // emit one intra frame every ten frames
	c->max_b_frames = 1;
	c->pix_fmt = AV_PIX_FMT_YUV420P;

	// open it
	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "could not open codec\n");
		exit(1);
	}

	FILE* f = fopen(filename, "wb");
	if (!f) {
		fprintf(stderr, "could not open %s\n", filename);
		exit(1);
	}

	picture->format = c->pix_fmt;
	picture->width = c->width;
	picture->height = c->height;

	int ret = av_frame_get_buffer(picture, 32);
	if (ret < 0) {
		fprintf(stderr, "could not alloc the frame data\n");
		exit(1);
	}

	// encode 1 second of video
	for (int i = 0; i < 25; i++) {
		fflush(stdout);

		// make sure the frame data is writable
		ret = av_frame_make_writable(picture);
		if (ret < 0) {
			fprintf(stderr, "Cannot make frame writeable\n");
			exit(1);
		}

		// prepare a dummy image
		// Y
		for (int y = 0; y < c->height; y++) {
			for (int x = 0; x < c->width; x++) {
				picture->data[0][y * picture->linesize[0] + x] = x + y + i * 3;
			}
		}

		// Cb and Cr
		for (int y = 0; y < c->height / 2; y++) {
			for (int x = 0; x < c->width / 2; x++) {
				picture->data[1][y * picture->linesize[1] + x] = 128 + y + i * 2;
				picture->data[2][y * picture->linesize[2] + x] = 64 + x + i * 5;
			}
		}

		picture->pts = i;

		// encode the image
		encode(c, picture, pkt, f);
	}

	// flush the encoder
	encode(c, NULL, pkt, f);

	// add sequence end code to have a real MPEG file
	fwrite(endcode, 1, sizeof(endcode), f);
	fclose(f);

	avcodec_free_context(&c);
	av_frame_free(&picture);
	av_packet_free(&pkt);
	return 0;
}

static int testDecode(const char* filename, const char* outfilename, const AVCodec* codec) {
	char outname[200];
	uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	uint8_t* data;

	AVPacket* pkt = av_packet_alloc();
	if (!pkt)
		exit(1);

	// set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams)
	memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

	if (!codec) {
		fprintf(stderr, "codec not found\n");
		exit(1);
	}

	AVCodecParserContext* parser = av_parser_init(codec->id);
	if (!parser) {
		fprintf(stderr, "parser not found\n");
		exit(1);
	}

	AVCodecContext* c = avcodec_alloc_context3(codec);
	AVFrame* picture = av_frame_alloc();

	// For some codecs, such as msmpeg4 and mpeg4, width and height
	//    MUST be initialized there because this information is not
	//    available in the bitstream.

	 // open it
	if (avcodec_open2(c, codec, NULL) < 0) {
		fprintf(stderr, "could not open codec\n");
		exit(1);
	}

	FILE* f = fopen(filename, "rb");
	if (!f) {
		fprintf(stderr, "could not open %s\n", filename);
		exit(1);
	}

	int num_frame = 0;
	while (!feof(f)) {
		// read raw data from the input file
		size_t data_size = fread(inbuf, 1, INBUF_SIZE, f);
		if (!data_size)
			break;

		// use the parser to split the data into frames
		uint8_t* data = inbuf;
		while (data_size > 0) {
			int ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size,
				data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			if (ret < 0) {
				fprintf(stderr, "Error while parsing\n");
				exit(1);
			}
			data += ret;
			data_size -= ret;

			if (pkt->size) {
				sprintf(outname, "%s%d.pgm", outfilename, num_frame++);
				printf("Outname %s\n", outname);
				decode(c, picture, pkt, outname);
			}
		}
	}

	// flush the decoder
	sprintf(outname, "%s%d.pgm", outfilename, num_frame++);
	printf("Outname %s\n", outname);
	decode(c, picture, NULL, outname);

	fclose(f);

	av_parser_close(parser);
	avcodec_free_context(&c);
	av_frame_free(&picture);
	av_packet_free(&pkt);

	return 0;
}

int static metadata(const char* filename) {
	AVFormatContext* fmt_ctx = NULL;
	AVDictionaryEntry* tag = NULL;
	int ret;

	if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)))
		return ret;

	if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
		return ret;
	}

	while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
		printf("%s=%s\n", tag->key, tag->value);

	avformat_close_input(&fmt_ctx);

	return 0;
}

int main() {

	bool debug = true;

	MyVulkanEngine mve(debug);	//enable or disable debugging (=callback, validation layers)


	mve.initEngine();
	mve.loadLevel(1);
	mve.run();

	//const char* filename = "D:\\Projects\\ViennaVulkanEngine\\delta.mpg";
	////const char* outfilename = "D:\\Projects\\ViennaVulkanEngine\\out\\out";

	//const AVCodec* encode_codec = avcodec_find_encoder(AV_CODEC_ID_MPEG1VIDEO);
	//const AVCodec* decode_codec = avcodec_find_encoder(AV_CODEC_ID_MPEG1VIDEO);

	////metadata(filename);
	//testEncode(filename, encode_codec);
	////testDecode(filename, outfilename, decode_codec);

	return 0;
}


