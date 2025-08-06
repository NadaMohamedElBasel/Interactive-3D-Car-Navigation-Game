#define _USE_MATH_DEFINES
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 
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

float light1Pos[] = { xVal + 20, 0.0, zVal, 1.0 }; // Spotlight position.
float light2Pos[] = { xVal - 20, 0.0, zVal, 1.0 }; // Spotlight position.
static float spotAngle = 20.0; // Spotlight cone half-angle.
float spotDirection[] = { 0.0, 0.0, -1.0 }; // Spotlight direction.
static float spotExponent = 10.0; // Spotlight attenuation exponent.
static float xMove = 0.0, zMove = 0.0; // Movement components.

GLuint skyTextureID, groundTextureID1, groundTextureID2, groundTextureIDcurrent;
GLuint textureID;


GLuint loadTexture(const char* filename) {
    int width, height, channels;

    // Load the image using stb_image
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return 0; // Return 0 if the texture loading fails
    }

    // Generate a texture ID
    GLuint textureID;
    glGenTextures(1, &textureID);

    // Bind the texture ID
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Determine the format based on the number of channels
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

    // Upload the texture data
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Free the image memory
    stbi_image_free(data);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Return the generated texture ID
    return textureID;
}


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
    glClearColor(0.0, 0.0, 0.0, 0.0);
    int i, j;

    glEnable(GL_TEXTURE_2D); // Enable 2D texturing
    // Load the textures for the sky and ground
    skyTextureID = loadTexture("sky_texture.jpg");
    groundTextureID1 = loadTexture("ground_1_texture.jpg");
    groundTextureID2 = loadTexture("ground_2_texture.jpg");
    groundTextureIDcurrent = groundTextureID1;

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

    // Turn on OpenGL lighting.
    glEnable(GL_LIGHTING);

    // Light property vectors.
    float lightAmb[] = { 0.0, 0.0, 0.0, 1.0 };
    float lightDifAndSpec[] = { 1.0, 1.0, 1.0, 1.0 };
    float globAmb[] = { 0.05, 0.05, 0.05, 1.0 };

    // Light properties.
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDifAndSpec);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightDifAndSpec);
    glEnable(GL_LIGHT0); // Enable particular light source.

    glLightfv(GL_LIGHT1, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lightDifAndSpec);
    glLightfv(GL_LIGHT1, GL_SPECULAR, lightDifAndSpec);
    glEnable(GL_LIGHT1); // Enable particular light source.

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globAmb); // Global ambient light.
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE); // Enable local viewpoint.
    // Material property vectors.
    float matSpec[] = { 1.0, 1.0, 1.0, 1.0 };
    float matShine[] = { 50.0 };

    // Material properties shared by all the spheres.
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
    glMaterialfv(GL_FRONT, GL_SHININESS, matShine);


    // Set up sunlight (directional light representing sunset).

    float sunlightAmb[] = { 0.25, 0.25, 0.25, 1.0 };
    float sunlightDifAndSpec[] = { 1.0, 0.3, 0.0, 1.0 }; // Warm sunset colors (orange).
    glLightfv(GL_LIGHT2, GL_AMBIENT, sunlightAmb);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, sunlightDifAndSpec);
    glLightfv(GL_LIGHT2, GL_SPECULAR, sunlightDifAndSpec);
    glEnable(GL_LIGHT2); // Enable sunlight.

    // Set light position (directional light simulating sunset).
    float sunlightPos[] = { 1.0, -1.0, 0.0, 0.0 }; // Light coming from the horizon.
    glLightfv(GL_LIGHT2, GL_POSITION, sunlightPos);

    // Enable color material mode:
    // The ambient and diffuse color of the front faces will track the color set by glColor().
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
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

void drawWinLoseMessage(const char* message)
{
    // Disable lighting to ensure text is unaffected by lighting
    glDisable(GL_LIGHTING);

    // Set the text color to white (or any visible color)
    glColor3f(1.0, 1.0, 1.0); // White text

    // Position the text at a specific location in 3D space
    glRasterPos3f(0.0, 10.0, -70.0); // Adjust position as needed

    // Render the message
    writeBitmapString((void*)font, (char*)message);

    // Re-enable lighting after rendering the text
    glEnable(GL_LIGHTING);
}

void drawSky() {

    //face 1
    glPushMatrix();
    glEnable(GL_TEXTURE_2D); // Enable texture mapping
    glBindTexture(GL_TEXTURE_2D, skyTextureID); // Bind the sky texture

    glBegin(GL_QUADS);
    // Define vertices and texture coordinates for the sky
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-500.0f, -25.0f, -250.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(500.0f, -25.0f, -250.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(500.0f, 500.0f, -250.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-500.0f, 500.0f, -250.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D); // Disable texture mapping
    glPopMatrix();

    //face2
    glPushMatrix();
    glEnable(GL_TEXTURE_2D); // Enable texture mapping
    glBindTexture(GL_TEXTURE_2D, skyTextureID); // Bind the sky texture

    glBegin(GL_QUADS);
    // Define vertices and texture coordinates for the sky
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-100.0f, -25.0f, 250.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-100.0f, -25.0f, -250.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-100.0f, 500.0f, -250.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-100.0f, 500.0f, 250.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D); // Disable texture mapping
    glPopMatrix();


    //face3
    glPushMatrix();
    glEnable(GL_TEXTURE_2D); // Enable texture mapping
    glBindTexture(GL_TEXTURE_2D, skyTextureID); // Bind the sky texture

    glBegin(GL_QUADS);
    // Define vertices and texture coordinates for the sky
    glTexCoord2f(0.0f, 0.0f); glVertex3f(100.0f, -25.0f, 250.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(100.0f, -25.0f, -250.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(100.0f, 500.0f, -250.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(100.0f, 500.0f, 250.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D); // Disable texture mapping
    glPopMatrix();
}

void drawGround() {
    glPushMatrix();
    glEnable(GL_TEXTURE_2D); // Enable texture mapping
    glBindTexture(GL_TEXTURE_2D, groundTextureIDcurrent); // Bind the selected ground texture

    glBegin(GL_QUADS);
    // Define vertices and texture coordinates for the ground
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-500.0f, -15.0f, 250.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(500.0f, -15.0f, 250.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(500.0f, -15.0f, -250.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-500.0f, -15.0f, -250.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D); // Disable texture mapping
    glPopMatrix();
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
    glColor3f(0.0, 0.0, 0.0);
    glRasterPos3f(-28.0, 25.0, -30.0);

    // Draw win/lose message
    if (isCollision)
    {
        drawWinLoseMessage("You Lose!");
    }
    else if (isWin)
    {
        drawWinLoseMessage("You Win!");
    }

    glPopMatrix();


    // Fixed camera.
    gluLookAt(0.0, 10.0, 20.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

    glPushAttrib(GL_TEXTURE_BIT);  // Save texture states
    glDisable(GL_TEXTURE_2D);  // disable textures temporarily 

    glPushMatrix();

    // Spotlight properties in the camera's perspective.
    light1Pos[0] = xVal + 20;
    light1Pos[2] = zVal;
    light2Pos[0] = xVal - 20;
    light2Pos[2] = zVal;

    spotDirection[0] = -sin((M_PI / 180.0) * angle);
    spotDirection[2] = -cos((M_PI / 180.0) * angle);




    // Spotlight position.
    glLightfv(GL_LIGHT0, GL_POSITION, light1Pos);
    glLightfv(GL_LIGHT1, GL_POSITION, light2Pos);
    // Spotlight properties.
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, spotAngle);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, spotAngle);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spotDirection);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, spotDirection);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, spotExponent);
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, spotExponent);

    glPopMatrix();

    // Draw all the cubes in arraycubes.
    for (j = 0; j < COLUMNS; j++)
        for (i = 0; i < ROWS; i++)
            arrayCubes[i][j].draw();
    drawCar();
    drawGoal();

    glPopAttrib();  // Restore the previous settings
    //glEnable(GL_TEXTURE_2D);

    glPushAttrib(GL_LIGHTING_BIT);  // Save lighting state to avoid accidental change to lighting
    glDisable(GL_LIGHTING);  // Disable lighting for the following textured objects
    // Set the texture environment mode to GL_REPLACE to avoid lighting effects
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    //ADD TEXTURE HERE!
    // 
    // 
    drawGround();
    drawSky();

    glPopAttrib();

    // End left viewport.



    // Begin right viewport.

    glPushAttrib(GL_TEXTURE_BIT);  // Save texture states
    glDisable(GL_TEXTURE_2D);  // disable textures temporarily 

    glViewport(width / 2.0, 0, width / 2.0, height);
    glLoadIdentity();


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

    glPushMatrix();

    // Update spotlight positions and directions based on car's position and orientation.
    light1Pos[0] = xVal + 20.0 + 1.0 * cos((M_PI / 180.0) * angle);
    light1Pos[2] = zVal + 1.0 * sin((M_PI / 180.0) * angle);
    light2Pos[0] = xVal - 20.0 - 1.0 * cos((M_PI / 180.0) * angle);
    light2Pos[2] = zVal - 1.0 * sin((M_PI / 180.0) * angle);

    spotDirection[0] = -sin((M_PI / 180.0) * angle);
    spotDirection[2] = -cos((M_PI / 180.0) * angle);

    // Spotlight position.
    glLightfv(GL_LIGHT0, GL_POSITION, light1Pos);
    glLightfv(GL_LIGHT1, GL_POSITION, light2Pos);
    // Spotlight properties.
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, spotAngle);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, spotAngle);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spotDirection);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, spotDirection);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, spotExponent);
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, spotExponent);

    glPopMatrix();

    // Draw all the cubes in arraycubes.
    for (j = 0; j < COLUMNS; j++)
        for (i = 0; i < ROWS; i++)
            arrayCubes[i][j].draw();

    drawGoal();

    glPopAttrib();  // Restore the previous texture settings
 /*   glEnable(GL_TEXTURE_2D);*/

    //ADD TEXTURE HERE!
    // 
    drawGround();
    drawSky();
    // 
    //drawSky();
    //drawGround();


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
    case 'g': // Toggle between groundTextureID1 and groundTextureID2
        if (groundTextureIDcurrent == groundTextureID1)
            groundTextureIDcurrent = groundTextureID2;
        else
            groundTextureIDcurrent = groundTextureID1;
        glutPostRedisplay(); // Redisplay the scene with the updated texture.
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

    glutInitContextVersion(3, 3);
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
