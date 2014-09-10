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
		root[i].dirx = 0;
		root[i].diry = 0;
		root[i].dirz = 0;
		root[i].rotx = 0;
		root[i].roty = 0;
		root[i].rotz = 0;
		root[i].dof = DOF_NONE;
		root[i].length = 0;
		root[i].name = NULL;
		root[i].children = (bone**) malloc(sizeof(bone*) * 5);

		//Challenge stuff
		root[i].currentTranslatex = 0;
		root[i].currentTranslatey = 0;
		root[i].currentTranslatez = 0;
		root[i].currentRotationx = 0;
		root[i].currentRotationy = 0;
		root[i].currentRotationz = 0;

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
//	glScalef(0.1, 0.1, 0.1);

    // Begin drawing the floor
	   	unsigned int GridSizeX = 64;
		unsigned int GridSizeZ = 64;
		unsigned int SizeX = 8;
		unsigned int SizeZ = 8;

		if(amcPlayerMode) {

		glPushMatrix();

			glTranslatef(-64*4,0,-64*4);
			glBegin(GL_QUADS);
			for (unsigned int x=0;x<GridSizeX;++x)
				for (unsigned int z=0;z<GridSizeZ;++z)
				{
					if ((x+z) % 2 == 0) //modulo 2
						glColor3f(0.8f,0.8f,0.8f); //white
					else
						glColor3f(0.1f,0.1f,0.1f); //black

					glVertex3f(    x*SizeX,0,    z*SizeZ);
					glVertex3f((x+1)*SizeX,0,    z*SizeZ);
					glVertex3f((x+1)*SizeX,0,(z+1)*SizeZ);
					glVertex3f(    x*SizeX,0,(z+1)*SizeZ);
				}

			glEnd();

		glPopMatrix();
		}


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
	amcFrame++;

	if (amcFrame > frameCount) {
		amcFrame = 0;
	}

}
void Skeleton::pause() {}
void Skeleton::stop() {

	amcFrame = 0;

}
void Skeleton:: rewind() {

	// Move AMC animation one frame forward.
	amcFrame-=4;

	if (amcFrame < 1) {
		amcFrame = 0;
	}

}
void Skeleton::fastforward() {

	// Move AMC animation one frame forward.
	amcFrame+=4;

	if (amcFrame > frameCount) {
		amcFrame = 0;
	}
}


void Skeleton::animationRotation(bone* bone) {

	boneOp b = bone->animationFrame[amcFrame];


	if(bone->dof == 8) {
//		printf("is it a root? %s", bone->name);
		cameraRotation.x = b.rotx;
		cameraRotation.y = b.roty;
		cameraRotation.z = b.rotz;
		cameraTranslation.x = b.tranx;
		cameraTranslation.y = b.trany;
		cameraTranslation.z = b.tranz;
		glTranslatef(b.tranx, b.trany, b.tranz);
	}


	glRotatef(b.rotz, 0.0, 0.0, 1.0);
	glRotatef(b.roty, 0.0, 1.0, 0.0);
	glRotatef(b.rotx, 1.0, 0.0, 0.0);

}


void Skeleton::drawComponent(bone* root, GLUquadric* q) {

	if (root == NULL) {
		return;
	}

	quaternion* quat;
	float quatMatrix[16];
	quat = eulerToQuat(
			root->rotx,
			root->roty,
			root->rotz
			);

	quat->toMatrix(quatMatrix);

	//draw the joint
	glPushMatrix();
	//rotate the socket/joint appropriately

	glColor3f(root->id.r / 255.0f, root->id.g / 255.0f, root->id.b / 255.0f);

	if(selected != NULL) {
		if(strcmp(selected->name,root->name) == 0) {
			glColor3f(1.0f,0.0f,1.0f);
		}
	}

	glutSolidSphere(1.0, 50, 50);

	glPopMatrix();

		//only render the axis that is currently being rotated about
		switch(currentAxis) {

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
	G308_Point v2 = { root->dirx, root->diry, root->dirz }; //bone segment vector
	G308_Point normal = { 0, 0, 0 }; //initialise normal vector
	calcCrossProduct(v1, v2, normal); //find the normal/axis of rotation for v1 and v2
	dotProductAngle(v1, v2, angle); //calculate the angle/distance we must rotate cylinder

	glColor3f(1, 1, 1); //color the bone white, obv.

	if(selected != NULL) {
		if(strcmp(selected->name,root->name) == 0) {
			glColor3f(1.0f,1.0f,0.0f);
		}
	}

	glRotatef(angle, normal.x, normal.y, normal.z); //rotate the bone segment around normal/axis
	gluCylinder(q, 0.3, 0.3, root->length, 3, 3); //draw the cylinder
	glutSolidCone(0.5, 1.0, 3, 3); //and a cone for good measure?

	glPopMatrix();

	//now translate the correctly rotated cylinder
	glTranslatef(root->length * root->dirx, root->length * root->diry,
			root->length * root->dirz);
	//done.

	//apply colourpicking rotations

	boneOp b = root->animationFrame[amcFrame];

	quat = eulerToQuat(
			b.rotx,
			b.roty,
			b.rotz
			);

	quat->toMatrix(quatMatrix);
	glMultMatrixf(quatMatrix);


}

bool Skeleton::readConfig(char* filename) {

	FILE* file = fopen(filename, "r");
	if(file == NULL) {
		printf("Failed to open config file %s\n", filename);
		exit(EXIT_FAILURE);
	}

	char* buff = new char[buffSize];
	printf("reading pose file in\n");

	char* temp = buff;

	char posename[50];
	int frameNumber;

	string delim = " ";

	while(!feof(file)) {
		fgets(buff,buffSize,file);
		temp = buff;
		sscanf(temp, "%d %s", &frameNumber, posename);
//		printf("frame number:%i\nfilename: %s\n", frameNumber, posename);
		readPose(frameNumber, posename);
	}

	delete[] buff;
	fclose(file);
	printf("completed reading\n");
	return true;



}


bool Skeleton::readPose(int framenum, char* filename) {

	FILE* file = fopen(filename,"r");
	if(file == NULL) {
		printf("Failed to open file %s\n",filename);
		exit(EXIT_FAILURE);
	}

	char* buff = new char[buffSize];


	printf("reading pose file in\n");

	char* temp = buff;
	char name[50];
	char transformations[200];
	float x,y,z = 0;
	float tx ,ty, tz = 0;

		for(int i=0; i < 29; i++) {
			fgets(buff,buffSize,file);
			temp = buff;
//			printf("tempee: %s\n", temp);
			sscanf(temp,"%s %[^\n]", name, transformations);
//					printf("temp: %s\n", temp);

			for(int j=0; j<31; j++) {

				if(strncmp(name,root[j].name,strlen(root[j].name)) == 0) {

					if(strcmp("root",root[j].name) == 0) {

						sscanf(transformations, "%f %f %f %f %f %f", &tx, &ty, &tz, &x, &y, &z);

						boneOp* op = new boneOp();

						op->rotx = x;
						op->roty = y;
						op->rotz = z;

						op->tranx = tx;
						op->trany = ty;
						op->tranz = tz;

						root->animationFrame[framenum] = *op;

						root->currentRotationx = x;
						root->currentRotationy = y;
						root->currentRotationz = z;

						root->currentTranslatex = tx;
						root->currentTranslatey = ty;
						root->currentTranslatez = tz;

						break;

					} else {

						sscanf(transformations, "%f %f %f %f %f %f", &tx, &ty, &tz, &x, &y, &z);

						boneOp* op = new boneOp();

						op->rotx = x;
						op->roty = y;
						op->rotz = z;

						op->tranx = tx;
						op->trany = ty;
						op->tranz = tz;
						//TODO was this a problem?
						root[j].animationFrame[framenum] = *op;

						root[j].currentRotationx = x;
						root[j].currentRotationy = y;
						root[j].currentRotationz = z;

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

	for(std::vector<std::string>::iterator itv = v.begin(); itv != v.end(); itv++) {
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
	float rotx = root->currentRotationx;
	float roty = root->currentRotationy;
	float rotz = root->currentRotationz;
	float transx = root->currentTranslatex;
	float transy = root->currentTranslatey;
	float transz = root->currentTranslatez;
	//concat the data to make a string for printing
	char* buffer = (char*) (malloc(sizeof(char) * 5000));
	sprintf(buffer, "%s %f %f %f %f %f %f", result, transx, transy,
			transz, rotx, roty, rotz); // puts string into buffer
	//		printf("%s\n",buffer);
	boneMap.insert(TStrStrPair(result, buffer));
	it = boneMap.find(result);
	printf("HELLO %s\n", it->second.c_str());

}

void Skeleton::readBoneToMap(bone* root) {

	char* result = root->name;
	//the angles
	float rotx = root->currentRotationx + root->rotx;
	float roty = root->currentRotationy + root->roty;
	float rotz = root->currentRotationz + root->rotz;
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
//		printf("not root");
		return NULL;
	}

	if (strcmp(root->name,name)==0) {

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
		//do nothing
		return NULL;
	}
	return findBoneById(root, pixel);

}

bone* Skeleton::findBoneById(bone* root, unsigned char * pixel) {

	int i;
	bone *p;

	if (!root) {
//		printf("not root");
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

//	printf("number of children bones: %d\n", root->numChildren);
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

quaternion* Skeleton::eulerToQuat(float rx, float ry, float rz) {

	rx = degreesToRad(rx);
	ry = degreesToRad(ry);
	rz = degreesToRad(rz);

	float bank,heading,attitude;
	float c1, c2, c3;
	float s1, s2, s3;
	float w, x, y, z;

	bank = rx; heading = ry; attitude = rz;
	c1 = cos(heading/2); c2 = cos(attitude/2); c3 = cos(bank/2);
	s1 = sin(heading/2); s2 = sin(attitude/2); s3 = sin(bank/2);

    w = sqrt(1.0 + c1 * c2 + c1*c3 - s1 * s2 * s3 + c2*c3) / 2.0;
	float w4 = (4.0 * w);
	x = (c2 * s3 + c1 * s3 + s1 * s2 * c3) / w4 ;
	y = (s1 * c2 + s1 * c3 + c1 * s2 * s3) / w4 ;
	z = (-s1 * s3 + c1 * s2 * c3 +s2) / w4 ;

	quaternion* q = new quaternion(w,x,y,z);

	return q;

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

