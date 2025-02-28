#include <iostream>
#include <SDL.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

const int SCR_W = 640;
const int SCR_H = 480;

float FOV = 90;

int FPS = 360;

SDL_Renderer* renderer = nullptr;
SDL_Window* window = nullptr;

float aspectRatio = float(SCR_W) / float(SCR_H);

float toRadianMultiplier = M_PI / 180.f;

float inverseTan = 1 / (tan(FOV / 2 * toRadianMultiplier));

float zNear = 0.1f;
float zFar = 1000.f;

struct Vector3
{
	float x = 0, y = 0, z = 0;
	Vector3() {}
	Vector3(float _x, float _y, float _z) :x(_x), y(_y), z(_z) {}

	string toString()
	{
		return "(" + to_string(x) + ", " + to_string(y) + ", " + to_string(z) + ")";
	}

	Vector3 operator+(Vector3 second)
	{
		return Vector3(x + second.x, y + second.y, z + second.z);
	}

	void operator+=(Vector3 second)
	{
		x += second.x;
		y += second.y;
		z += second.z;
	}
};

struct Quaternion
{
	float tab[4][4] = { 0 };

	Quaternion() {}

	Vector3 normalMultiply(Vector3 point)
	{
		Vector3 newPoint = Vector3();

		float pointMatrix[4] = { point.x, point.y, point.z, 1 };
		float sums[4] = { 0, 0, 0, 0 };

		for (int i = 0; i < 4; i++)
		{
			for (int y = 0; y < 4; y++)
			{
				sums[i] += tab[y][i] * pointMatrix[y];
			}

		}
		newPoint.x = sums[0];
		newPoint.y = sums[1];
		newPoint.z = sums[2];


		return newPoint;
	}
	Vector3 multiplyPoint(Vector3 point)
	{
		Vector3 newPoint = Vector3();

		float pointMatrix[4] = {point.x, point.y, point.z, 1};
		float sums[4] = { 0, 0, 0, 0};


		for (int i = 0; i < 4; i++)
		{
			for (int y = 0; y < 4; y++)
			{
				sums[i] += tab[y][i]*pointMatrix[y];
			}
		}
	
		//debug
		/*cout << "PM: ";
		for (int i = 0; i < 4; i++)
		{
			cout << pointMatrix[i] << " ";
		}
		cout << endl;
		*/
		if (sums[3] != 0)
		{
			newPoint.x = sums[0] / sums[3];
			newPoint.y = sums[1] / sums[3];
			newPoint.z = sums[2] / sums[3];
		}
	

		return newPoint;


	}

};

Quaternion projectionMatrix, rotationMatrix = Quaternion();




struct Triangle
{
	Vector3 points[3] = {};
	Triangle(){}
	Triangle(Vector3 a, Vector3 b, Vector3 c)
	{
		points[0] = a;
		points[1] = b;
		points[2] = c;
	}
};

struct Mesh
{
	vector<Triangle> triangles = vector<Triangle>();
	float rotationX=0, rotationY=0, rotationZ = 0;

	Vector3 offset = Vector3();

	Mesh(){}
	Mesh(vector<Triangle> _triangles):triangles(_triangles) {}

	Vector3 getMiddle()
	{
		float sumX = 0;
		float sumY = 0;
		float sumZ = 0;

		for (int i = 0; i < triangles.size(); i++)
		{
			for (int j = 0; j < 3; j++)
			{
				sumX += triangles[i].points[j].x;
				sumY += triangles[i].points[j].y;
				sumZ += triangles[i].points[j].z;
			}
			
		}

		return Vector3(sumX / 3 / triangles.size(), sumY / 3 / triangles.size(), sumZ / 3 / triangles.size());
	}
};

Vector3 projectedPoint(Vector3 point)
{
	return projectionMatrix.multiplyPoint(point);
}

float doubledArea(Vector3 a, Vector3 b, Vector3 c)
{
	return abs(a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y));
}

bool insideTriangle(Vector3 point, Triangle triangle)
{
	float full = doubledArea(triangle.points[0], triangle.points[1], triangle.points[2]);

	//0 = a, 1 = b, 2 = c;
	float part1 = doubledArea(triangle.points[0], point, triangle.points[1]);
	float part2 = doubledArea(triangle.points[2], point, triangle.points[1]);
	float part3 = doubledArea(triangle.points[0], point, triangle.points[2]);

	return round(part1 + part2 + part3) == round(full);
}

void fillTriangle(Triangle triangle)
{

	float yMin = triangle.points[0].y, yMax = triangle.points[0].y;
	float xMin = triangle.points[0].x, xMax = triangle.points[0].x;

	for (int i = 1; i < 3; i++)
	{
		if (triangle.points[i].y > yMax)
		{
			yMax = triangle.points[i].y;
		}

		if (triangle.points[i].y < yMin)
		{
			yMin = triangle.points[i].y;
		}

		if (triangle.points[i].x > xMax)
		{
			xMax = triangle.points[i].x;
		}

		if (triangle.points[i].x < xMin)
		{
			xMin = triangle.points[i].x;
		}
	}

	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);


	for (int y = yMin; y < yMax; y++)
	{
		int xStart = -1;
		for (int x = xMin; x < xMax; x++)
		{
			if (insideTriangle(Vector3(x, y, 0), triangle))
			{
				if (xStart == -1)
				{
					xStart = x;
				}
			}
			else if (xStart!=-1)
			{
				SDL_RenderDrawLine(renderer, xStart, y, x, y);
				break;
			}
		}
	}
}

void drawTriangle(Triangle& triangle, Mesh& mesh)
{
	Triangle rotatedTriangle, projectedTriangle;
	
	float sinX = sin(mesh.rotationX * toRadianMultiplier);
	float cosX = cos(mesh.rotationX * toRadianMultiplier);

	float sinY = sin(mesh.rotationY * toRadianMultiplier);
	float cosY = cos(mesh.rotationY * toRadianMultiplier);

	float sinZ = sin(mesh.rotationZ * toRadianMultiplier);
	float cosZ = cos(mesh.rotationZ * toRadianMultiplier);

	rotationMatrix.tab[0][0] = cosY * cosZ;
	rotationMatrix.tab[1][0] = cosY * sinZ;
	rotationMatrix.tab[2][0] = -sinY;

	rotationMatrix.tab[0][1] = -cosX * sinZ + sinX * sinY * cosZ;
	rotationMatrix.tab[1][1] = cosX * cosZ + sinX * sinY * sinZ;
	rotationMatrix.tab[2][1] = sinX * cosY;

	rotationMatrix.tab[0][2] = sinX * sinZ + cosX * sinY * cosZ;
	rotationMatrix.tab[1][2] = -sinX * cosZ + cosX * sinY * sinZ;
	rotationMatrix.tab[2][2] = cosX * cosY;



	for (int i = 0; i < 3; i++)
	{
		rotatedTriangle.points[i] = rotationMatrix.normalMultiply(triangle.points[i]);
		rotatedTriangle.points[i].z += 3;
		rotatedTriangle.points[i] += mesh.offset;
	}

	for (int i = 0; i < 3; i++)
	{
	
		projectedTriangle.points[i] = projectedPoint(rotatedTriangle.points[i]);

		projectedTriangle.points[i].x += 1;
		projectedTriangle.points[i].y += 1;
	

		projectedTriangle.points[i].x *= float(SCR_W)/2.f;
		projectedTriangle.points[i].y *= float(SCR_H)/2.f;

		
	}

	//fillTriangle(projectedTriangle);

	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);


	SDL_RenderDrawLine(renderer,
		projectedTriangle.points[0].x, projectedTriangle.points[0].y,
		projectedTriangle.points[1].x, projectedTriangle.points[1].y
	);

	SDL_RenderDrawLine(renderer,
		projectedTriangle.points[0].x, projectedTriangle.points[0].y,
		projectedTriangle.points[2].x, projectedTriangle.points[2].y
	);

	SDL_RenderDrawLine(renderer,
		projectedTriangle.points[1].x, projectedTriangle.points[1].y,
		projectedTriangle.points[2].x, projectedTriangle.points[2].y
	);

	
}

void drawVisible(vector<Mesh>& meshes)
{
	for (int i = 0; i < meshes.size(); i++)
	{
		for (int j = 0; j < meshes[i].triangles.size(); j++)
		{
			drawTriangle(meshes[i].triangles[j], meshes[i]);
		}
	}
}

void clamp(float& value, float min, float max)
{
	if (value > max)
	{
		value = min;
	}

	if (value < min)
	{
		value = max;
	}

}

void loadMesh(string fileName, vector<Mesh>& meshes)
{
	vector<Vector3> allPoints = vector<Vector3>();

	Mesh thisMesh = Mesh();

	ifstream file;
	string line = "";

	file.open("meshes/" + fileName);

	while (getline(file, line))
	{
		if (line[0] == 'v' && line[1] == ' ')
		{
			//add a new point
			stringstream thisLine(line);
			char temp;
			float x;
			float y;
			float z;

			thisLine >> temp >> x >> y >> z;

			allPoints.push_back(Vector3(x, y, z));

		}
		else if (line[0] == 'f')
		{
			//add a new triangle
			stringstream thisLine(line);
			string temp;
			int ind[3] = { 0 };
			thisLine >> temp >> ind[0] >> temp >> ind[1] >> temp>> ind[2];

			for (int i = 0; i < 3; i++)
			{
				cout << ind[i] << " ";
			}
			cout << endl;

			thisMesh.triangles.push_back(Triangle(allPoints[ind[0]-1], allPoints[ind[1]-1], allPoints[ind[2]-1]));
		}
	}



	meshes.push_back(thisMesh);

	file.close();

}

int main(int argc, char** argv)
{
	projectionMatrix.tab[0][0] = aspectRatio * inverseTan;
	projectionMatrix.tab[1][1] = inverseTan;
	projectionMatrix.tab[2][2] = zFar / (zFar - zNear);
	projectionMatrix.tab[2][3] = 1;
	projectionMatrix.tab[3][2] =  (-1*zFar)*zNear / (zFar - zNear);

	

	vector<Mesh> meshes = vector<Mesh>();
	
	loadMesh("untitled.obj", meshes);


	/*vector<Mesh> meshes = vector<Mesh>(
		{ 
			{
				{
					{ {1.0f, 0.0f, 0.0f},    {0.0f, 1.0f, 0.0f},    {1.0f, 1.0f, 0.0f }},
					{ {0.0f, 0.0f, 0.0f},    {1.0f, 1.0f, 0.0f},    {1.0f, 0.0f, 0.0f} },

					{ {1.0f, 0.0f, 1.0f},    {0.0f, 1.0f, 1.0f},    {1.0f, 1.0f, 1.0f }},
					{ {0.0f, 0.0f, 1.0f},    {1.0f, 1.0f, 1.0f},    {1.0f, 0.0f, 1.0f} }
				}
			}
		}
	);*/

	Mesh* selectedMesh = &(meshes[0]);
	selectedMesh->offset = Vector3(0, 1.2, 2);
	selectedMesh->rotationX = 180;
	selectedMesh->rotationZ = 15;

	window = SDL_CreateWindow("test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCR_W, SCR_H, SDL_RENDERER_ACCELERATED);
	renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED);
	/*if (SDL_CreateWindowAndRenderer(SCR_W, SCR_H, SDL_RENDERER_ACCELERATED, &window, &renderer) < 0)
	{
		return -1;
	}*/

	bool quit = 0;

	SDL_Event e;
	
	while (!quit)
	{
		Uint32 start_time = SDL_GetTicks();

		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
			{
				quit = true;
				break;
			}
		}
		
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		drawVisible(meshes);

		SDL_RenderPresent(renderer);

		selectedMesh->rotationY += 0.1;

		clamp(selectedMesh->rotationX, 0, 360);
		clamp(selectedMesh->rotationY, 0, 360);
		clamp(selectedMesh->rotationZ, 0, 360);
		//selectedMesh->rotationX += 1;
		//clamp(selectedMesh->rotationX, 0, 360);

		if ((1000 / FPS) > SDL_GetTicks() - start_time)
		{
			SDL_Delay((1000 / FPS) - (SDL_GetTicks() - start_time));
		}
		
		cout << "time took: " << (SDL_GetTicks() - start_time)<<endl;
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

