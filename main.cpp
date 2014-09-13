//---------------------------------------------------------------------------
//
// This software is provided 'as-is' for assignment of COMP308
// in ECS, Victoria University of Wellington,
// without any express or implied warranty.
// In no event will the authors be held liable for any
// damages arising from the use of this software.
//
// The contents of this file may not be copied or duplicated in any form
// without the prior permission of its owner.
//
// Copyright (c) 2012 by Taehyun Rhee
//
// Edited by Roma Klapaukh, Daniel Atkins, and Taehyun Rhee
//
//---------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <string>
#include <string.h>
#include <GL/glut.h>
#include "define.h"
#include "FileIO.h"
#include "G308_Skeleton.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

//menu options for amc player
enum MENU_ENUM {
	PLAY, PAUSE, STOP, REWIND, FAST_FORWARD
};

MENU_ENUM selected_mode = PAUSE;

char* filename;

bool playing = false;
bool firstClick = true;
static int menu_id;
char* selected_name[100];
//int right_button_state = 0; //is right button down?
//int shift_right_button_state = 0;
//int shift_left_button_state = 0;

bool ROTATING, PANNING;

float oldX, oldY; //old mouse x and y position
float oldMagnitude; //old mouse motion magnitude

GLuint g_mainWnd;
GLuint g_nWinWidth = G308_WIN_WIDTH;
GLuint g_nWinHeight = G308_WIN_HEIGHT;

float zoom = 100;

float yRot = 0;
float xRot = 0;
float zRot = 0;

float lightX = 200;
float lightY = 200;
float lightZ = 200;

//arcball
glm::quat cam_angle{1, 0, 0, 0};
glm::quat cam_angle_d{1, 0, 0, 0};
glm::quat click_old{1, 0, 0, 0};
glm::quat click_new{1, 0, 0, 0};

glm::vec3 focus;

// mouse action settings
float arcball_x = 0.0;
float arcball_y = 0.0;
float arcball_radius = 2.0;
float click_x = 0.0;
float click_y = 0.0;


glm::vec3 cameraPosition = {0.0,10.0,20.0};

void G308_keyboardListener(unsigned char, int, int);
void G308_Reshape(int w, int h);
void G308_display();
void G308_init();
void G308_SetCamera();
void G308_SetLight();
void angleToText();
boneOp getTargetBone();
void displayText( float x, float y, int r, int g, int b, const char *string);

void createMenu();
void menu(int);

void onMouse(int button, int state, int x, int y);
void onDrag(int x, int y);

void savePose();
void readPose(int framenum, char* filename);

//arc

void getArc(int, int, int, int, float, glm::quat &);
void getUnitCircle(int, int, int, int, glm::quat &);

Skeleton* skeleton = NULL;
Skeleton* skeletonDefault = NULL;
FileIO* fileInput = NULL;



int main(int argc, char** argv) {
	if (argc < 2) {
		//Usage instructions for core and challenge
		printf("Usage\n");
		printf("./Ass2 priman.asf [config.txt]\n");
		exit(EXIT_FAILURE);
	}

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(g_nWinWidth, g_nWinHeight);
	g_mainWnd = glutCreateWindow("COMP308 Assignment2");

	glutKeyboardFunc(G308_keyboardListener);
	glutDisplayFunc(G308_display);
	glutReshapeFunc(G308_Reshape);
	glutMouseFunc(onMouse);
	glutMotionFunc(onDrag);

	// [Assignment2] : Read ASF file
	if (argc > 1) {

		filename = argv[1];
		skeleton = new Skeleton();
		fileInput = new FileIO(skeleton->root);
		fileInput->readASF(argv[1]);
		skeleton->amcPlayerMode = false;
		if (argc > 2 ) {
			skeleton->amcPlayerMode = true;
			skeleton->readConfig(argv[2]);
		}
	}

	createMenu();
	G308_init();

	glutIdleFunc(glutPostRedisplay);
	glutMainLoop();

	return EXIT_SUCCESS;
}

void savePose() {
	skeleton->traverseHierachy();
	skeleton->writePoseToFile();
}

void readPose(int framenum, char* filename) {
	skeleton->readPose(framenum, filename);
}

//GLUT menu constructor
void createMenu() {

	menu_id = glutCreateMenu(menu);

	glutAddMenuEntry("Rewind", REWIND);
	glutAddMenuEntry("Play", PLAY);
	glutAddMenuEntry("Pause", PAUSE);
	glutAddMenuEntry("Stop", STOP);
	glutAddMenuEntry("Fast Forward", FAST_FORWARD);

	glutAttachMenu(GLUT_MIDDLE_BUTTON);
}
//GLUT menu callback
void menu(int button) {
	selected_mode = (MENU_ENUM) button;
}
// Init Light and Camera
void G308_init() {
	G308_SetLight();
	G308_SetCamera();
}

void angleToText() {
	//what joint has been selected?
	char* result = skeleton->selected->name;
	//the angles
	glm::vec3 r = glm::eulerAngles(getTargetBone().startQuat);
	float rotx = r.x;
	float roty = r.y;
	float rotz = r.z;
	//concat the data to make a string for printing
	char buffer[100];
	sprintf(buffer, "Bone: %s (rotx: %.2f, roty: %.2f, rotz: %.2f)", result,
			rotx, roty, rotz); // puts string into buffer
	displayText(-0.95f, 0.9f, 255, 255, 255, buffer);
}

void drawBoneText() {
	//TEXT
	glMatrixMode(GL_PROJECTION);
	glPushMatrix(); // save
	glLoadIdentity(); // and clear
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glDisable( GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	/*
	 * What is preferable? the total x,y,z angles, or just how much we
	 * have incremented the original positions by?
	 */

	if (skeleton->selected != NULL) {
		if (skeleton->selected->parent != NULL) {
			angleToText();
		}
	}
	glEnable (GL_LIGHTING);
	glEnable( GL_DEPTH_TEST);
	glMatrixMode( GL_PROJECTION);
	glPopMatrix(); // revert back to the matrix I had before.
	glMatrixMode( GL_MODELVIEW);
	glPopMatrix();
}
// Display call back
void G308_display() {

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glShadeModel(GL_SMOOTH);

	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		printf("%s\n", gluErrorString(err));
	}

	if(skeleton->amcPlayerMode) {
		G308_SetCamera();
	}

	// [Assignmet2] : render skeleton
	if (skeleton != NULL) {
		skeleton->display();
	}

	if(skeleton->amcPlayerMode) {
		glPopMatrix();
	}

	//TEXT
	drawBoneText();

	switch(selected_mode) {

		case PLAY:
			skeleton->play();
			break;
		case PAUSE:
			skeleton->pause();
			break;
		case STOP:
			skeleton->stop();
			break;
		case REWIND:
			skeleton->rewind();
			break;
		case FAST_FORWARD:
			skeleton->fastforward();
			break;
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);

	glutSwapBuffers();
	G308_SetLight();
}

void pickByColor(int x, int y) {
	//turn off texturing, lighting and fog
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_LIGHT0);
	if (skeleton->amcPlayerMode == false) {
		unsigned char pixel[3];
		skeleton->display();
		glEnable(GL_SCISSOR_TEST);
		glScissor(x, y, 1, 1);
		glReadPixels(x, g_nWinHeight - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE,
				pixel);
		glEnable(GL_LIGHT0);
		glDisable(GL_SCISSOR_TEST);
		//currently selected bone
		if (skeleton->findBoneById(pixel) != NULL) {
			skeleton->selected = skeleton->findBoneById(pixel);
		}
	}
}

// On mouse click call back, used for selected bones
void onMouse(int button, int state, int x, int y) {
	//reset
	if (state) {
		ROTATING = PANNING = false;
	}
	//if shift is being held down
	if((glutGetModifiers() & GLUT_ACTIVE_SHIFT) == GLUT_ACTIVE_SHIFT) {
		//rotating
		if(button == 0) {
			ROTATING = true;
			getArc( arcball_x, arcball_y, x, y, arcball_radius, click_new ); // initial click down
			click_old = click_new;
			firstClick = false;
		//panning
		} else if(button == 2) {
			PANNING = true;
			click_x = x;
			click_y = y;
			firstClick = false;
		//zoom
		} else if(button == 3) {
			zoom*=1.1;
		} else if(button == 4) {
			zoom*=0.9;
		}
		//check off that we have acquired coordinates from first click

	//if control is pressed down
	} else if((glutGetModifiers() & GLUT_ACTIVE_CTRL) == GLUT_ACTIVE_CTRL) {
	//turn off texturing, lighting and fog
		if(button == 0) {
			pickByColor(x, y);
		}
	}

}


void resize(int x, int y) {
	g_nWinWidth = x;
	g_nWinHeight = y;
	arcball_x = (x / 2.0);
	arcball_y = (y / 2.0);
	arcball_radius = (x / 2.0);
}


void rotateBone(float magnitude, float xamount,float yamount, float zamount, boneOp& b) {
	if (magnitude > oldMagnitude) {
		glm::quat change = glm::quat(glm::vec3(xamount, yamount, zamount));
		b.startQuat *= change;
	} else {
		glm::quat change = glm::quat(glm::vec3(-xamount, -yamount, -zamount));
		b.startQuat *= change;
	}
}

boneOp getTargetBone() {
	return skeleton->selected->parent->animationFrame[skeleton->amcFrame];
}

void boneRotationCtrl(float magnitude) {
	//if the mouse is generally moving in a positive direction
	boneOp b = getTargetBone();
	float amount = 0.01;
	switch (skeleton->currentAxis) {
	case X:
		rotateBone(magnitude, amount, 0, 0, b);
		break;
	case Y:
		rotateBone(magnitude, 0, amount, 0, b);
		break;
	case Z:
		rotateBone(magnitude, 0, 0, amount, b);
		break;
	default:
		break;
	}
	skeleton->selected->parent->animationFrame[skeleton->amcFrame] = b;
}

void rotateOnDrag(int x, int y) {
	//what we really want is the magnitude of the mouse movement
	//previously was check for oldX vs newX
	float magnitude = sqrt(x * x + y * y);
		/*
		 * Welcome to terrible C++ programming with reuben!
		 */
		if (skeleton->selected != NULL) {
			if (skeleton->selected->parent != NULL) {
				//if the mouse is generally moving in a positive direction
				boneRotationCtrl(magnitude);
			}
		}

	oldX = x;
	oldY = y;
	//set the reference magnitude for next call of this function
	oldMagnitude = sqrt(oldX * oldX + oldY * oldY);
}


void getArc(int arcx, int arcy, int ix, int iy, float rad, glm::quat &result) {

	float iyf = (float) (g_nWinHeight - (float) iy);
	float x = (ix - arcx) / rad;
	float y = (iyf - arcy) / rad;

	// check click is inside the arcball radius
	if (x*x + y*y < 1.0) {
		float z = sqrt(1 - (x*x + y*y));
		result = glm::quat(0, x, y, z);
	}
	else {
		float len = sqrt(x*x + y*y);
		result = glm::quat(0, x / len, y / len, 0);
	}
}

//On mouse motion callback, used for rotating selected joints
void onDrag(int x, int y) {
	//on shift and left click
	if(ROTATING) {
		getArc(arcball_x, arcball_y, x, y, arcball_radius, click_new);
		glm::quat q = cam_angle_d = click_new * glm::inverse(click_old);
		cam_angle_d = q * cam_angle_d;
		click_old = click_new;
	} else if(PANNING) {
    //on shift and right click
		float xn = click_x - x;
		float yn = -1*(click_y - y);
		float len_sq = xn*xn + yn*yn;
		if (len_sq > 0.1) {
			float len = sqrt(len_sq);
			glm::vec3 add = glm::axis( glm::inverse(cam_angle) * glm::quat(0, xn, yn, 0) * cam_angle );
			focus = focus + add * (len / arcball_radius);
			click_x = x;
			click_y = y;
		}
	} else {
		//if bone selected and right mouse
		rotateOnDrag(x, y);
	}

}

void changeRotAxis() {
	switch (skeleton->currentAxis) {
	case Y:
		skeleton->currentAxis = Z;
		break;
	case Z:
		skeleton->currentAxis = X;
		break;
	case X:
		skeleton->currentAxis = Y;
		break;
	default:
		break;
	}
}

//On keyboard event callback
void G308_keyboardListener(unsigned char key, int x, int y) {

	switch (key) {

		case 'r':
			if(!skeleton->amcPlayerMode) {
				glRotatef(5,0,1,0);
			}
			break;
		case 's':
			savePose();
			break;
		case 'f':
			if(skeleton->amcPlayerMode) {
				skeleton->play();
			}
			break;
		case 'a':
			yRot += 1;
			break;
		case 'd':
			yRot -= 1;
			break;
		//switch axis of rotation
		case 'x':
			changeRotAxis();
			break;
	}

	G308_SetLight();
	glutPostRedisplay();

}
// Reshape function
void G308_Reshape(int w, int h) {
	if (h == 0)
		h = 1;

	g_nWinWidth = w;
	g_nWinHeight = h;

	glViewport(0, 0, g_nWinWidth, g_nWinHeight);
}
// Set Camera Position
void G308_SetCamera() {


//	cam_angle_d = glm::slerp( glm::quat(), cam_angle_d, ( 1 - (float)tick.count() * 10 ) );
	resize(g_nWinWidth, g_nWinHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(G308_FOVY, (double) g_nWinWidth / (double) g_nWinHeight,
	G308_ZNEAR_3D, G308_ZFAR_3D);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glPushMatrix();
	float x = focus.x, y = focus.y, z = focus.z;
	glTranslatef(0.0,0.0,-zoom);

	if(!firstClick) {
		cam_angle = cam_angle_d * cam_angle;
		glMultMatrixf( &glm::mat4_cast(cam_angle)[0][0] );
	}

	gluLookAt(
			x,
			y,
			z,
			0.0f,
			0.0f,
			z - zoom,
			0.0, 1.0, 0.0
			);
}
// Set View Position
void G308_SetLight() {
	float direction[] = { lightX, lightY, lightZ, 1.0f };
	float diffintensity[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, direction);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffintensity);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glEnable(GL_LIGHT0);
}

void displayText( float x, float y, int r, int g, int b, char const* string ) {
	int j = strlen( string );
	glColor3f( 255, 255, 255 );
	glRasterPos2f( x, y );
	for( int i = 0; i < j; i++ ) {
		glutBitmapCharacter( GLUT_BITMAP_HELVETICA_18, string[i] );
	}
}
