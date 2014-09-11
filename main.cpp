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

//menu options for amc player
enum MENU_ENUM {
	PLAY, PAUSE, STOP, REWIND, FAST_FORWARD
};



MENU_ENUM selected_mode = PAUSE;

char* filename;

bool playing = false;
static int menu_id;
char* selected_name[100];
int right_button_state = 0; //is right button down?

float oldX, oldY; //old mouse x and y position
float oldMagnitude; //old mouse motion magnitude

GLuint g_mainWnd;
GLuint g_nWinWidth = G308_WIN_WIDTH;
GLuint g_nWinHeight = G308_WIN_HEIGHT;

float zoom = 100;

float yRot = 0;
float xRot = 0;

float lightX = 200;
float lightY = 200;
float lightZ = 200;

int arcball_on = false;

G308_Point cameraPosition = {0.0,10.0,20.0};

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


// On mouse click call back, used for selected bones
void onMouse(int button, int state, int x, int y) {

	int mod;
	if(button == 3) { //wheel up
		zoom *= 1.1;
	} else if(button == 4) { //wheel down
		zoom *= 0.9;
	} else if(button == GLUT_LEFT_BUTTON) {

		if (state != GLUT_DOWN) {
			return;
		}

		//check that ctrl has been pressed to select joint
	    mod = glutGetModifiers();
	    if (mod == (GLUT_ACTIVE_CTRL)) {

	    	//turn off texturing, lighting and fog
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
			glDisable(GL_LIGHT0);

			if(skeleton->amcPlayerMode == false) {
				unsigned char pixel[3];
				skeleton->display();
				glEnable(GL_SCISSOR_TEST);
					glScissor(x, y, 1, 1);
					glReadPixels(x, g_nWinHeight - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE,pixel);
					glEnable(GL_LIGHT0);
				glDisable(GL_SCISSOR_TEST);
				//currently selected bone
				if (skeleton->findBoneById(pixel) != NULL) {
					skeleton->selected = skeleton->findBoneById(pixel);
				}
			}
	    }
	}

	if(button == GLUT_RIGHT_BUTTON) {
		right_button_state = 1;
	} else {
		right_button_state = 0;
	}

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

//On mouse motion callback, used for rotating selected joints
void onDrag(int x, int y) {
	//what we really want is the magnitude of the mouse movement
	//previously was check for oldX vs newX
	float magnitude = sqrt(x*x + y*y);

	if(right_button_state == 1) {
		/*
		 * Welcome to terrible C++ programming with reuben!
		 */
		if (skeleton->selected != NULL) {
			if (skeleton->selected->parent != NULL) {
				//if the mouse is generally moving in a positive direction
				boneOp b = getTargetBone();
				float amount = 0.01;
				switch (skeleton->currentAxis) {
					case X:
						rotateBone(magnitude,amount,0,0,b);
						break;
					case Y:
						rotateBone(magnitude,0,amount,0,b);
						break;
					case Z:
						rotateBone(magnitude,0,0,amount,b);
						break;
					default:
						break;
				}

				skeleton->selected->parent->animationFrame[skeleton->amcFrame] = b;
			}
		}
	}

	oldX = x;
	oldY = y;

	//set the reference magnitude for next call of this function
	oldMagnitude = sqrt(oldX*oldX + oldY*oldY);
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
			switch(skeleton->currentAxis) {
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
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(G308_FOVY, (double) g_nWinWidth / (double) g_nWinHeight,
	G308_ZNEAR_3D, G308_ZFAR_3D);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glPushMatrix();
	glTranslatef(0.0,0.0,-zoom);
	glRotatef(skeleton->cameraRotation.y + yRot,0,1,0);
	cameraPosition = skeleton->cameraTranslation;
	gluLookAt(
			cameraPosition.x,
			cameraPosition.y,
			cameraPosition.z,
			cameraPosition.x,
			cameraPosition.y,
			cameraPosition.z - zoom,
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
