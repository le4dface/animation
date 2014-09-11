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

#ifndef SKELETONH
#define SKELETONH

#include <string>
#include <stdlib.h>
#include <GL/glut.h>
#include "G308_Skeleton.h"
#include "define.h"
#include <math.h>
#include <map>
#include "quaternion.h"


using namespace std;

//Using bitmasking to denote different degrees of freedom for joint motion
typedef int DOF;

#define DOF_NONE 0
#define DOF_RX 1
#define DOF_RY 2
#define DOF_RZ 4
#define DOF_ROOT 8

enum AXIS { X,Y,Z };

//Bone adjustment
typedef struct boneOp {

	float tranx,trany,tranz;
	float rotx,roty,rotz;

} boneOp;

typedef struct colorId {
	int r;
	int g;
	int b;
} colorId;

//Type to represent a bone
typedef struct bone {

	char* name;
	float dirx, diry, dirz;
	float rotx, roty, rotz;
	DOF dof;
	colorId id;
	float length;
	bone* parent;
	bone** children;
	int numChildren;
	boneOp animationFrame[20000];


} bone;

typedef std::map<string, string> TStrStrMap;
typedef std::pair<string, string> TStrStrPair;

void trim(char**);

class Skeleton {

private:

	float angle;

	int buffSize, maxBones;
	float quatMatrix[16];
	quaternion* quat;

	void animationRotation(bone*);
	void deleteBones(bone*);
	void display(bone*, GLUquadric*);
	quaternion* eulerToQuat(float x, float y, float z);
	void dotProductAngle(const G308_Point& v1, const G308_Point& v2, float& temp);
	void calcCrossProduct(const G308_Point& u, const G308_Point& v,G308_Normal& temp);//helper method for calculating vector cross product
	void findUV(const G308_Point& v2, const G308_Point& v1, const G308_Point& v3, G308_Point& u, G308_Point& v);//helper find edges of triangle UV
	DOF dofFromString(char*);
	void drawComponent(bone* root, GLUquadric* q);
	bone* findBoneById(bone* root, unsigned char * pixel);
	bone* findBoneByName(bone* root, char * name);
	bone* traverseHierachy(bone* root);


public:

	//sigh, had to do this for writing the bones to file in correct order...
	const char* boneNames[31] = {
			"root","lowerback","upperback",
			"thorax","lowerneck","upperneck",
			"head","rclavicle","rhipjoint","rhumerus",
			"rradius","rwrist","lhumerus",
			"rfingers","rthumb","lclavicle",
			"lhipjoint","lhumerus","lradius","lwrist",
			"lhand","lfingers","lthumb",
			"rfemur","rtibia","rfoot",
			"rtoes","lfemur", "ltibia",
			"lfoot","ltoes"
	};

	G308_Point cameraRotation = {0,0,0};
	G308_Point cameraTranslation = {0,0,0};
	colorId gColorId;
	bone* root; //tree structure of bones
	bone* selected; //selected bones via colour picking
	AXIS currentAxis = Y;
	bool amcPlayerMode;

	int amcFrame;
	int numBones;
	int frameCount;
	TStrStrMap boneMap;
	TStrStrMap::iterator it;

	void display();


	//glut menu controls, probably need refactoring
	void play();
	void pause();
	void rewind();
	void fastforward();
	void stop();

	void readAMC(FILE*,int*);
	bone* findBoneByName(char *name);
	bone* findBoneById(unsigned char * pixel);
	bone* traverseHierachy();
	void writePoseToFile();
	bool readPose(int framenum, char* filename);
	bool readConfig(char* filename);
	Skeleton();
	~Skeleton();
	void readRootToMap(bone* root);
	void readBoneToMap(bone* root);
	float degreesToRad(float rx);
};

#endif
