#define _USE_MATH_DEFINES

#include <cstdlib>
#include <cmath>
#include <iostream>

#include <glew.h>
#include <freeglut.h> 

#define ROWS 8  // Number of rows of cubes.
#define COLUMNS 6 // Number of columns of cubes.
#define FILL_PROBABILITY 100 // Percentage probability that a particular row-column slot will be 
// filled with a cube. It should be an integer between 0 and 100.

// Globals.
static long font = (long)GLUT_BITMAP_8_BY_13; // Font selection.
static int width, height; // Size of the OpenGL window.
static float angle = 0.0; // Angle of the car.
static float xVal = 0, zVal = 0; // Co-ordinates of the car.
static int isCollision = 0; // Is there collision between the car and a cube?
static unsigned int car; // Display lists base index.
static int frameCount = 0; // Number of frames
static int isWin = 0; // Flag to check if the car has reached the goal.


// Routine to draw a bitmap character string.
void writeBitmapString(void* font, char* string)
{
    char* c;

    for (c = string; *c != '\0'; c++) glutBitmapCharacter(font, *c);
}

// Cube class.
class Cube
{
public:
    Cube();
    Cube(float x, float y, float z, float r, unsigned char colorR,
        unsigned char colorG, unsigned char colorB);
    float getCenterX() { return centerX; }
    float getCenterY() { return centerY; }
    float getCenterZ() { return centerZ; }
    float getRadius() { return radius; }
    void draw();

private:
    float centerX, centerY, centerZ, radius;
    unsigned char color[3];
};

// Cube default constructor.
Cube::Cube()
{
    centerX = 0.0;
    centerY = 0.0;
    centerZ = 0.0;
    radius = 0.0; // Indicates no Cube exists in the position.
    color[0] = 0;
    color[1] = 0;
    color[2] = 0;
}

// Cube constructor.
Cube::Cube(float x, float y, float z, float r, unsigned char colorR,
    unsigned char colorG, unsigned char colorB)
{
    centerX = x;
    centerY = y;
    centerZ = z;
    radius = r;
    color[0] = colorR;
    color[1] = colorG;
    color[2] = colorB;
}

// Function to draw a cube.
void Cube::draw()
{
    if (radius > 0.0) // If Cube exists.
    {
        glPushMatrix();
        glTranslatef(centerX, centerY, centerZ); // Position the cube.
        glColor3ubv(color); // Set the color.
        float size = radius * 2; // Use radius as half the cube's size.
        glutSolidCube(size); // Draw a solid cube with edge length equal to size.
        glPopMatrix();
    }
}


Cube arrayCubes[ROWS][COLUMNS]; // Global array of cubes.

// Routine to count the number of frames drawn every second.
void frameCounter(int value)
{
    if (value != 0) // No output the first time frameCounter() is called (from main()).
        std::cout << "FPS = " << frameCount << std::endl;
    frameCount = 0;
    glutTimerFunc(1000, frameCounter, 1);
}


// Initialization routine.
void setup(void)
{
    int i, j;

    car = glGenLists(1);
    glNewList(car, GL_COMPILE);
    glPushMatrix();
    glRotatef(180.0, 0.0, 1.0, 0.0); // To make the car point down the z-axis initially.
    glColor3f(1.0, 1.0, 1.0);
    glutWireCone(5.0, 10.0, 10, 10);
    glPopMatrix();
    glEndList();

    // Initialize global arraycubes.
    for (j = 0; j < COLUMNS; j++)
        for (i = 0; i < ROWS; i++)
            if (rand() % 100 < FILL_PROBABILITY)
            {
                // Position the cubes depending on if there is an even or odd number of columns
                if (COLUMNS % 2) // Odd number of columns.
                    arrayCubes[i][j] = Cube(30.0 * (-COLUMNS / 2 + j), 0.0, -40.0 - 30.0 * i, 3.0,
                        rand() % 256, rand() % 256, rand() % 256);
                else // Even number of columns.
                    arrayCubes[i][j] = Cube(15 + 30.0 * (-COLUMNS / 2 + j), 0.0, -40.0 - 30.0 * i, 3.0,
                        rand() % 256, rand() % 256, rand() % 256);
            }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    glutTimerFunc(0, frameCounter, 0); // Initial call of frameCounter().
}

// Function to check if two spheres centered at (x1,y1,z1) and (x2,y2,z2) with
// radius r1 and r2 intersect.
int checkSpheresIntersection(float x1, float y1, float z1, float r1,
    float x2, float y2, float z2, float r2)
{
    return ((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) + (z1 - z2) * (z1 - z2) <= (r1 + r2) * (r1 + r2));
}

// Function to check if the car collides with a cube when the center of the base
// of the car is at (x, 0, z) and it is aligned at an angle a to to the -z direction.
// Collision detection is approximate as instead of the car we use a bounding sphere.
int cubeCarCollision(float x, float z, float a)
{
    int i, j;

    // Check for collision with each cube.
    for (j = 0; j < COLUMNS; j++)
        for (i = 0; i < ROWS; i++)
            if (arrayCubes[i][j].getRadius() > 0) // If cube exists.
                if (checkSpheresIntersection(x - 5 * sin((M_PI / 180.0) * a), 0.0,
                    z - 5 * cos((M_PI / 180.0) * a), 7.072,
                    arrayCubes[i][j].getCenterX(), arrayCubes[i][j].getCenterY(),
                    arrayCubes[i][j].getCenterZ(), arrayCubes[i][j].getRadius()))
                    return 1;
    return 0;
}

int goalCollision(float x, float z)
{
    // Goal position and size (center at (3.0, 0.0, -100.0) with a radius)
    float goalX = 3.0, goalZ = -95.0, goalRadius = 10.0;

    // Check if the car's bounding sphere intersects the goal
    return ((x - goalX) * (x - goalX) + (z - goalZ) * (z - goalZ) <= goalRadius * goalRadius);
}


void drawGoal(void)
{
    // Draw a white box with a black circle on it
    glPushMatrix();
    glTranslatef(3.0, 0.0, -100.0); // Position the box in the scene
    glScalef(15.0, 15.0, 5.0); // Scale to make it rectangular
    glColor3f(1.0, 1.0, 1.0); // White color for the box
    glutSolidCube(1.0); // Draw the white box

    // Draw the outer orange circle
    glPushMatrix();
    glColor3f(1.0, 0.65, 0.0); // Orange color for the circle
    glTranslatef(0.0, 0.0, 0.51); // Slightly offset the circle to avoid z-fighting
    float outerRadius = 0.4f; // Radius of the outer circle
    int numSegments = 100; // Number of segments to approximate the circle
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0, 0.0, 0.0); // Center of the circle
    for (int i = 0; i <= numSegments; i++)
    {
        float angle = 2.0f * M_PI * i / numSegments;
        float x = outerRadius * cos(angle);
        float y = outerRadius * sin(angle);
        glVertex3f(x, y, 0.0);
    }
    glEnd();
    glPopMatrix();

    // Draw the yellow circle inside the orange circle
    glPushMatrix();
    glColor3f(1.0, 1.0, 0.0); // Yellow color for the inner circle
    glTranslatef(0.0, 0.0, 0.52); // Slightly offset the circle to avoid z-fighting
    float innerRadiusYellow = 0.3f; // Radius of the yellow circle
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0, 0.0, 0.0); // Center of the circle
    for (int i = 0; i <= numSegments; i++)
    {
        float angle = 2.0f * M_PI * i / numSegments;
        float x = innerRadiusYellow * cos(angle);
        float y = innerRadiusYellow * sin(angle);
        glVertex3f(x, y, 0.0);
    }
    glEnd();
    glPopMatrix();

    // Draw the smaller orange circle inside the yellow circle
    glPushMatrix();
    glColor3f(1.0, 0.65, 0.0); // Orange color for the innermost circle
    glTranslatef(0.0, 0.0, 0.53); // Slightly offset the circle to avoid z-fighting
    float innerRadiusOrange = 0.2f; // Radius of the orange circle
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0, 0.0, 0.0); // Center of the circle
    for (int i = 0; i <= numSegments; i++)
    {
        float angle = 2.0f * M_PI * i / numSegments;
        float x = innerRadiusOrange * cos(angle);
        float y = innerRadiusOrange * sin(angle);
        glVertex3f(x, y, 0.0);
    }
    glEnd();
    glPopMatrix();

    glPopMatrix();
}
void drawCar(void)
{
    // Draw car
    glPushMatrix();

    // Position the car
    glTranslatef(xVal, 0.0, zVal);

    // Rotate the car based on angle
    glRotatef(angle, 0.0, 1.0, 0.0);

    // Draw car body
    glPushMatrix();
    glScalef(6.0, 1.5, 11.0); // Scale to make a rectangular body
    glColor3f(0.8, 0.0, 0.0); // Red color for car body
    glutSolidCube(1.0); // Cube representing the car body
    glPopMatrix();

    // Draw car top
    glPushMatrix();
    glTranslatef(0.0, 0.5, 0.0); // Position the top above the body
    glScalef(5.0, 1.5, 8.0); // Scale to make a smaller top
    glColor3f(1.0, 1.0, 1.0); // Dark gray color for the top
    glutSolidCube(1.0); // Cube representing the car top
    glPopMatrix();



    // Draw wheels
    glColor3f(0.7, 0.7, 0.7); // white color for wheels
    for (float xOffset : {-3.2f, 3.2f}) // Two wheels on the x-axis
    {
        for (float zOffset : {-4.0f, 4.0f}) // Two wheels on the z-axis
        {
            glPushMatrix();
            glTranslatef(xOffset, -0.5, zOffset); // Position each wheel
            glRotatef(90, 0.0, 1.0, 0.0); // Rotate to make the torus face outward
            glutSolidTorus(0.4, 0.6, 30, 30); // Torus representing the wheel
            glPopMatrix();
        }
    }

    glPopMatrix();
}

void resetGame(int value)
{
    // Reset car's position and angle.
    xVal = 0.0;
    zVal = 0.0;
    angle = 0.0;

    // Reset collision flag.
    isCollision = 0;
    isWin = 0;

    // Optionally, regenerate cubes or keep the same layout.
    setup();

    glutPostRedisplay();
}



// Drawing routine.
void drawScene(void)
{
    frameCount++; // Increment number of frames every redraw.

    int i, j;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Begin left viewport.
    glViewport(0, 0, width / 2.0, height);
    glLoadIdentity();

    // Write text in isolated (i.e., before gluLookAt) translate block.
    glPushMatrix();
    glColor3f(1.0, 0.0, 0.0);
    glRasterPos3f(-28.0, 25.0, -30.0);
    if (isCollision)
    {
        char message[] = "You Lose!...";
        writeBitmapString((void*)font, message);
    }
    else if (isWin)
    {
        char winMessage[] = "You Win!";
        writeBitmapString((void*)font, winMessage);
    }

    glPopMatrix();


    // Fixed camera.
    gluLookAt(0.0, 10.0, 20.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    // Draw all the cubes in arraycubes.
    for (j = 0; j < COLUMNS; j++)
        for (i = 0; i < ROWS; i++)
            arrayCubes[i][j].draw();
    drawCar();
    drawGoal();
    // End left viewport.

    // Begin right viewport.
    glViewport(width / 2.0, 0, width / 2.0, height);
    glLoadIdentity();

    // Write text in isolated (i.e., before gluLookAt) translate block.
    glPushMatrix();
    glColor3f(1.0, 0.0, 0.0);
    glRasterPos3f(-28.0, 25.0, -30.0);
    char message1[] = "You Lose!...";
    char message2[] = "You Win!";
    if (isCollision) {
        writeBitmapString((void*)font, message1);
    }
    else if (isWin) {
        writeBitmapString((void*)font, message2);
    }
    glPopMatrix();

    // Draw a vertical line on the left of the viewport to separate the two viewports
    glColor3f(1.0, 1.0, 1.0);
    glLineWidth(2.0);
    glBegin(GL_LINES);
    glVertex3f(-5.0, -5.0, -5.0);
    glVertex3f(-5.0, 5.0, -5.0);
    glEnd();
    glLineWidth(1.0);

    // Locate the camera at the tip of the cone and pointing in the direction of the cone.
    gluLookAt(xVal - 10 * sin((M_PI / 180.0) * angle),
        0.0,
        zVal - 10 * cos((M_PI / 180.0) * angle),
        xVal - 11 * sin((M_PI / 180.0) * angle),
        0.0,
        zVal - 11 * cos((M_PI / 180.0) * angle),
        0.0,
        1.0,
        0.0);

    // Draw all the cubes in arraycubes.
    for (j = 0; j < COLUMNS; j++)
        for (i = 0; i < ROWS; i++)
            arrayCubes[i][j].draw();

    drawGoal();
    // End right viewport.

    glutSwapBuffers();
}

// OpenGL window reshape routine.
void resize(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-5.0, 5.0, -5.0, 5.0, 5.0, 250.0);
    glMatrixMode(GL_MODELVIEW);

    // Pass the size of the OpenGL window.
    width = w;
    height = h;
}

// Keyboard input processing routine.
void keyInput(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 27:
        exit(0);
        break;
    default:
        break;
    }
}

// Callback routine for non-ASCII key entry.
void specialKeyInput(int key, int x, int y)
{
    if (isCollision || isWin)
        return; // Block all movement inputs during collision.

    float tempxVal = xVal, tempzVal = zVal, tempAngle = angle;

    switch (key)
    {
    case GLUT_KEY_LEFT: tempxVal = xVal; tempAngle += 5.0; break;
    case GLUT_KEY_RIGHT: tempxVal = xVal; tempAngle -= 5.0; break;
    case GLUT_KEY_UP:
        tempxVal -= sin(angle * M_PI / 180.0);
        tempzVal -= cos(angle * M_PI / 180.0);
        break;
    case GLUT_KEY_DOWN:
        tempxVal += sin(angle * M_PI / 180.0);
        tempzVal += cos(angle * M_PI / 180.0);
        break;
    default: break;
    }

    // Check for collisions and only update position if no collision occurs.
    if (!cubeCarCollision(tempxVal, tempzVal, tempAngle))
    {
        xVal = tempxVal;
        zVal = tempzVal;
        angle = tempAngle;

        if (goalCollision(xVal, zVal))
        {
            isWin = 1; // Set win flag
            glutTimerFunc(3000, resetGame, 0); // Restart game after 3 seconds
        }
    }
    else
    {
        isCollision = 1; // Set collision flag.
        glutTimerFunc(3000, resetGame, 0); // Reset game after 3 seconds.
    }

    glutPostRedisplay();
}


// Routine to output interaction instructions to the C++ window.
void printInteraction(void)
{
    std::cout << "Interaction:" << std::endl;
    std::cout << "Press the left/right arrow keys to turn the Car." << std::endl
        << "Press the up/down arrow keys to move the Car." << std::endl;
}

// Main routine.
int main(int argc, char** argv)
{
    printInteraction();
    glutInit(&argc, argv);

    glutInitContextVersion(4, 3);
    glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 400);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Car Game.cpp");
    glutDisplayFunc(drawScene);
    glutReshapeFunc(resize);
    glutKeyboardFunc(keyInput);
    glutSpecialFunc(specialKeyInput);

    glewExperimental = GL_TRUE;
    glewInit();

    setup();

    glutMainLoop();
}
