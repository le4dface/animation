#ifndef FILEIOH
#define FILEIOH

#include <stdio.h>
#include <GL/glut.h>
#include <GL/glut.h>
#include "G308_Skeleton.h"

// Using bitmasking to denote different degrees of freedom for joint motion
typedef int DOF;

#define DOF_NONE 0
#define DOF_RX 1
#define DOF_RY 2
#define DOF_RZ 4
#define DOF_ROOT 8

void trim(char**);

using namespace std;

class FileIO {

private:



public:

	int buffSize, maxBones;
	int numBones;
	int frameCount;
	bone* root;

	FileIO(bone *);
	virtual ~FileIO();

	bool readAMC(char* fn);
	bool readASF(char* fn);
	void decomment(char* buff);
	void readHeading(char* buff, FILE* file);
	void readHierarchy(char* buff, FILE* file);
	void readBone(char* buff, FILE* file);
	DOF dofFromString(char* s);

};

#endif /* FILEIOH */
