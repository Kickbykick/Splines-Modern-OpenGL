#include "common.h"

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <math.h>

/* Coefficients for Matrix M */
#define M11	 0.0
#define M12	 1.0
#define M13	 0.0
#define M14	 0.0
#define M21	-0.5
#define M22	 0.0
#define M23	 0.5
#define M24	 0.0
#define M31	 1.0
#define M32	-2.5
#define M33	 2.0
#define M34	-0.5
#define M41	-0.5
#define M42	 1.5
#define M43	-1.5
#define M44	 0.5

const char *WINDOW_TITLE = "Splines";
const double FRAME_RATE_MS = 1000.0/60.0;
int SCREEN_WIDTH = 640;
int SCREEN_HEIGHT = 640;

float mouseXValue, mouseYValue = 0.0;

typedef glm::vec4  line4;
typedef glm::vec4  point4;

//Points to be drawn
point4 firstPoint = point4(-0.5, -0.5, 0.0, 1.0);
point4 secondPoint = point4(0.0, 0.0, 0.0, 1.0);
point4 thirdPoint = point4(0.5, 0.2, 0.0, 1.0);
point4 fourthPoint = point4(0.7, -0.3, 0.0, 1.0);
point4 newPoint = point4(0.0, 0.0, 0.0, 0.0);

const int NumVertices = 3;
int arrayLength = 4*3;

// Vertices of a unit cube centered at origin, sides aligned with axes
point4 vertices[4];

double tXValue = 0.0;
double tYValue = 0.0;
int currentCurve = 0;

line4 startLine = line4(-0.5, -0.5, 0.0, 1.0);
line4 stopLine = line4(tXValue, tYValue, 0.0, 1.0);
line4 t_value[2] ;

GLuint OurColor, pointVao, lineVao, program, vPosition, lineBuffer, pointBuffer;
bool newPointFlag = false;

//----------------------------------------------------------------------------

// OpenGL initialization
void
init()
{
	// Load shaders and use the resulting shader program
 	program = InitShader("vshader32.glsl", "fshader32.glsl");
	glUseProgram(program);

    // Create a vertex array object
    glGenVertexArrays( 1, &pointVao);
    glBindVertexArray(pointVao);

    // Create and initialize a buffer object
     pointBuffer;
    glGenBuffers( 1, &pointBuffer);
    glBindBuffer( GL_ARRAY_BUFFER, pointBuffer);
    glBufferData( GL_ARRAY_BUFFER, sizeof(vertices),
		  NULL, GL_STATIC_DRAW );

	// set up vertex arrays
	vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	glGenVertexArrays(1, &lineVao);
	glBindVertexArray(lineVao);

	lineBuffer;
	glGenBuffers(1, &lineBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, lineBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(t_value),
		NULL, GL_STATIC_DRAW);

    // set up vertex arrays
     vPosition = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
			   BUFFER_OFFSET(0) );

	OurColor = glGetUniformLocation(program, "OurColor");

    glEnable( GL_DEPTH_TEST );
    glClearColor( 1.0, 1.0, 1.0, 1.0 ); 
}

//----------------------------------------------------------------------------
double catmullRomSpline(float x, float v0, float v1,
	float v2, float v3) {

	double c1, c2, c3, c4;

	c1 = M12 * v1;
	c2 = M21 * v0 + M23 * v2;
	c3 = M31 * v0 + M32 * v1 + M33 * v2 + M34 * v3;
	c4 = M41 * v0 + M42 * v1 + M43 * v2 + M44 * v3;

	return(((c4*x + c3)*x + c2)*x + c1);
}

//----------------------------------------------------------------------------
double bezierCurve(float t, float p0, float p1, float p2, float p3)
{
	double c1, c2, c3, c4;

	c1 = pow(1-t, 3) * p0;
	c2 = 3 * pow(1 - t, 2) * t * p1;
	c3 = 3 * (1 - t) * pow(t, 2) * p2;
	c4 = pow(t, 3) * p3;

	return c1 + c2 + c3 + c4;
}

//----------------------------------------------------------------------------
double bSpline(float t, float p0, float p1, float p2, float p3)
{
	double t2 = t * t;
	double t3 = t2 * t;
	double mt = 1.0 - t;
	double mt3 = mt * mt * mt;

	double bi3 = mt3;
	double bi2 = 3 * t3 - 6 * t2 + 4;
	double bi1 = -3 * t3 + 3 * t2 + 3 * t + 1;
	double bi = t3;

	double result = p0 * bi3 +
					p1 * bi2 +
					p2 * bi1 +
					p3 * bi;
	result /= 6.0;

	return result;
}

//----------------------------------------------------------------------------

void
display( void )
{
	//if(!newPointFlag)
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	point4 vertices[] = { firstPoint,
						secondPoint,
						thirdPoint, 
						fourthPoint};
	glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices),
		vertices);
	glBindVertexArray(pointVao);
	glPointSize(8);
	glDrawArrays(GL_POINTS, 0, 12);
	newPointFlag = false;

	float first = firstPoint.x;
	float second = secondPoint.x;
	float third = thirdPoint.x;
	float fourth = fourthPoint.x;

	float firstY = firstPoint.y;
	float secondY = secondPoint.y;
	float thirdY = thirdPoint.y;
	float fourthY = fourthPoint.y;

	glBindVertexArray(lineVao);
	int loopOver = 0;

	if (currentCurve == 0 ||  currentCurve == 3)
	{
		for (float x = 0; x <= 1; x += 0.01)
		{

			tXValue = bezierCurve(x, first, second, third, fourth);
			tYValue = bezierCurve(x, firstY, secondY, thirdY, fourthY);
			stopLine = line4(tXValue, tYValue, 0.0, 0.0);
			line4 t_value[] = { startLine,
								stopLine };
			
			glBindBuffer(GL_ARRAY_BUFFER, lineBuffer);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(t_value),
				t_value);
			glPointSize(2);
			glDrawArrays(GL_POINTS, 0, 4);
			startLine = stopLine;
		}

	}
	else if (currentCurve == 1)
	{
		for (float x = 0; x <= 1; x += 0.01)
		{

			tXValue = catmullRomSpline(x, first, second, third, fourth);
			tYValue = catmullRomSpline(x, firstY, secondY, thirdY, fourthY);
			stopLine = line4(tXValue, tYValue, 0.0, 0.0);
			line4 t_value[] = { startLine,
								stopLine };
			
			glBindBuffer(GL_ARRAY_BUFFER, lineBuffer);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(t_value),
				t_value);
			glPointSize(2);
			glDrawArrays(GL_POINTS, 0, 4);
			startLine = stopLine;
		}

	}
		else if (currentCurve == 2) 
	{
		loopOver = 0;
		while (loopOver <= 2)
		{
			for (float x = 0; x <= 1; x += 0.01)
			{

				tXValue = bSpline(x, first, second, third, fourth);
				tYValue = bSpline(x, firstY, secondY, thirdY, fourthY);
				stopLine = line4(tXValue, tYValue, 0.0, 0.0);
				line4 t_value[] = { startLine,
									stopLine };

				glBindBuffer(GL_ARRAY_BUFFER, lineBuffer);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(t_value),
					t_value);
				glPointSize(2);
				glDrawArrays(GL_POINTS, 0, 4);
				startLine = stopLine;
			}
			
			loopOver++;
		}

	}

	glutSwapBuffers();
}

//----------------------------------------------------------------------------

void
keyboard( unsigned char key, int x, int y )
{
    switch( key ) {
       case 033: // Escape Key
       case 'q': case 'Q':
	       exit( EXIT_SUCCESS );
          break;
	   case ' ':
			if (currentCurve == 3)
				currentCurve = 0;
			display();
			currentCurve++;

			break;
    }
}

//----------------------------------------------------------------------------

void redraw()
{
	point4 vertices[] = {
	firstPoint,
	secondPoint,
	thirdPoint,
	fourthPoint
	};

	glBindBuffer(GL_ARRAY_BUFFER, pointBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices),
		vertices);
	glBindVertexArray(pointVao);
	glPointSize(8);
	glDrawArrays(GL_POINTS, 0, arrayLength);

}

void
mouse( int button, int state, int x, int y )
{
	mouseXValue = (float)(x - (SCREEN_WIDTH / 2)) * 2 / SCREEN_WIDTH;
	mouseYValue = -(float)(y - (SCREEN_HEIGHT / 2)) * 2 / SCREEN_HEIGHT;

    if ( state == GLUT_DOWN ) {
       switch( button ) {
	       case GLUT_LEFT_BUTTON:  

			   firstPoint = secondPoint;
			   secondPoint = thirdPoint;
			   thirdPoint = fourthPoint;
			   fourthPoint = line4(mouseXValue, mouseYValue, 0.0, 1.0);
			   newPoint = fourthPoint;

			   newPointFlag = true;
			   glutDisplayFunc(redraw);
			   display();

			   break;
       }
    }

}

//----------------------------------------------------------------------------

void
update( void )
{

}

//----------------------------------------------------------------------------

void
reshape( int width, int height )
{
}