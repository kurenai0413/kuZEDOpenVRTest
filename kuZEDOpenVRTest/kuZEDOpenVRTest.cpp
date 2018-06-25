#include <opencv2/opencv.hpp>

// OpenGL includes
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#include <sl_zed/Camera.hpp>

#include <OpenVR.h>

#include "kuShaderHandler.h"

#define numEyes			2

#define Left			0
#define Right			1

#define ZEDImgWidth		1280
#define ZEDImgHeight	720

#define	nearClip		1.0
#define farClip			5000.0

vr::IVRSystem	*	kuOpenVRInit(uint32_t &hmdWidth, uint32_t &hmdHeight);
GLFWwindow		*	kuOpenGLInit(int width, int height, const std::string& title, GLFWkeyfun cbfun);
sl::ERROR_CODE		kuZEDInit(sl::Camera &zedCam, sl::InitParameters initParams, sl::RuntimeParameters rtParams, sl::Mat &imgZEDLeft, sl::Mat &imgZEDRight, cv::Mat &imgCVLeft, cv::Mat &imgCVRight);

void				SetIntrinsicParams(sl::Camera &zedCam, cv::Mat &intrinsicParamsLeft, cv::Mat &intrinsicParamsRight, cv::Mat  &distParamsLeft, cv::Mat &distParamsRight, GLfloat intrinsicMatGL[2][16]);
void				SetIntrinsicMatCV(sl::CameraParameters camCalibParams, cv::Mat &intrinsicParams, cv::Mat &distPrams);
void				IntrinsicCVtoGL(cv::Mat IntParam, GLfloat GLProjection[16]);
void				ExtrinsicCVtoGL(cv::Mat RotMat, cv::Mat TransVec, GLfloat GLModelView[16]);

void				DrawBGImage(cv::Mat BGImg, kuShaderHandler BGShader);
GLuint				CreateTexturebyImage(cv::Mat Img);

std::string			getHMDString(vr::IVRSystem * pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError * peError = nullptr);
cv::Mat				MatSL2CV(sl::Mat& input);

void				key_callback(GLFWwindow * window, int key, int scancode, int action, int mode);

#pragma region // Camera parameters OpenGL //
GLfloat						IntrinsicProjMatGL[2][16];
GLfloat						ExtrinsicViewMat[2][16];
#pragma endregion

kuShaderHandler				Tex2DShaderHandler;

void main()
{
	uint32_t frameBufferWidth, frameBufferHeight;

	vr::IVRSystem	*	hmd	   = kuOpenVRInit(frameBufferWidth, frameBufferHeight);
	const int windowHeight = 720;
	const int windowWidth = (frameBufferWidth * windowHeight) / frameBufferHeight;
	GLFWwindow		*	window = kuOpenGLInit(windowWidth, windowHeight, "kuOpenGLVRTest", key_callback);
	Tex2DShaderHandler.Load("BGImgVertexShader.vert", "BGImgFragmentShader.frag");

	GLuint	FrameBufferID[2];
	GLuint	SceneTextureID[2];

	glGenFramebuffers(numEyes, FrameBufferID);							// Create frame buffers for each eyes.
	glGenTextures(numEyes, SceneTextureID);								// Prepare texture memory space and give it an index. In this case, glGenTextures creates two textures for colorRenderTarget

	for (int eye = 0; eye < numEyes; eye++)
	{
		glBindTexture(GL_TEXTURE_2D, SceneTextureID[eye]);				//Bind which texture if active for processing
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, frameBufferWidth, frameBufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		
		glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID[eye]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SceneTextureID[eye], 0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	#pragma region // Camera parameters OpenCV //
	cv::Mat						IntrinsicMat[2];
	cv::Mat						DistParam[2];
	cv::Mat						RotationVec[2];
	cv::Mat						RotationMat[2];
	cv::Mat						TranslationVec[2];
	#pragma endregion

	sl::Camera					ZEDCam;														// ZED camera object
	sl::InitParameters			initParams;
	sl::CalibrationParameters	calibParams;
	sl::RuntimeParameters		rtParams;

	// Camera frames
	cv::Mat						camFrameCVRGBA[2];
	cv::Mat						camFrameCVBGR[2];
	sl::Mat						camFrameZED[2];

	sl::ERROR_CODE res = kuZEDInit(ZEDCam,
								   initParams, rtParams,									// Camera parameters for setting
								   camFrameZED[0], camFrameZED[1],							// Image pointers
								   camFrameCVRGBA[0], camFrameCVRGBA[1]);					// Image pointers
	if (res == sl::ERROR_CODE::SUCCESS)
	{
		std::cout << "ZED initialized." << std::endl;
	}
	SetIntrinsicParams(ZEDCam, IntrinsicMat[0], IntrinsicMat[1], DistParam[0], DistParam[1], IntrinsicProjMatGL);

	double deltaT, lastFrameT = 0.0f;

	GLuint leftFramebuffer = 0;
	glGenFramebuffers(1, &leftFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, leftFramebuffer);

	// ID of the texture where to render to
	GLuint renderedTexture;
	glGenTextures(1, &renderedTexture);

	glBindTexture(GL_TEXTURE_2D, renderedTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ZEDImgWidth, ZEDImgHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	// Poor filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);

	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);

#pragma region // Set texture quad //
	static const GLfloat leftQuadVertices[] = {
		0.709f,  0.334f, 1.0f, 0.0f,				// <= 這邊是比例，UV是相對於視窗的座標，不是絕對座標
		0.709f, -0.334f, 1.0f, 1.0f,
		-0.59f, -0.334f, 0.0f, 1.0f,
		-0.59f,  0.334f, 0.0f, 0.0f
	};

	GLuint indices[] = { 0, 1, 3,
		1, 2, 3 };

	GLuint quadVertexArray = 0;
	GLuint quadVertexBuffer = 0;
	GLuint quadElementBuffer = 0;

	glGenVertexArrays(1, &quadVertexArray);
	glBindVertexArray(quadVertexArray);
	glGenBuffers(1, &quadVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(leftQuadVertices), leftQuadVertices, GL_STATIC_DRAW);
	glGenBuffers(1, &quadElementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Assign vertex position data
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Assign texture coordinates
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#pragma endregion

	while (!glfwWindowShouldClose(window))
	{
		double currFrameT = glfwGetTime();
		deltaT = currFrameT - lastFrameT;
		lastFrameT = currFrameT;

		std::cout << "FPS: " << 1/deltaT << std::endl;

		vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
		vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);			// Can be replaced by GetDeviceToAbsoluteTrackingPose(?)

		#pragma region // Render camera frame to texture //
		//DrawBGImage(camFrameCVBGR[0], Tex2DShaderHandler);
		glBindFramebuffer(GL_FRAMEBUFFER, leftFramebuffer);
		glViewport(0, 0, ZEDImgWidth, ZEDImgHeight);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		ZEDCam.grab(rtParams);
		ZEDCam.retrieveImage(camFrameZED[0], sl::VIEW_LEFT, sl::MEM_CPU);
		cv::cvtColor(camFrameCVRGBA[0], camFrameCVBGR[0], CV_RGBA2BGR);
		cv::flip(camFrameCVBGR[0], camFrameCVBGR[0], 0);
		//cv::imshow("Test", camFrameCVBGR[0]);
		DrawBGImage(camFrameCVBGR[0], Tex2DShaderHandler);

		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		#pragma endregion

		for (int eye = 0; eye < numEyes; ++eye)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID[eye]);
			glViewport(0, 0, frameBufferWidth, frameBufferHeight);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//glEnable(GL_DEPTH_TEST);

			Tex2DShaderHandler.Use();

			glBindTexture(GL_TEXTURE_2D, renderedTexture);
			glBindVertexArray(quadVertexArray);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		vr::Texture_t LTexture = { reinterpret_cast<void*>(intptr_t(SceneTextureID[Left])), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::EVREye(Left), &LTexture);
		vr::Texture_t RTesture = { reinterpret_cast<void*>(intptr_t(SceneTextureID[Right])), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::EVREye(Right), &RTesture);

		vr::VRCompositor()->PostPresentHandoff();

		// Mirror to GLFW window
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_NONE);
		glViewport(0, 0, 640, 720);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBlitFramebuffer(0, 0, frameBufferWidth, frameBufferHeight, 0, 0, 640, 720, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, GL_NONE);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}


vr::IVRSystem * kuOpenVRInit(uint32_t & hmdWidth, uint32_t & hmdHeight)
{
	vr::EVRInitError eError = vr::VRInitError_None;
	vr::IVRSystem* hmd = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None) {
		fprintf(stderr, "OpenVR Initialization Error: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		return nullptr;
	}

	const std::string& driver = getHMDString(hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);		// Graphic card name
	const std::string& model  = getHMDString(hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ModelNumber_String);				// HMD device name
	const std::string& serial = getHMDString(hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);			// HMD device serial
	const float freq = hmd->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);	// HMD FPS
																													//get the proper resolution of the hmd
	hmd->GetRecommendedRenderTargetSize(&hmdWidth, &hmdHeight);

	fprintf(stderr, "HMD: %s '%s' #%s (%d x %d @ %g Hz)\n", driver.c_str(), model.c_str(), serial.c_str(), hmdWidth, hmdHeight, freq);

	// Initialize the compositor
	vr::IVRCompositor* compositor = vr::VRCompositor();
	if (!compositor) {
		fprintf(stderr, "OpenVR Compositor initialization failed. See log file for details\n");
		vr::VR_Shutdown();
		assert("VR failed" && false);
	}

	return hmd;
}

GLFWwindow* kuOpenGLInit(int width, int height, const std::string& title, GLFWkeyfun cbfun)
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW\n");
		::exit(1);
	}

	// Without these, shaders actually won't initialize properly
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

#   ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#   endif

	GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	if (!window)
	{
		fprintf(stderr, "ERROR: could not open window with GLFW\n");
		glfwTerminate();
		::exit(2);
	}
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, cbfun);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	//glfwSetCursorPosCallback(window, mouse_callback);				// 顧名思義...大概只有位置資訊而沒有button事件資訊吧

																	// Start GLEW extension handler, with improved support for new features
	glewExperimental = GL_TRUE;
	glewInit();

	// Clear startup errors
	while (glGetError() != GL_NONE) {}

#   ifdef _DEBUG
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glEnable(GL_DEBUG_OUTPUT);
#   endif

	// Negative numbers allow buffer swaps even if they are after the vertical retrace,
	// but that causes stuttering in VR mode
	glfwSwapInterval(0);

	fprintf(stderr, "GPU: %s (OpenGL version %s)\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

	// Bind a single global vertex array (done this way since OpenGL 3)
	/*GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);*/

	// Check for errors
	const GLenum error = glGetError();
	assert(error == GL_NONE);

	return window;
}

sl::ERROR_CODE kuZEDInit(sl::Camera & zedCam, sl::InitParameters initParams, sl::RuntimeParameters rtParams, sl::Mat & imgZEDLeft, sl::Mat & imgZEDRight, cv::Mat & imgCVLeft, cv::Mat & imgCVRight)
{
	// Initialize ZED camera initial parameters
	initParams.camera_resolution = sl::RESOLUTION_HD720;
	initParams.depth_mode = sl::DEPTH_MODE_QUALITY;
	initParams.coordinate_units = sl::UNIT_MILLIMETER;
	initParams.camera_fps = 60;

	// Set ZED camera runtime parameters
	rtParams.sensing_mode = sl::SENSING_MODE_FILL;

	// Allocate memory space for ZED image
	imgZEDLeft.alloc(ZEDImgWidth, ZEDImgHeight, sl::MAT_TYPE_8U_C4, sl::MEM_CPU);
	imgZEDRight.alloc(ZEDImgWidth, ZEDImgHeight, sl::MAT_TYPE_8U_C4, sl::MEM_CPU);

	// Set image pointer to OpenCV image
	imgCVLeft = MatSL2CV(imgZEDLeft);
	imgCVRight = MatSL2CV(imgZEDRight);

	sl::ERROR_CODE eCode = zedCam.open(initParams);

	return eCode;
}

std::string getHMDString(vr::IVRSystem * pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError * peError)
{
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, nullptr, 0, peError);
	if (unRequiredBufferLen == 0)
	{
		return "";
	}

	char* pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;

	return sResult;
}

void key_callback(GLFWwindow * window, int key, int scancode, int action, int mode)
{
	/*if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
	glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024)
	{
	if (action == GLFW_PRESS)
	{
	keyPressArray[key] = true;
	}
	else if (action == GLFW_RELEASE)
	{
	keyPressArray[key] = false;
	}
	}

	if (key == GLFW_KEY_C)
	{
	cout << "CameraPos: (" << CameraPos.x << ", " << CameraPos.y << ", " << CameraPos.z << ")" << endl;
	}*/
}

cv::Mat MatSL2CV(sl::Mat& input)
{
	// Mapping between MAT_TYPE and CV_TYPE
	int cv_type = -1;
	switch (input.getDataType()) {
	case sl::MAT_TYPE_32F_C1: cv_type = CV_32FC1; break;
	case sl::MAT_TYPE_32F_C2: cv_type = CV_32FC2; break;
	case sl::MAT_TYPE_32F_C3: cv_type = CV_32FC3; break;
	case sl::MAT_TYPE_32F_C4: cv_type = CV_32FC4; break;
	case sl::MAT_TYPE_8U_C1: cv_type = CV_8UC1; break;
	case sl::MAT_TYPE_8U_C2: cv_type = CV_8UC2; break;
	case sl::MAT_TYPE_8U_C3: cv_type = CV_8UC3; break;
	case sl::MAT_TYPE_8U_C4: cv_type = CV_8UC4; break;
	default: break;
	}

	// Since cv::Mat data requires a uchar* pointer, we get the uchar1 pointer from sl::Mat (getPtr<T>())
	// cv::Mat and sl::Mat will share a single memory structure
	return cv::Mat((int)input.getHeight(), (int)input.getWidth(), cv_type, input.getPtr<sl::uchar1>(sl::MEM_CPU));
}

void SetIntrinsicParams(sl::Camera &zedCam, cv::Mat &intrinsicParamsLeft, cv::Mat &intrinsicParamsRight, cv::Mat  &distParamsLeft, cv::Mat &distParamsRight, GLfloat intrinsicMatGL[2][16])
{
	sl::CalibrationParameters calibParam = zedCam.getCameraInformation().calibration_parameters;

	// Left
	SetIntrinsicMatCV(calibParam.left_cam, intrinsicParamsLeft, distParamsLeft);
	IntrinsicCVtoGL(intrinsicParamsLeft, intrinsicMatGL[0]);
	//std::cout << calibParam.left_cam.fx << " " << calibParam.left_cam.fy
	//		  << calibParam.left_cam.cx << " " << calibParam.left_cam.cy << std::endl;

	// Right
	SetIntrinsicMatCV(calibParam.right_cam, intrinsicParamsRight, distParamsRight);
	IntrinsicCVtoGL(intrinsicParamsRight, intrinsicMatGL[1]);
	//std::cout << calibParam.right_cam.fx << " " << calibParam.right_cam.fy
	//		  << calibParam.right_cam.cx << " " << calibParam.right_cam.cy << std::endl;

}

void SetIntrinsicMatCV(sl::CameraParameters camCalibParams, cv::Mat &intrinsicParams, cv::Mat &distPrams)
{
	if (intrinsicParams.empty())
	{
		intrinsicParams.create(3, 3, CV_32FC1);
	}

	if (distPrams.empty())
	{
		distPrams.create(1, 4, CV_32FC1);
	}

	intrinsicParams.at<float>(0, 0) = camCalibParams.fx;
	intrinsicParams.at<float>(0, 1) = 0.0f;
	intrinsicParams.at<float>(0, 2) = camCalibParams.cx;
	intrinsicParams.at<float>(1, 0) = 0.0f;
	intrinsicParams.at<float>(1, 1) = camCalibParams.fy;
	intrinsicParams.at<float>(1, 2) = camCalibParams.cy;
	intrinsicParams.at<float>(2, 0) = 0.0f;
	intrinsicParams.at<float>(2, 1) = 0.0f;
	intrinsicParams.at<float>(2, 2) = 1.0f;

	// Since ZED can directly retrive rectified camera frame, temporary set distortion factor as all zero.
	distPrams.at<float>(0, 0) = 0.0f;
	distPrams.at<float>(0, 1) = 0.0f;
	distPrams.at<float>(0, 2) = 0.0f;
	distPrams.at<float>(0, 3) = 0.0f;

	std::cout << "fx: " << intrinsicParams.at<float>(0, 0)
		<< ", fy: " << intrinsicParams.at<float>(1, 1)
		<< ", cx: " << intrinsicParams.at<float>(0, 2)
		<< ", cy: " << intrinsicParams.at<float>(1, 2) << std::endl;
}

void IntrinsicCVtoGL(cv::Mat intParam, GLfloat glProjection[16])
{
	int			i, j;
	double		p[3][3];
	double		q[4][4];

	memset(glProjection, 0, 16 * sizeof(double));

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			p[i][j] = intParam.at<float>(i, j);
		}
	}

	for (i = 0; i < 3; i++)
	{
		p[1][i] = (ZEDImgWidth - 1) * p[2][i] - p[1][i];
	}

	q[0][0] = (2.0  * p[0][0] / (ZEDImgWidth - 1));
	q[0][1] = (2.0  * p[0][1] / (ZEDImgWidth - 1));
	q[0][2] = ((2.0 * p[0][2] / (ZEDImgWidth - 1)) - 1.0);
	q[0][3] = 0.0;

	q[1][0] = 0.0;
	q[1][1] = (2.0  * p[1][1] / (ZEDImgHeight - 1));
	q[1][2] = ((2.0 * p[1][2] / (ZEDImgHeight - 1)) - 1.0);
	q[1][3] = 0.0;

	q[2][0] = 0.0;
	q[2][1] = 0.0;
	q[2][2] = (farClip + nearClip) / (farClip - nearClip);
	q[2][3] = -2.0 * farClip * nearClip / (farClip - nearClip);

	q[3][0] = 0.0;
	q[3][1] = 0.0;
	q[3][2] = 1.0;
	q[3][3] = 0.0;

	// transpose
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			glProjection[4 * i + j] = (GLfloat)q[j][i];
		}
	}
}

void ExtrinsicCVtoGL(cv::Mat RotMat, cv::Mat TransVec, GLfloat GLModelView[16])
{
	memset(GLModelView, 0, 16 * sizeof(GLfloat));

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			GLModelView[4 * i + j] = (GLfloat)RotMat.at<double>(j, i);
		}
		GLModelView[12 + i] = (GLfloat)TransVec.at<double>(i, 0);
	}
	GLModelView[15] = 1;
}

void DrawBGImage(cv::Mat BGImg, kuShaderHandler BGShader)
{
	static const GLfloat BGVertices[] = {
		1.0f,  1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 0.0f
	};

	GLuint indices[] = { 0, 1, 3,
		1, 2, 3 };

	GLuint BGVertexArray = 0;
	glGenVertexArrays(1, &BGVertexArray);
	GLuint BGVertexBuffer = 0;						// Vertex Buffer Object (VBO)
	glGenBuffers(1, &BGVertexBuffer);				// Give an ID to vertex buffer
	GLuint BGElementBuffer = 0;						// Element Buffer Object (EBO)
	glGenBuffers(1, &BGElementBuffer);

	glBindVertexArray(BGVertexArray);

	glBindBuffer(GL_ARRAY_BUFFER, BGVertexBuffer);  // Bind buffer as array buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(BGVertices), BGVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BGElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Assign vertex position data
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Assign texture coordinates
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	GLuint BGImgTextureID = CreateTexturebyImage(BGImg);

	BGShader.Use();

	glBindTexture(GL_TEXTURE_2D, BGImgTextureID);

	glBindVertexArray(BGVertexArray);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDeleteTextures(1, &BGImgTextureID);

	glDeleteVertexArrays(1, &BGVertexArray);
	glDeleteBuffers(1, &BGVertexBuffer);
	glDeleteBuffers(1, &BGElementBuffer);
}

GLuint CreateTexturebyImage(cv::Mat Img)
{
	GLuint	texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT (usually basic wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Img.cols, Img.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, Img.data);
	//glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}
