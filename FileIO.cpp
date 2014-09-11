/*
 * FileIO.cpp
 *
 *  Created on: 12/08/2014
 *      Author: Reuben Blake Pepperell
 */
#include "FileIO.h"
#include <string.h>
#include <stdlib.h>
#include <GL/glut.h>
#include "G308_Skeleton.h"
#include "define.h"
#include <math.h>
#include "quaternion.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace std;

FileIO::FileIO(bone* rootBone) {

	buffSize = 2000000;
	maxBones = 60;
	frameCount = 0;
	numBones = 1;
	root = rootBone;
}

FileIO::~FileIO() {
 //do nothing really
}

void FileIO::readHeading(char* buff, FILE* file) {
	char* temp = buff;
	decomment(buff);
	trim(&temp);

	char head[50];
	char rest[200];
	int num = sscanf(temp, ":%s %s", head, rest);
	if (num == 0) {
		printf("Could not get heading name from %s, all is lost\n", temp);
		exit(EXIT_FAILURE);
	}
	if (strcmp(head, "version") == 0) {
		//version string - must be 1.10
		char* version = rest;
		if (num != 2) {
			char *p = { '\0' };
			while (strlen(p) == 0) {
				char* p = fgets(buff, buffSize, file);
				decomment(p);
				trim(&p);
				version = p;
			}
		}
		if (strcmp(version, "1.10") != 0) {
			printf("Invalid version: %s, must be 1.10\n", version);
			exit(EXIT_FAILURE);
		}
		//Finished reading version so read the next thing?
	} else if (strcmp(head, "name") == 0) {
		//This allows the skeleton to be called something
		//other than the file name
		//We don't actually care what the name is, so can
		//ignore this whole section

	} else if (strcmp(head, "documentation") == 0) {
		//Documentation section has no meaningful information
		//only of use if you want to copy the file. So we skip it
	} else if (strcmp(head, "units") == 0) {
		//Has factors for the units
		//To be able to model the real person,
		//these must be parsed correctly
		//Only really need to check if deg or rad, but even
		//that is probably not needed for the core or extension
	} else if (strcmp(head, "root") == 0) {
		//Read in information about root
		//Or be lazy and just assume it is going to be the normal CMU thing!
	} else if (strcmp(head, "bonedata") == 0) {
		//Description of each bone
		//This does need to actually be read :(
		char *p;
		while ((p = fgets(buff, buffSize, file)) != NULL) {
			decomment(p);
			trim(&p);
			if (strlen(p) > 0) {
				if (p[0] == ':') {
					return readHeading(buff, file);
				} else if (strcmp(p, "begin") != 0) {
					printf("Expected begin for bone data %d, found \"%s\"", numBones, p);
					exit(EXIT_FAILURE);
				} else {
					readBone(buff, file);
					numBones++;
				}

			}
		}
	} else if (strcmp(head, "hierarchy") == 0) {
		//Description of how the bones fit together
		char *p;
		while ((p = fgets(buff, buffSize, file)) != NULL) {
			trim(&p);
			decomment(p);
			if (strlen(p) > 0) {
				if (p[0] == ':') {
					return readHeading(buff, file);
				} else if (strcmp(p, "begin") != 0) {
					printf("Expected begin in hierarchy, found %s", p);
					exit(EXIT_FAILURE);
				} else {
					readHierarchy(buff, file);
				}

			}
		}
	} else {
		printf("Unknown heading %s\n", head);
	}

}

void FileIO::readHierarchy(char* buff, FILE* file) {
	char *p;
	char t1[200];
	while ((p = fgets(buff, buffSize, file)) != NULL) {
		trim(&p);
		decomment(p);
		if (strlen(p) > 0) {
			if (strcmp(p, "end") == 0) {
				//end of hierarchy
				return;
			} else {
				//Read the root node
				sscanf(p, "%s ", t1);
				bone* rootBone = NULL;
//				printf("Num Bones: %d", numBones );
				for (int i = 0; i < numBones; i++) {
					if (strcmp(root[i].name, t1) == 0) {
						rootBone = root + i; //get the next bone in the bone array
						break;
					}
				}
				//Read the connections
				p += strlen(t1);
				bone* other = NULL;

				while (*p != '\0') {
					sscanf(p, "%s ", t1);

					for (int i = 0; i < numBones; i++) {
						if (strcmp(root[i].name, t1) == 0) {
							other = root + i;
							//printf("other: %s,%s\n ",root[i].name, t1);
							break;
						}
					}
					if (other == NULL) {
						printf("Unknown bone %s found in hierarchy. Failure", t1);
						exit(EXIT_FAILURE);
					}
					rootBone->children[rootBone->numChildren] = other;
					rootBone->numChildren++;
					p += strlen(t1) + 1;

				}
			}
		}

	}
}

void FileIO::readBone(char* buff, FILE* file) {
	char *p;
	char t1[50];
	while ((p = fgets(buff, buffSize, file)) != NULL) {
		trim(&p);
		decomment(p);
		if (strlen(p) > 0) {
			if (strcmp(p, "end") == 0) {
				//end of this bone
				return;
			} else {
				sscanf(p, "%s ", t1);
				if (strcmp(t1, "name") == 0) {
					//Read the name and actually remember it
					char* name = (char*) malloc(sizeof(char) * 10);
					sscanf(p, "name %s", name);
					root[numBones].name = name;
					//print the bones being read in
					//printf("reading bone name %s\n", name);
				} else if (strcmp(t1, "direction") == 0) {
					//Also actually important
					float x, y, z;
					sscanf(p, "direction %f %f %f", &x, &y, &z);
					root[numBones].dir = glm::vec3(x, y, z);
					//printf("reading the bone vector: x = %f, y = %f, z = %f\n", x,y,z);

				} else if (strcmp(t1, "length") == 0) {
					//Also actually important
					float len;
					sscanf(p, "length %f", &len);
					root[numBones].length = len;
					//read the bone length
					//printf("Bone length: %f\n", len);
				} else if (strcmp(t1, "dof") == 0) {
					//Read the degrees of freedom!
					char d1[5];
					char d2[5];
					char d3[5];
					int num = sscanf(p, "dof %s %s %s", d1, d2, d3);

					//printf("Degrees of freedom: %s, %s, %s\n", d1,d2,d3);
					switch (num) {
					DOF dof;
				case 3:
					dof = dofFromString(d3);
					root[numBones].dof = root[numBones].dof | dof; //bitwise OR
					//printf("degree of freedom d3: %d\n", root[numBones].dof);
					//fallthrough!!
					/* no break */
				case 2:
					dof = dofFromString(d2);
					root[numBones].dof = root[numBones].dof | dof;
					//printf("degree of freedom d2: %d\n", root[numBones].dof);
					//fallthrough!!
					/* no break */
				case 1:
					dof = dofFromString(d1);
					root[numBones].dof = root[numBones].dof | dof;
					//printf("degree of freedom d1: %d\n", root[numBones].dof);
					break;
					}
				} else if (strcmp(t1, "axis") == 0) {
					//Read the rotation axis
					float x, y, z;
					int num = sscanf(p, "axis %f %f %f XYZ", &x, &y, &z);
					if (num != 3) {
						//printf("axis format doesn't match expected\n");
						//printf("Expected: axis %%f %%f %%f XYZ\n");
						printf("Got: %s", p);
						exit(EXIT_FAILURE);
					}

					//root[numBones].startQuat = glm::quat(glm::vec3(x, y, z));
					//root[numBones].startQuat = eulerToQuat(x,y,z);
					//root[numBones].endQuat = eulerToQuat(x,y,z);

					root[numBones].animationFrame[0].startQuat = glm::quat(1, 0, 0, 0);
					//root[numBones].animationFrame[0].endQuat = eulerToQuat(x,y,z);

					//read rotation axis
					//printf("Rotation Axis: %f, %f, %f\n", x,y,z);
				}
				//There are more things but they are not needed for the core
			}

		}
	}
}

float FileIO::degreesToRad(float rx) {
	rx = (M_PI * rx) / 180;
	return rx;
}

quaternion* FileIO::eulerToQuat(float rx, float ry, float rz) {

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



bool FileIO::readAMC(char* filename) {

	//scan lines based on DOF
	FILE* file = fopen(filename,"r");
	if(file == NULL) {
		printf("Failed to open file %s\n",filename);
		exit(EXIT_FAILURE);
	}

	char* buff = new char[buffSize];

	printf("reading motion capture file in\n");

	if(fgets(buff,buffSize,file)) {
		while(strchr(buff, '#')) {
			fgets(buff, buffSize, file);
			char* temp = buff;
			char head[50];
			char rest[200];
			char name[50];
			char transformations[200];
			float x,y,z = 0;
			float tx ,ty, tz = 0;

			sscanf(temp, ":%s %s", head, rest);

			if(strcmp(head,"FULLY-SPECIFIED") == 0) {
//				printf("here!\n");
				fgets(buff,buffSize,file);
//				printf("buffer %s: ", buff);

				while(fgets(buff,buffSize, file) != NULL) {
					sscanf(buff,"%d", &frameCount);
					for(int i=0; i < 29; i++) {
						fgets(buff,buffSize,file);
						temp = buff;
						sscanf(temp,"%s %[^\n]", name, transformations);
						for(int j=0; j<numBones; j++) {


							if(strncmp(name,root[j].name,strlen(root[j].name)) == 0) {
								int hasX,hasY,hasZ;
								hasX = root[j].dof & DOF_RX;
								hasY = root[j].dof & DOF_RY;
								hasZ = root[j].dof & DOF_RZ;

								if(hasX && hasY && hasZ) {
									sscanf(transformations, "%f %f %f", &x, &y, &z);
//									printf("has x,y,z: %f %f %f\n",x,y,z);
									boneOp* op = new boneOp();
									op->startQuat = glm::quat(glm::vec3(degreesToRad(x), degreesToRad(y), degreesToRad(z)));

//									printf("frame count: %d", frameCount);
									root[j].animationFrame[frameCount] = *op;
									break;
								} else if (hasX && hasY) {
									sscanf(transformations, "%f %f", &x, &y);
//									printf("has x,y: %f %f\n",x,y);
									boneOp* op = new boneOp();
									op->startQuat = glm::quat(glm::vec3(degreesToRad(x), degreesToRad(y), 0));

									root[j].animationFrame[frameCount] = *op;
									break;
								} else if (hasX && hasZ) {
									sscanf(transformations, "%f %f", &x, &z);
//									printf("has x,z: %f %f\n",x,z);
									boneOp* op = new boneOp();
									op->startQuat = glm::quat(glm::vec3(degreesToRad(x), 0, degreesToRad(z)));
									root[j].animationFrame[frameCount] = *op;
									break;
								} else if(hasX) {
									sscanf(transformations, "%f", &x);
//									printf("has x: %f\n",x);
									boneOp* op = new boneOp();
									op->startQuat = glm::quat(glm::vec3(degreesToRad(x),0,0));

									root[j].animationFrame[frameCount] = *op;
									break;
								} else if(hasY) {
									sscanf(transformations, "%f", &y);
//									printf("has y: %f\n",y);
									boneOp* op = new boneOp();
									op->startQuat = glm::quat(glm::vec3(0, degreesToRad(y), 0));
									root[j].animationFrame[frameCount] = *op;
									break;
								} else if(hasZ) {
									sscanf(transformations, "%f", &z);
//									printf("has z: %f\n",z);
									boneOp* op = new boneOp();
									op->startQuat = glm::quat(glm::vec3(0, 0, degreesToRad(z)));
									root[j].animationFrame[frameCount] = *op;
									break;
								} else if(root[j].dof == 8) {

									sscanf(transformations, "%f %f %f %f %f %f", &tx, &ty, &tz, &x, &y, &z);

									boneOp* op = new boneOp();
									op->tran = glm::vec3(tx, ty, tz);
									op->startQuat = glm::quat(glm::vec3(degreesToRad(x), degreesToRad(y), degreesToRad(z)));

									root->animationFrame[frameCount] = *op;

//									root->currentTranslatex = tx;
//									root->currentTranslatey = tx;
//									root->currentTranslatez = tz;
//
//									root->currentRotationx = x;
//									root->currentRotationy = y;
//									root->currentRotationz = z;


									break;

								}
							}
						}
					}
				}
			}
		}
	}

	delete[] buff;
	fclose(file);
	printf("completed reading\n");
	return true;

}

bool FileIO::readASF(char* filename) {
	FILE* file = fopen(filename, "r");
	if (file == NULL) {
		printf("Failed to open file %s\n", filename);
		exit(EXIT_FAILURE);
	}

	char* buff = new char[buffSize];
	char *p;

	printf("Starting reading skeleton file\n");
	while ((p = fgets(buff, buffSize, file)) != NULL) {
		//Reading actually happened!

		char* start = buff;
		trim(&start);

		//Check if it is a comment or just empty
		if (buff[0] == '#' || buff[0] == '\0') {
			continue;
		}

		start = strchr(buff, ':');
		if (start != NULL) {
			//This actually is a heading
			//Reading may actually consume the next heading
			//so headings need to be a recursive construct?
			readHeading(buff, file);
		}
	}

	delete[] buff;
	fclose(file);
	printf("Completed reading skeleton file\n");
	return true;
}

/**
 * Trim the current string, by adding a null character into where the comments start
 */
void FileIO::decomment(char * buff) {
	char* comStart = strchr(buff, '#');
	if (comStart != NULL) {
		//remove irrelevant part of string
		*comStart = '\0';
	}
}

/*
 * Remove leading and trailing whitespace. Increments the
 * pointer until it points to a non whitespace char
 * and then adds nulls to the end until it has no
 * whitespace on the end
 */
void trim(char **p) {
	if (p == NULL) {
		printf("File terminated without version number!\n");
		exit(EXIT_FAILURE);
	}

	//Remove leading whitespace
	while (**p == ' ' || **p == '\t') {
		(*p)++;
	}

	int len = strlen(*p);
	while (len > 0) {
		char last = (*p)[len - 1];
		if (last == '\r' || last == '\n' || last == ' ' || last == '\t') {
			(*p)[--len] = '\0';
		} else {
			return;
		}
	}
}

/**
 * Helper function to turn a DOF from the AMC file into the correct DOF value
 */
DOF FileIO::dofFromString(char* s) {
	if (strcmp(s, "rx") == 0)
		return DOF_RX;
	if (strcmp(s, "ry") == 0)
		return DOF_RY;
	if (strcmp(s, "rz") == 0)
		return DOF_RZ;
	printf("Unknown DOF found: %s", s);
	return DOF_NONE;
}





