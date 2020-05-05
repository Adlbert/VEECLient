/**
* The Vienna Vulkan Engine
*
* (c) bei Helmut Hlavacs, University of Vienna
*
*/
#define _WINSOCKAPI_ 

#include "VEInclude.h"
#include "glm/ext.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <WS2tcpip.h>
#include <thread> 



extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libswscale/swscale.h>
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/common.h"
#include "libavutil/mathematics.h"
}


#pragma comment(lib, "Ws2_32.lib")

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
	// Encode:
	//	  https://github.com/codefromabove/FFmpegRGBAToYUV/blob/master/FFmpegRGBAToYUV/ConvertRGBA.cpp
	//
	class EventListenerEncodeFrame : public VEEventListenerGLFW {

	private:
		bool encodeframe = true;
		float* m_AvgFrameTime;
		int frameCount = 0;
		const char* filename = "D:\\Projects\\ViennaVulkanEngine\\MPG1Video_highbitrate.mpg";
		AVCodec* codec;
		AVCodecContext* c = NULL;
		int i, ret, x, y, got_output, interval;
		FILE* f;
		AVFrame* frame;
		AVPacket pkt;
		SwsContext* ctx;
		uint8_t* dataImage;
		VkExtent2D extent;
		uint32_t imageSize;

		// A callable object 
		class frame_thread {
		private:
			int fragNum;
			int maxBuffersize = 1400;
			uint8_t* sendBuffer;

			char* getBuffer(uint8_t* dataImage, int frameCount) {
				char buff[sizeof(dataImage)];
				for (int i = 0; i < sizeof(dataImage); i++) {
					buff[i] = dataImage[i];
				}
				return buff;
			}

			void sendFragment(char* buff) {
				//https://bitbucket.org/sloankelly/youtube-source-repository/src/bb84cf7f8d95d37354cf7dd0f0a57e48f393bd4b/cpp/networking/UDPClientServerBasic/?at=master
				////////////////////////////////////////////////////////////
				// INITIALIZE WINSOCK
				////////////////////////////////////////////////////////////

				// Structure to store the WinSock version. This is filled in
				// on the call to WSAStartup()
				WSADATA data;

				// To start WinSock, the required version must be passed to
				// WSAStartup(). This server is going to use WinSock version
				// 2 so I create a word that will store 2 and 2 in hex i.e.
				// 0x0202
				WORD version = MAKEWORD(2, 2);

				// Start WinSock
				int wsOk = WSAStartup(version, &data);
				if (wsOk != 0)
				{
					// Not ok! Get out quickly
					std::cout << "Can't start Winsock! " << wsOk;
					return;
				}

				////////////////////////////////////////////////////////////
				// CONNECT TO THE SERVER
				////////////////////////////////////////////////////////////

				// Create a hint structure for the server
				sockaddr_in server;
				server.sin_family = AF_INET; // AF_INET = IPv4 addresses
				server.sin_port = htons(54000); // Little to big endian conversion
				inet_pton(AF_INET, "127.0.0.1", &server.sin_addr); // Convert from string to byte array

				// Socket creation, note that the socket type is datagram
				SOCKET out = socket(AF_INET, SOCK_DGRAM, 0);

				// Write out to that socket
				std::string s("Test");
				//int sendOk = sendto(out, buff, strlen(buff), 0, (sockaddr*)&server, sizeof(server));
				int sendOk = sendto(out, s.c_str(), s.size() + 1, 0, (sockaddr*)&server, sizeof(server));


				if (sendOk == SOCKET_ERROR)
				{
					std::cout << "That didn't work! " << WSAGetLastError() << std::endl;
				}

				// Close the socket
				closesocket(out);

				// Close down Winsock
				WSACleanup();
			}


			void sendFrame(uint8_t* pkg, int pkgsize, int frameCount) {
				char* buf = reinterpret_cast<char*>(pkg);
				int currentBuffersize = 0;
				for (int i = 0; i < pkgsize; i++) {
					if (currentBuffersize < maxBuffersize) {
						sendBuffer[currentBuffersize] = pkg[i];
						currentBuffersize++;
					}
					else {
						sendFragment(reinterpret_cast<char*>(sendBuffer));
						currentBuffersize = 0;
					}
				}
			}

		public:
			void operator()(uint8_t* pkg, int pkgsize, int frameCount) {
				sendBuffer = new uint8_t[maxBuffersize];
				this->maxBuffersize = maxBuffersize;
				fragNum = 0;
				sendFrame(pkg, pkgsize, frameCount);
			}
		};

	protected:
		virtual void onFrameEnded(veEvent event) {

			interval++;
			//only encode every 4th frame
			if (interval % 2 != 0) {
				return;
			}
			////Compute fps
			//float fps = 1 / *m_AvgFrameTime;
			//only every 4th frame is encoded
			//c->time_base.den = fps;
			////std::cout << c->time_base.den << std::endl;
			////std::cout << std::endl;

			//Method from VEEventListenerGLFW
			VkImage image = getRendererPointer()->getSwapChainImage();
			dataImage = new uint8_t[imageSize];
			vh::vhBufCopySwapChainImageToHost(getRendererPointer()->getDevice(),
				getRendererPointer()->getVmaAllocator(),
				getRendererPointer()->getGraphicsQueue(),
				getRendererPointer()->getCommandPool(),
				image, VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				dataImage, extent.width, extent.height, imageSize);
			//
			frameCount++;

			/* the image can be allocated by any means and av_image_alloc() is
			* just the most convenient way if av_malloc() is to be used */
			ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,
				c->pix_fmt, 32);
			if (ret < 0) {
				fprintf(stderr, "Could not allocate raw picture buffer\n");
				exit(6);
			}

			/* encode 1 frame of video */
			av_init_packet(&pkt);
			pkt.data = NULL;    // packet data will be allocated by the encoder
			pkt.size = 0;

			fflush(stdout);
			uint8_t* inData[1] = { dataImage }; // RGBA32 have one plane
			//
			// NOTE: In a more general setting, the rows of your input image may
			//       be padded; that is, the bytes per row may not be 4 * width.
			//       In such cases, inLineSize should be set to that padded width.
			//
			int inLinesize[1] = { 4 * c->width }; // RGBA stride
			sws_scale(ctx, inData, inLinesize, 0, c->height, frame->data, frame->linesize);

			frame->pts = frameCount;

			/* encode the image */
			ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
			if (ret < 0) {
				fprintf(stderr, "Error encoding frame\n");
				exit(7);
			}

			if (got_output) {
				//printf("Write frame %3d (size=%5d)\n", frameCount, pkt.size);
				//if (addToSendBuffer(pkt.data, pkt.size)) {
				//}
				std::thread th2(frame_thread(), pkt.data, pkt.size, frameCount);
				th2.detach();
				fwrite(pkt.data, 1, pkt.size, f);
				av_free_packet(&pkt);
			}



			if (false && (g_gameLost || g_gameWon)) {

				/* get the delayed frames */
				for (got_output = 1; got_output; frameCount++) {
					fflush(stdout);

					ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
					if (ret < 0) {
						fprintf(stderr, "Error encoding frame\n");
						exit(8);
					}

					if (got_output) {
						//printf("Write frame %3d (size=%5d)\n", frameCount, pkt.size);
						fwrite(pkt.data, 1, pkt.size, f);
						av_free_packet(&pkt);
					}
				}

				/* add sequence end code to have a real mpeg file */
				printf("Encode video file %s\n", filename);
				uint8_t endcode[] = { 0, 0, 1, 0xb7 };
				fwrite(endcode, 1, sizeof(endcode), f);
				fclose(f);

				avcodec_close(c);
				av_free(c);
				av_freep(&frame->data[0]);
				av_frame_free(&frame);
				exit(0);
			}
		}


	public:
		///Constructor of class EventListenerCameraMocement
		EventListenerEncodeFrame(std::string name, float* avgFrameTime) : VEEventListenerGLFW(name) {

			dataImage = new uint8_t[0];
			got_output = i = interval = ret = x = y = 0;
			pkt = AVPacket();

			m_AvgFrameTime = avgFrameTime;
			extent = getWindowPointer()->getExtent();
			imageSize = extent.width * extent.height * 4;

			/* find the mpeg1 video encoder */
			codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
			if (!codec) {
				fprintf(stderr, "Codec not found\n");
				exit(1);
			}

			c = avcodec_alloc_context3(codec);
			if (!c) {
				fprintf(stderr, "Could not allocate video codec context\n");
				exit(2);
			}

			/* put sample parameters */
			//resolution must be a multiple of two 
			c->width = extent.width;
			c->height = extent.height;
			//
			//Set pixel format
			c->pix_fmt = AVPixelFormat::AV_PIX_FMT_YUV420P;
			/* frames per second */
			c->time_base = { 1, 25 };
			/* emit one intra frame every ten frames
			 * check frame pict_type before passing frame
			 * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
			 * then gop_size is ignored and the output of encoder
			 * will always be I frame irrespective to gop_size
			 */
			c->gop_size = 8;
			c->max_b_frames = 2;

			//motion estimation comparison function
			c->me_cmp = 1;
			//maximum motion estimation search range in subpel units If 0 then no limit.
			c->me_range = 0;
			//subpel ME quality
			c->me_subpel_quality = 5;

			//minimum quantizer
			c->qmin = 10;
			//maximum quantizer
			c->qmax = 51;
			//maximum quantizer difference between frames
			c->max_qdiff = 2;
			//amount of qscale change between easy & hard scenes (0.0-1.0)
			c->qcompress = 0.8;
			//qscale factor between P- and I-frames If > 0 then the last P-frame quantizer will be used (q = lastp_q * factor + offset).
			c->i_quant_factor = 0.71;

			//LowBitrate
			if (false) {
				//the average bitrate
				c->bit_rate = 1024 * 256; // 256kb
				//number of bits the bitstream is allowed to diverge from the reference.
				c->bit_rate_tolerance = 1024 * 64; //64kb
				//maximum bitrate
				c->rc_max_rate = 1024 * 512;//512kb
				//decoder bitstream buffer size
				c->rc_buffer_size = 1024 * 1000;//1Mb
			}
			//MidBitrate
			if (false) {
				//the average bitrate
				c->bit_rate = 1024 * 1000 * 2;//2Mb
				//number of bits the bitstream is allowed to diverge from the reference.
				c->bit_rate_tolerance = 1024 * 1000 * 1;//1Mb
				//maximum bitrate
				c->rc_max_rate = 1024 * 1000 * 3;//3Mb
				//decoder bitstream buffer size
				c->rc_buffer_size = 1024 * 1000 * 8;//8Mb
			}
			//HighBitrate
			if (true) {
				//the average bitrate
				c->bit_rate = 1024 * 1000 * 20;//20Mb
				//number of bits the bitstream is allowed to diverge from the reference.
				c->bit_rate_tolerance = 1024 * 1000 * 4;//4Mb
				//maximum bitrate
				c->rc_max_rate = 1024 * 1000 * 24;//24Mb
				//decoder bitstream buffer size
				c->rc_buffer_size = 1024 * 1000 * 40;//40Mb
			}

			c->flags |= AV_CODEC_FLAG_LOOP_FILTER;
			c->flags2 |= AV_CODEC_FLAG2_FAST;

			/* open it */
			if (avcodec_open2(c, codec, NULL) < 0) {
				fprintf(stderr, "Could not open codec\n");
				exit(3);
			}

			f = fopen(filename, "wb");
			if (!f) {
				fprintf(stderr, "Could not open %s\n", filename);
				exit(4);
			}

			frame = av_frame_alloc();
			if (!frame) {
				fprintf(stderr, "Could not allocate video frame\n");
				exit(5);
			}
			frame->format = c->pix_fmt;
			frame->width = c->width;
			frame->height = c->height;

			//Use Bicubic interpolation
			ctx = sws_getContext(extent.width, extent.height,
				AV_PIX_FMT_RGBA, c->width, c->height,
				c->pix_fmt, SWS_BICUBIC, 0, 0, 0);
		};

		///Destructor of class EventListenerCameraMocement
		virtual ~EventListenerEncodeFrame() {};
	};


	///user defined manager class, derived from VEEngine
	class MyVulkanEngine : public VEEngine {
	private:
		float* avgFrameTime;

	public:

		MyVulkanEngine(bool debug = false) : VEEngine(debug) {
			avgFrameTime = &m_AvgFrameTime;
		};
		~MyVulkanEngine() {};


		///Register an event listener to interact with the user

		virtual void registerEventListeners() {
			registerEventListener(new EventListenerKeyboardBindings("KeyboardBindings"));
			registerEventListener(new EventListenerEncodeFrame("EncodeFrame", avgFrameTime), { veEvent::VE_EVENT_FRAME_ENDED });
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


