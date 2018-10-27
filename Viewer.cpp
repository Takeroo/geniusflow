/*******************************************************************************
******************/

#include <map>
#include "Viewer.h"
#include <iostream>
#include <fstream>
#include <cmath> 

#include <windows.h>



#if (ONI_PLATFORM == ONI_PLATFORM_MACOSX)
        #include <GLUT/glut.h>
#else
        #include <GL/glut.h>
#endif

#include "HistoryBuffer.h"
#include <NiteSampleUtilities.h>

#define GL_WIN_SIZE_X	800
#define GL_WIN_SIZE_Y	600
#define TEXTURE_SIZE	512

#define DEFAULT_DISPLAY_MODE	DISPLAY_MODE_DEPTH

#define MIN_NUM_CHUNKS(data_size, chunk_size)	((((data_size)-1) / (chunk_size) + 1))
#define MIN_CHUNKS_SIZE(data_size, chunk_size)	(MIN_NUM_CHUNKS(data_size, chunk_size) * (chunk_size))

using namespace std;

SampleViewer* SampleViewer::ms_self = NULL;

std::map<int, HistoryBuffer<20> *> g_histories;

bool g_drawDepth = true;
bool g_smoothing = false;
bool g_drawFrameId = false;

int g_nXRes = 0, g_nYRes = 0;

bool rightBool[2] = { false };
bool leftBool[2] = { false };
bool upBool[2] = { false };
bool downBool[2] = { false };
bool changeInX[2] = { false };
bool changeInY[2] = { false };
int count = 0;
int warnCount = 0;
double initialX = 0;
double initialY = 0;
double finalX = 0;
double finalY = 0;
int X=0, Y=0;



ofstream a_file("OmurX.txt");

void SampleViewer::glutIdle()
{
	glutPostRedisplay();
}
void SampleViewer::glutDisplay()
{
	SampleViewer::ms_self->Display();
}
void SampleViewer::glutKeyboard(unsigned char key, int x, int y)
{
	SampleViewer::ms_self->OnKey(key, x, y);
}

SampleViewer::SampleViewer(const char* strSampleName)
{
	ms_self = this;
	strncpy(m_strSampleName, strSampleName, ONI_MAX_STR);
	m_pHandTracker = new nite::HandTracker;
}
SampleViewer::~SampleViewer()
{
	Finalize();

	delete[] m_pTexMap;

	ms_self = NULL;
}

void SampleViewer::Finalize()
{
	delete m_pHandTracker;
	nite::NiTE::shutdown();
	openni::OpenNI::shutdown();

	a_file.close();
}




openni::Status SampleViewer::Init(int argc, char **argv)
{
	m_pTexMap = NULL;

	openni::OpenNI::initialize();

	const char* deviceUri = openni::ANY_DEVICE;
	for (int i = 1; i < argc-1; ++i)
	{
		if (strcmp(argv[i], "-device") == 0)
		{
			deviceUri = argv[i+1];
			break;
		}
	}

	openni::Status rc = m_device.open(deviceUri);
	if (rc != openni::STATUS_OK)
	{
		printf("Open Device failed:\n%s\n", openni::OpenNI::getExtendedError());
		return rc;
	}

	nite::NiTE::initialize();

	if (m_pHandTracker->create(&m_device) != nite::STATUS_OK)
	{
		return openni::STATUS_ERROR;
	}

	m_pHandTracker->startGestureDetection(nite::GESTURE_WAVE);
	m_pHandTracker->startGestureDetection(nite::GESTURE_CLICK);
	m_pHandTracker->startGestureDetection(nite::GESTURE_HAND_RAISE);

	return InitOpenGL(argc, argv);

}
openni::Status SampleViewer::Run()	//Does not return
{
	glutMainLoop();

	return openni::STATUS_OK;
}

float Colors[][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 1}};
int colorCount = 3;

void DrawHistory(nite::HandTracker* pHandTracker, int id, HistoryBuffer<20>* pHistory)
{
	glColor3f(Colors[id % colorCount][0], Colors[id % colorCount][1], Colors[id % colorCount][2]);
	float coordinates[60] = {0};
	float coordinates2[60] = { 0 };
	float factorX = GL_WIN_SIZE_X / (float)g_nXRes;
	float factorY = GL_WIN_SIZE_Y / (float)g_nYRes;

	for (int i = 0; i < pHistory->GetSize(); ++i)
	{
		const nite::Point3f& position = pHistory->operator[](i);
		
		pHandTracker->convertHandCoordinatesToDepth(position.x, position.y, position.z, &coordinates[i*3], &coordinates[i*3+1]);

		coordinates[i*3]   *= factorX;
		coordinates[i*3+1] *= factorY;
		coordinates[i * 3 + 2] = position.z;

		coordinates2[i * 3] = coordinates[i * 3] * 2 * 0.54*position.z / GL_WIN_SIZE_X;

		coordinates2[i * 3+1] = coordinates[i * 3+1] * 2 * 0.4*position.z / GL_WIN_SIZE_Y;

	}

	//cout << coordinates2[0] <<endl;

		


	//MOUSE
	
	/*double screenRes_width = ::GetSystemMetrics(SM_CXSCREEN) - 1;
	double screenRes_height = ::GetSystemMetrics(SM_CYSCREEN) - 1;

	double dx = (coordinates2[0])*(65535.0f / screenRes_width);
	double dy = coordinates2[1]*(65535.0f / screenRes_height);

	INPUT Input = { 0 };

	Input.type = INPUT_MOUSE;

	Input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
	Input.mi.dx = LONG(dx);
	Input.mi.dy = LONG(dy);
	SendInput(1, &Input, sizeof(INPUT));
*/
	//LEFT


	//cout << coordinates2[0] << endl;

	//cout << coordinates2[2] << endl;

	if (coordinates2[0] - coordinates2[3] > 0){
		changeInX[1] = changeInX[0];
		changeInX[0] = true;
	}
	if (coordinates2[0] - coordinates2[3] < 0){
		changeInX[1] = changeInX[0];
		changeInX[0] = false;
	}
	if (coordinates2[1] - coordinates2[4] > 0){
		changeInY[1] = changeInY[0];
		changeInY[0] = true;
	}
	if (coordinates2[1] - coordinates2[4] < 0){
		changeInY[1] = changeInY[0];
		changeInY[0] = false;
	}
	count++;
	warnCount++;
	if (changeInX[0] != changeInX[1]){
		finalX = coordinates2[3];
		finalY = coordinates2[4];
		if (abs(finalX - initialX)>10 && abs(finalY - initialY) > 10){

			a_file << finalX - initialX << ",	" << (initialY - finalY) << ",	" << count;
			a_file << "\n";
			warnCount = 0;

		}

		if (warnCount > 150){

			cout << "YOU HAVE NOT MOVE YOUR HANDS IN 5 SECONDS, MOVE YOUR HANDS PLS" << endl;
			cout << "YOU HAVE NOT MOVE YOUR HANDS IN 5 SECONDS, MOVE YOUR HANDS PLS" << endl;
			cout << "YOU HAVE NOT MOVE YOUR HANDS IN 5 SECONDS, MOVE YOUR HANDS PLS" << endl;
			cout << "YOU HAVE NOT MOVE YOUR HANDS IN 5 SECONDS, MOVE YOUR HANDS PLS" << endl;
			warnCount = 0;
		}
		count = 0;
		initialX = coordinates2[0];
		initialY = coordinates2[1];
	}



	if ((coordinates2[0] - coordinates2[57])<-450 && coordinates[59] - coordinates[2]>250 && coordinates2[57]!=0){
		
		leftBool[1] = leftBool[0];
		leftBool[0] = true;
		
	}
	else{

		leftBool[1] = leftBool[0];
		leftBool[0] = false;
	}
	if (leftBool[0] == false && leftBool[1] == true){

		INPUT next;
		next.type = INPUT_KEYBOARD;
		next.ki.wScan = 0; // hardware scan code for key
		next.ki.time = 0;
		next.ki.dwExtraInfo = 0;

		next.ki.wVk = 0x27;

		next.ki.dwFlags = 0;
		SendInput(1, &next, sizeof(INPUT));
		// Release the "A" key
		next.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
		SendInput(1, &next, sizeof(INPUT));
		printf("SWIPE RIGHT");
		
	}


	//RIGHT

	if ((coordinates2[0] - coordinates2[57])>350 && coordinates[59] - coordinates[2]>150 && coordinates2[57] != 0){
		bool a = true;
		rightBool[1] = rightBool[0];
		rightBool[0] = a;

	}
	else{

		rightBool[1] = rightBool[0];
		rightBool[0] = false;
	}
	if (rightBool[0] == false && rightBool[1] == true){


		INPUT next;
		next.type = INPUT_KEYBOARD;
		next.ki.wScan = 0; // hardware scan code for key
		next.ki.time = 0;
		next.ki.dwExtraInfo = 0;

		next.ki.wVk = 0x25;

		next.ki.dwFlags = 0;
		SendInput(1, &next, sizeof(INPUT));
		// Release the "A" key
		next.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
		SendInput(1, &next, sizeof(INPUT));
		printf("SWIPE LEFT");

	}


	//Up
	if ((coordinates2[1] - coordinates2[58])<-350 && coordinates[59] - coordinates[2]>150 && coordinates2[58] != 0){
		bool a = true;
		upBool[1] = upBool[0];
		upBool[0] = a;

	}
	else{

		upBool[1] = upBool[0];
		upBool[0] = false;
	}

	if (upBool[0] == false && upBool[1] == true){
		//printf("up and TASKSWITCH\n");


		INPUT alt;
		INPUT tab;

		alt.type = INPUT_KEYBOARD;
		alt.ki.wScan = 0; // hardware scan code for key
		alt.ki.time = 0;
		alt.ki.dwExtraInfo = 0;

		tab.type = INPUT_KEYBOARD;
		tab.ki.wScan = 0; // hardware scan code for key
		tab.ki.time = 0;
		tab.ki.dwExtraInfo = 0;

		alt.ki.wVk = 0x12;
		tab.ki.wVk = 0x09;

		alt.ki.dwFlags = 0;
		SendInput(1, &alt, sizeof(INPUT));

		tab.ki.dwFlags = 0;
		SendInput(1, &tab, sizeof(INPUT));
		// Release the "A" key
		tab.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
		SendInput(1, &tab, sizeof(INPUT));

		//alt.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
		//SendInput(1, &alt, sizeof(INPUT));

		printf("taskSwitch\n");

	}




	//DOWN
	if ((coordinates2[1] - coordinates2[58])>350 && coordinates[59] - coordinates[2]>150 && coordinates2[58] != 0){
		bool a = true;
		downBool[1] = downBool[0];
		downBool[0] = a;

	}
	else{

		downBool[1] = downBool[0];
		downBool[0] = false;
	}

	if (downBool[0] == false && downBool[1] == true){

		//printf("down and TASKSWITCH Release\n");


		INPUT alt;

		alt.type = INPUT_KEYBOARD;
		alt.ki.wScan = 0; // hardware scan code for key
		alt.ki.time = 0;
		alt.ki.dwExtraInfo = 0;

		alt.ki.wVk = 0x12;

		alt.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
		SendInput(1, &alt, sizeof(INPUT));
		printf("RELEASE\n");
	}


	glPointSize(20);
	glVertexPointer(3, GL_FLOAT, 0, coordinates);
	glDrawArrays(GL_LINE_STRIP, 0, pHistory->GetSize());


	glPointSize(30);
	glVertexPointer(3, GL_FLOAT, 0, coordinates);
	glDrawArrays(GL_POINTS, 0, 1);

}

#ifndef USE_GLES
void glPrintString(void *font, const char *str)
{
	int i,l = (int)strlen(str);

	for(i=0; i<l; i++)
	{   
		glutBitmapCharacter(font,*str++);
	}   
}
#endif
void DrawFrameId(int frameId)
{
	char buffer[80] = "";
	sprintf(buffer, "%d", frameId);
	glColor3f(1.0f, 0.0f, 0.0f);
	glRasterPos2i(20, 20);
	glPrintString(GLUT_BITMAP_HELVETICA_18, buffer);
}


void SampleViewer::Display()
{
	nite::HandTrackerFrameRef handFrame;
	openni::VideoFrameRef depthFrame;
	nite::Status rc = m_pHandTracker->readFrame(&handFrame);
	if (rc != nite::STATUS_OK)
	{
		printf("GetNextData failed\n");
		return;
	}

	depthFrame = handFrame.getDepthFrame();

	if (m_pTexMap == NULL)
	{
		// Texture map init
		m_nTexMapX = MIN_CHUNKS_SIZE(depthFrame.getVideoMode().getResolutionX(), TEXTURE_SIZE);
		m_nTexMapY = MIN_CHUNKS_SIZE(depthFrame.getVideoMode().getResolutionY(), TEXTURE_SIZE);
		m_pTexMap = new openni::RGB888Pixel[m_nTexMapX * m_nTexMapY];
	}


	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, GL_WIN_SIZE_X, GL_WIN_SIZE_Y, 0, -1.0, 1.0);

	if (depthFrame.isValid())
	{
		calculateHistogram(m_pDepthHist, MAX_DEPTH, depthFrame);
	}

	memset(m_pTexMap, 0, m_nTexMapX*m_nTexMapY*sizeof(openni::RGB888Pixel));

	float factor[3] = {1, 1, 1};

	// check if we need to draw depth frame to texture
	if (depthFrame.isValid() && g_drawDepth)
	{
		const openni::DepthPixel* pDepthRow = (const openni::DepthPixel*)depthFrame.getData();
		openni::RGB888Pixel* pTexRow = m_pTexMap + depthFrame.getCropOriginY() * m_nTexMapX;
		int rowSize = depthFrame.getStrideInBytes() / sizeof(openni::DepthPixel);

		for (int y = 0; y < depthFrame.getHeight(); ++y)
		{
			const openni::DepthPixel* pDepth = pDepthRow;
			openni::RGB888Pixel* pTex = pTexRow + depthFrame.getCropOriginX();

			for (int x = 0; x < depthFrame.getWidth(); ++x, ++pDepth, ++pTex)
			{
				if (*pDepth != 0)
				{
					factor[0] = Colors[colorCount][0];
					factor[1] = Colors[colorCount][1];
					factor[2] = Colors[colorCount][2];

					int nHistValue = m_pDepthHist[*pDepth];
					pTex->r = nHistValue*factor[0];
					pTex->g = nHistValue*factor[1];
					pTex->b = nHistValue*factor[2];

					factor[0] = factor[1] = factor[2] = 1;
				}
			}

			pDepthRow += rowSize;
			pTexRow += m_nTexMapX;
		}
	}

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_nTexMapX, m_nTexMapY, 0, GL_RGB, GL_UNSIGNED_BYTE, m_pTexMap);

	// Display the OpenGL texture map
	glColor4f(1,1,1,1);

	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);

	g_nXRes = depthFrame.getVideoMode().getResolutionX();
	g_nYRes = depthFrame.getVideoMode().getResolutionY();

	// upper left
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	// upper right
	glTexCoord2f((float)g_nXRes/(float)m_nTexMapX, 0);
	glVertex2f(GL_WIN_SIZE_X, 0);
	// bottom right
	glTexCoord2f((float)g_nXRes/(float)m_nTexMapX, (float)g_nYRes/(float)m_nTexMapY);
	glVertex2f(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	// bottom left
	glTexCoord2f(0, (float)g_nYRes/(float)m_nTexMapY);
	glVertex2f(0, GL_WIN_SIZE_Y);

	glEnd();
	glDisable(GL_TEXTURE_2D);

	const nite::Array<nite::GestureData>& gestures = handFrame.getGestures();
	for (int i = 0; i < gestures.getSize(); ++i)
	{
		if (gestures[i].isComplete())
		{
			if (gestures[i].getType() == 1){
				enter();
			}
			const nite::Point3f& position = gestures[i].getCurrentPosition();
			//printf("Gesture %d at (%f,%f,%f)\n", gestures[i].getType(), position.x, position.y, position.z);

			nite::HandId newId;
			m_pHandTracker->startHandTracking(gestures[i].getCurrentPosition(), &newId);
		}
	}

	const nite::Array<nite::HandData>& hands= handFrame.getHands();

	for (int i = 0; i < hands.getSize(); ++i)
	{
		const nite::HandData& user = hands[i];

		if (!user.isTracking())
		{
			printf("Lost hand %d\n", user.getId());
			nite::HandId id = user.getId();
			HistoryBuffer<20>* pHistory = g_histories[id];
			g_histories.erase(g_histories.find(id));
			delete pHistory;

			m_pHandTracker->startGestureDetection(nite::GESTURE_HAND_RAISE);
		}
		else
		{
			if (user.isNew())
			{
				printf("Found hand %d\n", user.getId());
				g_histories[user.getId()] = new HistoryBuffer<20>;

				m_pHandTracker->stopGestureDetection(nite::GESTURE_HAND_RAISE);
			}
			// Add to history
			HistoryBuffer<20>* pHistory = g_histories[user.getId()];
			pHistory->AddPoint(user.getPosition());
			// Draw history
			DrawHistory(m_pHandTracker, user.getId(), pHistory);
		}
	}
	if (hands.getSize() == 2){
		if (hands[0].isTracking() &&  hands[1].isTracking()){

			HistoryBuffer<20>* aHistory = g_histories[hands[0].getId()];
			aHistory->AddPoint(hands[0].getPosition());

			HistoryBuffer<20>* bHistory = g_histories[hands[1].getId()];
			bHistory->AddPoint(hands[1].getPosition());

			float coordinates0[60] = { 0 };
			float coordinates1[60] = { 0 };

			float coordinates2[60] = { 0 };
			float coordinates3[60] = { 0 };

			float factorX = GL_WIN_SIZE_X / (float)g_nXRes;
			float factorY = GL_WIN_SIZE_Y / (float)g_nYRes;

			for (int i = 0; i < aHistory->GetSize(); ++i)
			{
				const nite::Point3f& position0 = aHistory->operator[](i);
				const nite::Point3f& position1 = bHistory->operator[](i);

				m_pHandTracker->convertHandCoordinatesToDepth(position0.x, position0.y, position0.z, &coordinates0[i * 3], &coordinates0[i * 3 + 1]);
				m_pHandTracker->convertHandCoordinatesToDepth(position1.x, position1.y, position1.z, &coordinates2[i * 3], &coordinates2[i * 3 + 1]);

				coordinates0[i * 3] *= factorX;
				coordinates0[i * 3 + 1] *= factorY;
				coordinates0[i * 3 + 2] = position0.z;

				coordinates1[i * 3] = coordinates0[i * 3] * 2 * 0.54*position0.z / GL_WIN_SIZE_X;

				coordinates1[i * 3 + 1] = coordinates0[i * 3 + 1] * 2 * 0.4*position0.z / GL_WIN_SIZE_Y;


				coordinates2[i * 3] *= factorX;
				coordinates2[i * 3 + 1] *= factorY;
				coordinates2[i * 3 + 2] = position1.z;

				coordinates3[i * 3] = coordinates2[i * 3] * 2 * 0.54*position1.z / GL_WIN_SIZE_X;

				coordinates3[i * 3 + 1] = coordinates2[i * 3 + 1] * 2 * 0.4*position1.z / GL_WIN_SIZE_Y;

			}

			

			if (coordinates1[57]-coordinates1[0]>200 && coordinates3[0]-coordinates3[57]>200){
				

				if (abs(coordinates3[0] - coordinates1[0]) > abs(coordinates3[57] - coordinates1[57])){
					zoomIn();
				}
				else{
					zoomOut();
				}
					

			}

			if (coordinates1[0] - coordinates1[57]>200 && coordinates3[57] - coordinates3[0]>200){
				
				if (abs(coordinates1[0] - coordinates3[0]) > abs(coordinates1[57] - coordinates3[57])){
					zoomIn();
				}
				else{
					zoomOut();
				}
			}
		}
	}

	if (g_drawFrameId)
	{
		DrawFrameId(handFrame.getFrameIndex());
	}

	// Swap the OpenGL display buffers
	glutSwapBuffers();

}

void SampleViewer::OnKey(unsigned char key, int /*x*/, int /*y*/)
{
	switch (key)
	{
	case 27:
		Finalize();
		exit (1);
	case 'd':
		g_drawDepth = !g_drawDepth;
		break;
	case 's':
		if (g_smoothing)
		{
			// Turn off smoothing
			m_pHandTracker->setSmoothingFactor(0);
			g_smoothing = FALSE;
		}
		else
		{
			m_pHandTracker->setSmoothingFactor(0.1);
			g_smoothing = TRUE;
		}
		break;
	case 'f':
		// Draw frame ID
		g_drawFrameId = !g_drawFrameId;
		break;
	}

}

openni::Status SampleViewer::InitOpenGL(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	glutCreateWindow (m_strSampleName);
	// 	glutFullScreen();
	glutSetCursor(GLUT_CURSOR_NONE);

	InitOpenGLHooks();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	return openni::STATUS_OK;

}
void SampleViewer::InitOpenGLHooks()
{
	glutKeyboardFunc(glutKeyboard);
	glutDisplayFunc(glutDisplay);
	glutIdleFunc(glutIdle);
}

void SampleViewer::nextImage(){
	INPUT next;
	next.type = INPUT_KEYBOARD;
	next.ki.wScan = 0; // hardware scan code for key
	next.ki.time = 0;
	next.ki.dwExtraInfo = 0;

	next.ki.wVk = 0x27;

	next.ki.dwFlags = 0;
	SendInput(1, &next, sizeof(INPUT));
	// Release the "A" key
	next.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &next, sizeof(INPUT));
	printf("Next image\n");
}

void SampleViewer::taskSwitch(){
	INPUT alt;
	INPUT tab;

	alt.type = INPUT_KEYBOARD;
	alt.ki.wScan = 0; // hardware scan code for key
	alt.ki.time = 0;
	alt.ki.dwExtraInfo = 0;

	tab.type = INPUT_KEYBOARD;
	tab.ki.wScan = 0; // hardware scan code for key
	tab.ki.time = 0;
	tab.ki.dwExtraInfo = 0;

	alt.ki.wVk = 0x12;
	tab.ki.wVk = 0x09;

	alt.ki.dwFlags = 0;
	SendInput(1, &alt, sizeof(INPUT));

	tab.ki.dwFlags = 0;
	SendInput(1, &tab, sizeof(INPUT));
	// Release the "A" key
	tab.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &tab, sizeof(INPUT));

	//alt.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	//SendInput(1, &alt, sizeof(INPUT));

	printf("taskSwitch\n");
}

void SampleViewer::taskSwitchRelease(){
	INPUT alt;

	alt.type = INPUT_KEYBOARD;
	alt.ki.wScan = 0; // hardware scan code for key
	alt.ki.time = 0;
	alt.ki.dwExtraInfo = 0;

	alt.ki.wVk = 0x12;

	alt.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &alt, sizeof(INPUT));
	printf("RELEASE\n");
}

void SampleViewer::enter(){
	INPUT enter;
	enter.type = INPUT_KEYBOARD;
	enter.ki.wScan = 0; // hardware scan code for key
	enter.ki.time = 0;
	enter.ki.dwExtraInfo = 0;

	enter.ki.wVk = 0x0D;

	enter.ki.dwFlags = 0;
	SendInput(1, &enter, sizeof(INPUT));
	// Release the "A" key
	enter.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &enter, sizeof(INPUT));
	printf("ENTER\n");


/*
	INPUT input;
	input.type = INPUT_MOUSE;

	POINT t1;
	GetCursorPos(&t1);

	input.mi.dx = t1.x;

	cout << t1.x << endl;

	cout << t1.y << endl;
	input.mi.dy = t1.y;
	input.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP);
	input.mi.mouseData = 0;
	input.mi.dwExtraInfo = NULL;
	input.mi.time = 0;
	SendInput(1, &input, sizeof(INPUT));*/


	INPUT alt;

	alt.type = INPUT_KEYBOARD;
	alt.ki.wScan = 0; // hardware scan code for key
	alt.ki.time = 0;
	alt.ki.dwExtraInfo = 0;
	
	alt.ki.wVk = 0x12;

	alt.ki.dwFlags = 0;
	SendInput(1, &alt, sizeof(INPUT));

	alt.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &alt, sizeof(INPUT));
	printf("RELEASE\n");
}

void SampleViewer::zoomIn(){
	INPUT ctrl;
	INPUT zoom;

	ctrl.type = INPUT_KEYBOARD;
	ctrl.ki.wScan = 0; // hardware scan code for key
	ctrl.ki.time = 0;
	ctrl.ki.dwExtraInfo = 0;

	zoom.type = INPUT_KEYBOARD;
	zoom.ki.wScan = 0; // hardware scan code for key
	zoom.ki.time = 0;
	zoom.ki.dwExtraInfo = 0;

	ctrl.ki.wVk = 0x11;
	zoom.ki.wVk = 0xBB;

	ctrl.ki.dwFlags = 0;
	SendInput(1, &ctrl, sizeof(INPUT));

	zoom.ki.dwFlags = 0;
	SendInput(1, &zoom, sizeof(INPUT));
	// Release the "A" key
	zoom.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &zoom, sizeof(INPUT));

	ctrl.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &ctrl, sizeof(INPUT));
	cout << "ZOOM IN" << endl;
}

void SampleViewer::zoomOut(){
	INPUT ctrl;
	INPUT zoom;

	ctrl.type = INPUT_KEYBOARD;
	ctrl.ki.wScan = 0; // hardware scan code for key
	ctrl.ki.time = 0;
	ctrl.ki.dwExtraInfo = 0;

	zoom.type = INPUT_KEYBOARD;
	zoom.ki.wScan = 0; // hardware scan code for key
	zoom.ki.time = 0;
	zoom.ki.dwExtraInfo = 0;

	ctrl.ki.wVk = 0x11;
	zoom.ki.wVk = 0xBD;

	ctrl.ki.dwFlags = 0;
	SendInput(1, &ctrl, sizeof(INPUT));

	zoom.ki.dwFlags = 0;
	SendInput(1, &zoom, sizeof(INPUT));
	// Release the "A" key
	zoom.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &zoom, sizeof(INPUT));

	ctrl.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &ctrl, sizeof(INPUT));
	cout << "ZOOM OUT" << endl;
}