//---------------------------------------------------------------------------
/// This software is provided 'as-is' for assignment of COMP308
// in ECS, Victoria University of Wellington,
// without any express or implied warranty.
// In no event will the authors be held liable for any
// damages arising from the use of this software.
//
// The contents of this file may not be copied or duplicated in any form
// without the prior permission of its owner.
//
// Copyright (c) 2012 by Taehyun Rhee
// Edited by Roma Klapaukh, Daniel Atkins, and Taehyun Rhee
//
//---------------------------------------------------------------------------

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <vector>
#include <map>
#include <math.h>
#include "FileIO.h"
#include "G308_Skeleton.h"
#include "quaternion.h"

#include "define.h"

using namespace std;

Skeleton::Skeleton() {

	numBones = 1;
	//color ids for picking; increment after each drawing
	gColorId.r = 0;
	gColorId.g = 220;
	gColorId.b = 220;

	buffSize = 2000000;
	maxBones = 60;
	//have we loaded an amc file?
	amcPlayerMode = false;
	selected = NULL;
	frameCount = 0;
	amcFrame = 0;
	angle = 0;
	root = (bone*) malloc(sizeof(bone) * maxBones);

	for (int i = 0; i < 60; i++) {

		root[i].id = gColorId;
		/*
		 * Increment colorpicker id
		 */
		gColorId.r++;
		if (gColorId.r > 255) {
			gColorId.r = 0;
			gColorId.g++;
			if (gColorId.g > 255) {
				gColorId.g = 0;
				gColorId.b++;
			}
		}
		/*
		 * End colorpicker id increment
		 */
		root[i].numChildren = 0;
		root[i].dir = glm::vec3(0, 0, 0);
		root[i].startQuat = glm::quat(1, 0, 0, 0);
		root[i].dof = DOF_NONE;
		root[i].length = 0;
		root[i].name = NULL;
		root[i].children = (bone**) malloc(sizeof(bone*) * 5);
	}
	char*name = (char*) malloc(sizeof(char) * 5);
	name[0] = 'r';
	name[1] = name[2] = 'o';
	name[3] = 't';
	name[4] = '\0';
	root[0].name = name;
	root[0].dof = DOF_ROOT;
}

Skeleton::~Skeleton() {
	deleteBones(root);
}

void Skeleton::deleteBones(bone* root) {
	for (int i = 0; i < maxBones; i++) {
		if (root[i].name != NULL) {
			free(root[i].name);
		}
		if (root[i].children != NULL) {
			free(root[i].children);
		}
		if (root[i].animationFrame != NULL) {
			free(root[i].animationFrame);
		}
	}
	free(root);
}

// [Assignment2] you may need to revise this function

void Skeleton::display() {
	if (root == NULL) {
		return;
	}
	gColorId = {0, 220, 220};
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	GLUquadric* quad = gluNewQuadric(); //Create a new quadric to allow you to draw cylinders
	if (quad == 0) {
		printf("Not enough memory to allocate space to draw\n");
		exit(EXIT_FAILURE);
	}
	//Actually draw the skeleton
	display(root, quad);
	glPopMatrix();
	gluDeleteQuadric(quad);
}

void Skeleton::play() {

	// Move AMC animation one frame forward.
	amcFrameFloat += 0.01;

}
void Skeleton::pause() {
}
void Skeleton::stop() {
	amcFrameFloat = 0;
}
void Skeleton::rewind() {

	if(amcFrameFloat <= 0) {
		amcFrameFloat = 0;
	} else {
		amcFrameFloat -= 0.09;
	}

}
void Skeleton::fastforward() {

	amcFrameFloat += 0.09;

}

void Skeleton::drawComponent(bone* root, GLUquadric* q) {

	if (root == NULL) {
		return;
	}

	//draw the joint
	glPushMatrix();
	//rotate the socket/joint appropriately
	glColor3f(root->id.r / 255.0f, root->id.g / 255.0f, root->id.b / 255.0f);
	if (selected != NULL) {
		if (strcmp(selected->name, root->name) == 0) {
			glColor3f(1.0f, 0.0f, 1.0f);
		}
	}
	glutSolidSphere(1.0, 50, 50);
	glPopMatrix();
	//only render the axis that is currently being rotated about
	switch (currentAxis) {

	case Y:
		//draw the y-axis arrows
		glPushMatrix();
		glColor3f(1.0, 0.5, 0.0);
		glRotatef(-90, 1, 0, 0);
		gluCylinder(q, 0.1, 0.1, 1.5, 100, 100);
		glTranslatef(0, 0, 1.5);
		glutSolidCone(0.3, 0.8, 100, 100);
		glPopMatrix();
		break;

	case X:
		//draw the x-axis arrows
		glPushMatrix();
		glColor3f(1, 0, 0);
		glRotatef(90, 0, 1, 0);
		gluCylinder(q, 0.1, 0.1, 1.5, 100, 100);
		glTranslatef(0, 0, 1.5);
		glutSolidCone(0.3, 0.8, 100, 100);
		glPopMatrix();
		break;

	case Z:
		//draw the z-axis arrows
		glPushMatrix();
		glColor3f(0, 0, 1);
		glRotatef(90, 0, 0, 1);
		gluCylinder(q, 0.1, 0.1, 1.5, 100, 100);
		glTranslatef(0, 0, 1.5);
		glutSolidCone(0.3, 0.8, 100, 100);
		glPopMatrix();
		break;

	}

	GLfloat angle = 0.0;

	glPushMatrix();

	G308_Point v1 = { 0, 0, 1 }; //by default, bone points towards viewport
	G308_Point v2 = { root->dir.x, root->dir.y, root->dir.z }; //bone segment vector
	G308_Point normal = { 0, 0, 0 }; //initialise normal vector
	calcCrossProduct(v1, v2, normal); //find the normal/axis of rotation for v1 and v2
	dotProductAngle(v1, v2, angle); //calculate the angle/distance we must rotate cylinder

	glColor3f(1, 1, 1); //color the bone white, obv.

	if (selected != NULL) {
		if (strcmp(selected->name, root->name) == 0) {
			glColor3f(1.0f, 1.0f, 0.0f);
		}
	}

	glRotatef(angle, normal.x, normal.y, normal.z); //rotate the bone segment around normal/axis
	gluCylinder(q, 0.3, 0.3, root->length, 3, 3); //draw the cylinder
	glutSolidCone(0.5, 1.0, 3, 3); //and a cone for good measure?

	glPopMatrix();

	//now translate the correctly rotated cylinder
	glTranslatef(root->length * root->dir.x, root->length * root->dir.y,
			root->length * root->dir.z);
	//done.

	//if we have a config file provided that start slerping
	if (amcPlayerMode) {
		int roundedFrame = (int) amcFrameFloat;
		boneOp b = root->animationFrame[roundedFrame % frameCount];
		boneOp c = root->animationFrame[(roundedFrame + 1) % frameCount];
		glm::quat result = glm::slerp(b.startQuat, c.startQuat,
				amcFrameFloat - roundedFrame);
		glMultMatrixf(&glm::mat4_cast(result)[0][0]);
	} else { //else we just want to update the joint rotations
		boneOp b = root->animationFrame[amcFrame];
		glMultMatrixf(&glm::mat4_cast(b.startQuat)[0][0]);
	}

}

bool Skeleton::readConfig(char* filename) {

	FILE* file = fopen(filename, "r");
	if (file == NULL) {
		printf("Failed to open config file %s\n", filename);
		exit(EXIT_FAILURE);
	}

	char* buff = new char[buffSize];
	char* temp = buff;
	char posename[50];
	int frameNumber;
	while (!feof(file)) {
		fgets(buff, buffSize, file);
		temp = buff;
		sscanf(temp, "%d %s", &frameNumber, posename);
		readPose(frameNumber, posename);
	}
	delete[] buff;
	fclose(file);
	printf("completed reading\n");
	return true;

}

glm::quat Skeleton::rotationDataToQuaternion(float x, float y, float z) {
	return glm::quat(
			glm::vec3(degreesToRad(x), degreesToRad(y), degreesToRad(z)));
}

bool Skeleton::readPose(int framenum, char* filename) {

	FILE* file = fopen(filename, "r");
	if (file == NULL) {
		printf("Failed to open file %s\n", filename);
		exit(EXIT_FAILURE);
	}

	char* buff = new char[buffSize];
	printf("reading pose file in\n");

	char* temp = buff;
	char name[50];
	char transformations[200];
	float x, y, z = 0;
	float tx, ty, tz = 0;

	for (int i = 0; i < 29; i++) {
		fgets(buff, buffSize, file);
		temp = buff;
		sscanf(temp, "%s %[^\n]", name, transformations);
		for (int j = 0; j < 31; j++) {

			if (strncmp(name, root[j].name, strlen(root[j].name)) == 0) {

				sscanf(transformations, "%f %f %f %f %f %f", &tx, &ty, &tz, &x,
						&y, &z);
				boneOp* op = new boneOp();
				op->tran = glm::vec3(tx, ty, tz);
				op->startQuat = rotationDataToQuaternion(x, y, z);

				if (strcmp("root", root[j].name) == 0) {
					root->animationFrame[framenum] = *op;
					break;
				} else {
					root[j].animationFrame[framenum] = *op;
					break;
				}
			}
		}
	}
	frameCount++;
	delete[] buff;
	fclose(file);
	printf("completed reading\n");
	return true;

}

void Skeleton::writePoseToFile() {
	vector<string> v;
	v.insert(v.end(), boneNames, boneNames + 29);

	char filename[20];
	printf("Please enter a filename for the pose:\n");
	scanf("%s", filename);
	FILE *fp = fopen(filename, "w");

	for (std::vector<std::string>::iterator itv = v.begin(); itv != v.end();
			itv++) {
		it = boneMap.find(itv->c_str());
		fprintf(fp, "%s\n", it->second.c_str());
	}
	fclose(fp);
}

bone* Skeleton::traverseHierachy() {
	if (root == NULL) {
		//do nothing
		return NULL;
	}
	return traverseHierachy(root);
}

void Skeleton::readRootToMap(bone* root) {
	char* result = root->name;
	//the angles
	glm::vec3 r = glm::eulerAngles(root->animationFrame[amcFrame].startQuat);
	float rotx = r.x;
	float roty = r.y;
	float rotz = r.z;
	float transx = root->animationFrame[amcFrame].tran.x;
	float transy = root->animationFrame[amcFrame].tran.y;
	float transz = root->animationFrame[amcFrame].tran.z;

	//concat the data to make a string for printing
	char* buffer = (char*) (malloc(sizeof(char) * 5000));
	sprintf(buffer, "%s %f %f %f %f %f %f", result, transx, transy, transz,
			rotx, roty, rotz); // puts string into buffer
	//		printf("%s\n",buffer);
	boneMap.insert(TStrStrPair(result, buffer));
	it = boneMap.find(result);
	printf("HELLO %s\n", it->second.c_str());

}

void Skeleton::readBoneToMap(bone* root) {
	char* result = root->name;
	//the angles
	glm::vec3 r = glm::eulerAngles(root->animationFrame[amcFrame].startQuat);
	float rotx = r.x;
	float roty = r.y;
	float rotz = r.z;

	//concat the data to make a string for printing
	char* buffer = (char*) (malloc(sizeof(char) * 5000));
	sprintf(buffer, "%s %f %f %f", result, rotx, roty, rotz); // puts string into buffer
	//		printf("%s\n",buffer);
	boneMap.insert(TStrStrPair(result, buffer));
	it = boneMap.find(result);
	printf("HELLO %s %s\n", it->first.c_str(), it->second.c_str());
}

bone* Skeleton::traverseHierachy(bone* root) {
	int i;
	bone *p;
	if (!root) {
		return NULL;
	}
	readRootToMap(root);
	for (i = 0; i < root->numChildren; i++) {
		p = traverseHierachy(root->children[i]);
		if (p) {
			return p;
		}
	}
	return NULL;
}

bone* Skeleton::findBoneByName(char * name) {
	if (root == NULL) {
		//do nothing
		return NULL;
	}
	return findBoneByName(root, name);
}

bone* Skeleton::findBoneByName(bone* root, char * name) {
	int i;
	bone *p;
	if (!root) {
		return NULL;
	}
	if (strcmp(root->name, name) == 0) {
		return root;
	}
	for (i = 0; i < root->numChildren; i++) {
		p = findBoneByName(root->children[i], name);
		if (p) {
			return p;
		}
	}
	return NULL;
}

bone* Skeleton::findBoneById(unsigned char * pixel) {
	if (root == NULL) {
		return NULL;
	}
	return findBoneById(root, pixel);
}

bone* Skeleton::findBoneById(bone* root, unsigned char * pixel) {
	int i;
	bone *p;
	if (!root) {
		return NULL;
	}
	if ((root->id.r) == (pixel[0]) && (root->id.g) == (pixel[1])
			&& (root->id.b) == (pixel[2])) {
//		printf("id: %s", root->name);
		return root;
	}
	for (i = 0; i < root->numChildren; i++) {
		p = findBoneById(root->children[i], pixel);
		if (p) {
			return p;
		}
	}
	return NULL;
}

// [Assignment2] you need to fill this function
// CORE: MODIFY TO DISPLAY PRIMAN.ASF; READ INTO ARRAY; DRAW WHOLE SKELETON
void Skeleton::display(bone* root, GLUquadric* q) {
	if (root == NULL) {
		return;
	}
	glPushMatrix();
	drawComponent(root, q);
	int i;
	for (i = 0; i < root->numChildren; i++) {
		root->children[i]->parent = root;
		display(root->children[i], q);
	}
	glPopMatrix();
}

void Skeleton::findUV(const G308_Point& v2, const G308_Point& v1,
		const G308_Point& v3, G308_Point& u, G308_Point& v) {
	//U = v2 - v1
	u.x = v2.x - v1.x;
	u.y = v2.y - v1.y;
	u.z = v2.z - v1.z;
	//V = v3 - v1
	v.x = v3.x - v1.x;
	v.y = v3.y - v1.y;
	v.z = v3.z - v1.z;
}

void Skeleton::calcCrossProduct(const G308_Point& u, const G308_Point& v,
		G308_Normal& temp) {
	//calculate cross product of u and v
	temp.x = u.y * v.z - u.z * v.y; //Nx = UyVz - UzVy
	temp.y = u.z * v.x - u.x * v.z; //Ny = UzVx - UxVz
	temp.z = u.x * v.y - u.y * v.x; //Nz = UxVy - UyVx

	//get the magnitude of our normal vector for normalisation
	GLfloat d = sqrt(temp.x * temp.x + temp.y * temp.y + temp.z * temp.z);

	//simply can't divide our normal components by 0
	if (d == 0.0) {
		return;
	}
	//normalise our normal
	temp.x /= d;
	temp.y /= d;
	temp.z /= d;
}

float Skeleton::degreesToRad(float rx) {
	rx = (M_PI * rx) / 180;
	return rx;
}

void Skeleton::dotProductAngle(const G308_Point& v1, const G308_Point& v2,
		float& temp) {
	//v1 · v2 = |v1| × |v2| × cos(θ)
	float dotProdV1V2, magV1, magV2, productMag;
	//calculate dot product of v1 and v2
	dotProdV1V2 = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;

	//calculate the magnitude of v1 and v2
	magV1 = sqrt(v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
	magV2 = sqrt(v2.x * v2.x + v2.y * v2.y + v2.z * v2.z);

	//calculate product of magnitudes
	productMag = magV1 * magV2;
	//calculate cos theta
	float cosTheta = 0;
	cosTheta = dotProdV1V2 / productMag;
	//find the angle (in radians)
	float radians = 0;
	radians = acos(cosTheta);
	//convert to degrees
	temp = radians * (180 / M_PI);
}


