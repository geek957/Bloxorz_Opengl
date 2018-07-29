#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <string.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assert.h>
#include <ao/ao.h>
#include <cstring>
#include <string>
#include <stdlib.h>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
    glm::mat4 projection;
    glm::mat4 model;
    glm::mat4 view;
    GLuint MatrixID;
} Matrices;



struct WavHeader {
    char id[4]; //should contain RIFF
    int32_t totalLength;
    char wavefmt[8];
    int32_t format; // 16 for PCM
    int16_t pcm; // 1 for PCM
    int16_t channels;
    int32_t frequency;
    int32_t bytesPerSecond;
    int16_t bytesByCapture;
    int16_t bitsPerSample;
    char data[4]; // "data"
    int32_t bytesInData;
};

GLFWwindow* window; // window desciptor/handle


static const int BUF_SIZE = 4096;

GLuint programID;
double last_update_time, current_time;
glm::vec3 rect_pos, floor_pos,eye_pos,block_view_pos,heli_block_view_pos;
float rectangle_rotation = 0;
glm::vec3 up;
glm::vec3 target,target_blk_view_pos,focus_block_view_pos;

float teta=45.0,phi=0.0,rad=5.0;
int level=1;
int score=0;
bool level_up=false,flag_exit=false,flag_left=false;

int gameover_sound=0,block_sound=0;

bool press_t=false,press_w=false,press_h=false,press_f=false,press_b=false,press_r=false;
bool transition=false,left_key=false,right_key=false,up_key=false,down_key=false,gameover=false;
bool right1_key=false,left1_key=false,up1_key=false,down1_key=false;


int orientation=0;
float rotation_angle=0;

glm::vec3 block_pos,rotation_axis,base_pos;

glm::mat4 History = glm::mat4(1.0f);

vector <pair<float,float> > block_centers;
vector <pair<float,float> > spl_block_centers;


float block_x=0,block_y=0;

float bridge_3_x[4]={0,-1.5,3.0,-2.5};
float bridge_3_y[4]={0,-1,2.0,1.0};
int bridge_3[4]={0,-1,-1,-1};

float bridge_2_x[2]={0,1.0};
float bridge_2_y[2]={0,0};
int bridge_2[2]={0,-1};
double prvxpos;


void check_bridges();


void getprvxpos()
{
    double xpos,ypos;
    glfwGetCursorPos(window,&xpos,&ypos);
    prvxpos=xpos;
}

char* itoa(int i)
{
    char const digit[] = "0123456789";
    static char b[100];
    char* p=b;
    if(i<0){
        *p++ = '-';
        i *= -1;
    }
    int shifter = i;
    do{ 
        ++p;
        shifter = shifter/10;
    }while(shifter);
    *p = '\0';
    do{ 
        *--p = digit[i%10];
        i = i/10;
    }while(i);
    return b;
}


double area(double x1, double y1, double x2, double y2,double  x3, double y3)
{
   return fabs((x1*(y2-y3) + x2*(y3-y1)+ x3*(y1-y2))/2.0);
}
 
int isInside(double x1, double y1, double x2, double y2, double x3, double y3, double x,double y)
{   
   double A = area (x1, y1, x2, y2, x3, y3);
   double A1 = area (x, y, x2, y2, x3, y3);
   double A2 = area (x1, y1, x, y, x3, y3);
   double A3 = area (x1, y1, x2, y2, x, y);
   if(A < A1 + A2 + A3)
   	return -1;
   else 
   	return 1;
}
/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open())
    {
        std::string Line = "";
        while(getline(VertexShaderStream, Line))
            VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
        std::string Line = "";
        while(getline(FragmentShaderStream, Line))
            FragmentShaderCode += "\n" + Line;
        FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    //    printf("Compiling shader : %s\n", vertex_file_path);
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> VertexShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    //    fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

    // Compile Fragment Shader
    //    printf("Compiling shader : %s\n", fragment_file_path);
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
    //    fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

    // Link the program
    //    fprintf(stdout, "Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    //    fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

void initGLEW(void){
    glewExperimental = GL_TRUE;
    if(glewInit()!=GLEW_OK){
        fprintf(stderr,"Glew failed to initialize : %s\n", glewGetErrorString(glewInit()));
    }
    if(!GLEW_VERSION_3_3)
        fprintf(stderr, "3.3 version not available\n");
}



/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
            0,                  // attribute 0. Vertices
            3,                  // size (x,y,z)
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
            );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
            1,                  // attribute 1. Color
            3,                  // size (r,g,b)
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
            );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/


/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE && transition==false) {
        switch (key) {
            default:
                break;
        }
    }
    else if (action == GLFW_PRESS && transition==false) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            case GLFW_KEY_T:
                {
                    eye_pos.x=0;eye_pos.y=0;eye_pos.z=5;
                    press_f=false;
                    press_b=false;
                    press_h=false;
                    press_w=false;
                    break;
                }
            case GLFW_KEY_W:
                {
                    eye_pos.x=-2;eye_pos.y=-2;eye_pos.z=5;
                    press_b=false;
                    press_f=false;
                    press_h=false;press_w=true;
                    break;
                }
            case GLFW_KEY_B:
                {
                    press_f=false;
                    press_b=true;
                    press_h=false;press_w=false;
                    break;
                }
            case GLFW_KEY_F:
                {
                    press_f=true;
                    press_b=false;
                    press_h=false;press_w=false;
                    break;
                }
            case GLFW_KEY_H:
                {
                    press_b=false;
                    press_f=false;
                    press_h=true;press_w=false;
                    break;
                }
            case GLFW_KEY_R:
            {
            	press_r=true;
            	break;
            }
            case GLFW_KEY_RIGHT:
                {
                    score++;
                    right_key=true;
                    transition=true;
                    if(orientation==0)
                    {
                        rotation_axis=glm::vec3 (block_pos.x+0.25,block_pos.y,block_pos.z-0.5);
                        block_pos.x+=0.75;block_pos.z-=0.25;orientation=1;
                        block_x+=0.75;
                        block_view_pos.x=block_pos.x+0.5;target_blk_view_pos.x=block_view_pos.x+2.0;
                        block_view_pos.y=block_pos.y;target_blk_view_pos.y=block_pos.y;
                        focus_block_view_pos.x=block_pos.x-1.25;focus_block_view_pos.y=block_pos.y;
                    }
                    else if(orientation==1)
                    {
                        rotation_axis=glm::vec3 (block_pos.x+0.5,block_pos.y,block_pos.z-0.25);
                        block_pos.x+=0.75;block_pos.z+=0.25;orientation=0;
                        block_x+=0.75;
                        block_view_pos.x=block_pos.x+0.25;target_blk_view_pos.x=block_view_pos.x+2.0;
                        block_view_pos.y=block_pos.y;target_blk_view_pos.y=block_pos.y;
                        focus_block_view_pos.x=block_pos.x-0.5;focus_block_view_pos.y=block_pos.y;
                    }
                    else if(orientation==2)
                    {
                        rotation_axis=glm::vec3 (block_pos.x+0.25,block_pos.y,block_pos.z-0.25);
                        block_pos.x+=0.5;orientation=2;
                        block_x+=0.5;
                        block_view_pos.x=block_pos.x+0.25;target_blk_view_pos.x=block_view_pos.x+2.0;
                        block_view_pos.y=block_pos.y;target_blk_view_pos.y=block_pos.y;
                        focus_block_view_pos.x=block_pos.x-0.5;focus_block_view_pos.y=block_pos.y;
                    }
                    check_bridges();
                    break;
                }
            case GLFW_KEY_LEFT:
                {
                    score++;
                    left_key=true;
                    transition=true;
                    if(orientation==0)
                    {
                        rotation_axis=glm::vec3 (block_pos.x-0.25,block_pos.y,block_pos.z-0.5);
                        block_x-=0.75;
                        block_pos.x-=0.75;block_pos.z-=0.25;orientation=1;
                        block_view_pos.x=block_pos.x-0.5;target_blk_view_pos.x=block_view_pos.x-2.0;
                        block_view_pos.y=block_pos.y;target_blk_view_pos.y=block_pos.y;
                        focus_block_view_pos.x=block_pos.x+1.5;focus_block_view_pos.y=block_pos.y;
                    }
                    else if(orientation==1)
                    {
                        rotation_axis=glm::vec3 (block_pos.x-0.5,block_pos.y,block_pos.z-0.25);
                        block_x-=0.75;
                        block_pos.x-=0.75;block_pos.z+=0.25;orientation=0;
                        block_view_pos.x=block_pos.x-0.25;target_blk_view_pos.x=block_view_pos.x-2.0;
                        block_view_pos.y=block_pos.y;target_blk_view_pos.y=block_pos.y;
                        focus_block_view_pos.x=block_pos.x+0.5;focus_block_view_pos.y=block_pos.y;
                    }
                    else if(orientation==2)
                    {
                        rotation_axis=glm::vec3 (block_pos.x-0.25,block_pos.y,block_pos.z-0.25);
                        block_x-=0.5;
                        block_pos.x-=0.5;orientation=2;
                        block_view_pos.x=block_pos.x-0.25;target_blk_view_pos.x=block_view_pos.x-2.0;
                        block_view_pos.y=block_pos.y;target_blk_view_pos.y=block_pos.y;
                        focus_block_view_pos.x=block_pos.x+0.5;focus_block_view_pos.y=block_pos.y;
                    }
                    check_bridges();
                    break;
                }
            case GLFW_KEY_UP:
                {
                    score++;
                    transition=true;
                    up_key=true;
                    if(orientation==0)
                    {
                        rotation_axis=glm::vec3 (block_pos.x,block_pos.y+0.25,block_pos.z-0.5);
                        block_pos.y+=0.75;block_pos.z-=0.25;orientation=2;
                        block_y+=0.75;
                        block_view_pos.y=block_pos.y+0.5;target_blk_view_pos.y=block_view_pos.y+2.0;
                        block_view_pos.x=block_pos.x;target_blk_view_pos.x=block_pos.x;
                        focus_block_view_pos.y=block_pos.y-1.5;focus_block_view_pos.x=block_pos.x;
                    }
                    else if(orientation==1)
                    {
                        rotation_axis=glm::vec3 (block_pos.x,block_pos.y+0.25,block_pos.z-0.25);
                        block_pos.y+=0.5;orientation=1;
                        block_y+=0.5;
                        block_view_pos.y=block_pos.y+0.25;target_blk_view_pos.y=block_view_pos.y+2.0;
                        block_view_pos.x=block_pos.x;target_blk_view_pos.x=block_pos.x;
                        focus_block_view_pos.y=block_pos.y-0.5;focus_block_view_pos.x=block_pos.x;
                    }
                    else if(orientation==2)
                    {
                        rotation_axis=glm::vec3 (block_pos.x,block_pos.y+0.5,block_pos.z-0.25);
                        block_pos.y+=0.75;block_pos.z+=0.25;orientation=0;
                        block_y+=0.75;
                        block_view_pos.y=block_pos.y+0.25;target_blk_view_pos.y=block_view_pos.y+2.0;
                        block_view_pos.x=block_pos.x;target_blk_view_pos.x=block_pos.x;
                        focus_block_view_pos.y=block_pos.y-0.5;focus_block_view_pos.x=block_pos.x;
                    }
                    check_bridges();
                    break;
                }
            case GLFW_KEY_DOWN:
                {
                    score++;
                    transition=true;
                    down_key=true;
                    if(orientation==0)
                    {
                        rotation_axis=glm::vec3 (block_pos.x,block_pos.y-0.25,block_pos.z-0.5);
                        block_pos.y-=0.75;block_pos.z-=0.25;orientation=2;
                        block_y-=0.75;
                        block_view_pos.y=block_pos.y-0.5;target_blk_view_pos.y=block_view_pos.y-2.0;
                        block_view_pos.x=block_pos.x;target_blk_view_pos.x=block_pos.x;
                        focus_block_view_pos.y=block_pos.y+1.5;focus_block_view_pos.x=block_pos.x;
                    }
                    else if(orientation==1)
                    {
                        rotation_axis=glm::vec3 (block_pos.x,block_pos.y-0.25,block_pos.z-0.25);
                        block_pos.y-=0.5;orientation=1;
                        block_y-=0.5;
                        block_view_pos.y=block_pos.y-0.25;target_blk_view_pos.y=block_view_pos.y-2.0;
                        target_blk_view_pos.x=block_pos.x;block_view_pos.x=block_pos.x;
                        focus_block_view_pos.y=block_pos.y+0.5;focus_block_view_pos.x=block_pos.x;
                    }
                    else if(orientation==2)
                    {
                        rotation_axis=glm::vec3 (block_pos.x,block_pos.y-0.5,block_pos.z-0.25);
                        block_pos.y-=0.75;block_pos.z+=0.25;orientation=0;
                        block_y-=0.75;
                        block_view_pos.y=block_pos.y-0.25;target_blk_view_pos.y=block_view_pos.y-2.0;
                        block_view_pos.x=block_pos.x;target_blk_view_pos.x=block_pos.x;
                        focus_block_view_pos.y=block_pos.y+0.5;focus_block_view_pos.x=block_pos.x;
                    }
                    check_bridges();
                    break;
                }
            default:
                break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
    switch (key) {
        case 'Q':
        case 'q':
            quit(window);
            flag_exit=true;
            break;
        default:
            break;
    }
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            {
                if (action == GLFW_RELEASE) {
                    flag_left=0;
                }
                else if(action==GLFW_PRESS)
                {
                    getprvxpos();
                    double xpos,ypos;
        			glfwGetCursorPos(window,&xpos,&ypos);
        			xpos=((xpos*(8.0))/600.0)-4.0;ypos=(-(ypos*(8.0))/600.0)+4.0;
                    if(isInside(0.5,3.5,-0.5,3.5,0,4.0,xpos,ypos)==1 && transition==false)//top
                    {
                    	score++;
                    	transition=true;
                    	up_key=true;
                    	up1_key=true;
                    	if(orientation==0)
                    	{
                        	rotation_axis=glm::vec3 (block_pos.x,block_pos.y+0.25,block_pos.z-0.5);
                        	block_pos.y+=0.75;block_pos.z-=0.25;orientation=2;
                        	block_y+=0.75;
                        	block_view_pos.y=block_pos.y+0.5;target_blk_view_pos.y=block_view_pos.y+2.0;
                        	block_view_pos.x=block_pos.x;target_blk_view_pos.x=block_pos.x;
                        	focus_block_view_pos.y=block_pos.y-1.5;focus_block_view_pos.x=block_pos.x;
                    	}
                    	else if(orientation==1)
                    	{
                        	rotation_axis=glm::vec3 (block_pos.x,block_pos.y+0.25,block_pos.z-0.25);
                        	block_pos.y+=0.5;orientation=1;
                        	block_y+=0.5;
                        	block_view_pos.y=block_pos.y+0.25;target_blk_view_pos.y=block_view_pos.y+2.0;
                        	block_view_pos.x=block_pos.x;target_blk_view_pos.x=block_pos.x;
                        	focus_block_view_pos.y=block_pos.y-0.5;focus_block_view_pos.x=block_pos.x;
                    	}
                    	else if(orientation==2)
                    	{
                        	rotation_axis=glm::vec3 (block_pos.x,block_pos.y+0.5,block_pos.z-0.25);
                        	block_pos.y+=0.75;block_pos.z+=0.25;orientation=0;
                        	block_y+=0.75;
                        	block_view_pos.y=block_pos.y+0.25;target_blk_view_pos.y=block_view_pos.y+2.0;
                        	block_view_pos.x=block_pos.x;target_blk_view_pos.x=block_pos.x;
                        	focus_block_view_pos.y=block_pos.y-0.5;focus_block_view_pos.x=block_pos.x;
                    	}
                    	check_bridges();
                    	break;
                    }
                    else if(isInside(0.5,2.5,-0.5,2.5,0,2.0,xpos,ypos)==1 && transition==false)//bottom
                    {
                    score++;
                    transition=true;
                    down_key=true;
                    down1_key=true;
                    if(orientation==0)
                    {
                        rotation_axis=glm::vec3 (block_pos.x,block_pos.y-0.25,block_pos.z-0.5);
                        block_pos.y-=0.75;block_pos.z-=0.25;orientation=2;
                        block_y-=0.75;
                        block_view_pos.y=block_pos.y-0.5;target_blk_view_pos.y=block_view_pos.y-2.0;
                        block_view_pos.x=block_pos.x;target_blk_view_pos.x=block_pos.x;
                        focus_block_view_pos.y=block_pos.y+1.5;focus_block_view_pos.x=block_pos.x;
                    }
                    else if(orientation==1)
                    {
                        rotation_axis=glm::vec3 (block_pos.x,block_pos.y-0.25,block_pos.z-0.25);
                        block_pos.y-=0.5;orientation=1;
                        block_y-=0.5;
                        block_view_pos.y=block_pos.y-0.25;target_blk_view_pos.y=block_view_pos.y-2.0;
                        target_blk_view_pos.x=block_pos.x;block_view_pos.x=block_pos.x;
                        focus_block_view_pos.y=block_pos.y+0.5;focus_block_view_pos.x=block_pos.x;
                    }
                    else if(orientation==2)
                    {
                        rotation_axis=glm::vec3 (block_pos.x,block_pos.y-0.5,block_pos.z-0.25);
                        block_pos.y-=0.75;block_pos.z+=0.25;orientation=0;
                        block_y-=0.75;
                        block_view_pos.y=block_pos.y-0.25;target_blk_view_pos.y=block_view_pos.y-2.0;
                        block_view_pos.x=block_pos.x;target_blk_view_pos.x=block_pos.x;
                        focus_block_view_pos.y=block_pos.y+0.5;focus_block_view_pos.x=block_pos.x;
                    }
                    check_bridges();
                    break;
                    }
                    else if(isInside(0.6,3.5,0.6,2.5,1.1,3.0,xpos,ypos)==1 && transition==false)//right
                    {
                    score++;
                    right_key=true;
                    transition=true;
                    right1_key=true;
                    if(orientation==0)
                    {
                        rotation_axis=glm::vec3 (block_pos.x+0.25,block_pos.y,block_pos.z-0.5);
                        block_pos.x+=0.75;block_pos.z-=0.25;orientation=1;
                        block_x+=0.75;
                        block_view_pos.x=block_pos.x+0.5;target_blk_view_pos.x=block_view_pos.x+2.0;
                        block_view_pos.y=block_pos.y;target_blk_view_pos.y=block_pos.y;
                        focus_block_view_pos.x=block_pos.x-1.25;focus_block_view_pos.y=block_pos.y;
                    }
                    else if(orientation==1)
                    {
                        rotation_axis=glm::vec3 (block_pos.x+0.5,block_pos.y,block_pos.z-0.25);
                        block_pos.x+=0.75;block_pos.z+=0.25;orientation=0;
                        block_x+=0.75;
                        block_view_pos.x=block_pos.x+0.25;target_blk_view_pos.x=block_view_pos.x+2.0;
                        block_view_pos.y=block_pos.y;target_blk_view_pos.y=block_pos.y;
                        focus_block_view_pos.x=block_pos.x-0.5;focus_block_view_pos.y=block_pos.y;
                    }
                    else if(orientation==2)
                    {
                        rotation_axis=glm::vec3 (block_pos.x+0.25,block_pos.y,block_pos.z-0.25);
                        block_pos.x+=0.5;orientation=2;
                        block_x+=0.5;
                        block_view_pos.x=block_pos.x+0.25;target_blk_view_pos.x=block_view_pos.x+2.0;
                        block_view_pos.y=block_pos.y;target_blk_view_pos.y=block_pos.y;
                        focus_block_view_pos.x=block_pos.x-0.5;focus_block_view_pos.y=block_pos.y;
                    }
                    check_bridges();
                    break;
                    }
                    else if(isInside(-0.6,3.5,-0.6,2.5,-1.1,3.0,xpos,ypos)==1 && transition==false)//left
                    {
                    score++;
                    left_key=true;
                    left1_key=true;
                    transition=true;
                    if(orientation==0)
                    {
                        rotation_axis=glm::vec3 (block_pos.x-0.25,block_pos.y,block_pos.z-0.5);
                        block_x-=0.75;
                        block_pos.x-=0.75;block_pos.z-=0.25;orientation=1;
                        block_view_pos.x=block_pos.x-0.5;target_blk_view_pos.x=block_view_pos.x-2.0;
                        block_view_pos.y=block_pos.y;target_blk_view_pos.y=block_pos.y;
                        focus_block_view_pos.x=block_pos.x+1.5;focus_block_view_pos.y=block_pos.y;
                    }
                    else if(orientation==1)
                    {
                        rotation_axis=glm::vec3 (block_pos.x-0.5,block_pos.y,block_pos.z-0.25);
                        block_x-=0.75;
                        block_pos.x-=0.75;block_pos.z+=0.25;orientation=0;
                        block_view_pos.x=block_pos.x-0.25;target_blk_view_pos.x=block_view_pos.x-2.0;
                        block_view_pos.y=block_pos.y;target_blk_view_pos.y=block_pos.y;
                        focus_block_view_pos.x=block_pos.x+0.5;focus_block_view_pos.y=block_pos.y;
                    }
                    else if(orientation==2)
                    {
                        rotation_axis=glm::vec3 (block_pos.x-0.25,block_pos.y,block_pos.z-0.25);
                        block_x-=0.5;
                        block_pos.x-=0.5;orientation=2;
                        block_view_pos.x=block_pos.x-0.25;target_blk_view_pos.x=block_view_pos.x-2.0;
                        block_view_pos.y=block_pos.y;target_blk_view_pos.y=block_pos.y;
                        focus_block_view_pos.x=block_pos.x+0.5;focus_block_view_pos.y=block_pos.y;
                    }
                    check_bridges();
                    break;
                    }

                    flag_left=1;
                }
                break;
            }
        default:
            break;
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

    GLfloat fov = M_PI/2;

    // sets the viewport of openGL renderer
    glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

    // Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    if(level<4)
        Matrices.projection = glm::perspective(fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    //Ortho projection for 2D views
    if(level>3)
        Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

VAO *block,*triangle, *base,*spl_base,*cross,*circle,*segment[17],*triangle1;


void createBlock ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static GLfloat vertex_buffer_data [] = {
        0,-1,0, 1,-1,2, 0,-1,2,  0,-1,0,  1,-1,2,  1,-1,0,  // 0,-1,0, 1,-1,0, 1,-1,2,  1,-1,2,  0,-1,0, 0,-1,2,//forward
        1,-1,0, 1,0,2,  1,-1,2,  1,-1,0,  1,0,2,   1,0,0,//1,0,0,  1,0,2,  1,-1,2,  1,-1,2,  1,0,0,  1,-1,0,//right
        1,0,2,  0,0,0,  1,0,0,   1,0,2,   0,0,0,   0,0,2,//0,0,2,  1,0,2,  1,0,0,   0,0,2,   1,0,0,   0,0,0,//backword
        0,0,2,  0,-1,0, 0,0,0,   0,0,2,   0,-1,0,  0,-1,2,//0,0,0,  0,0,2,  0,-1,0,  0,-1,0,  0,0,2,   0,-1,2,//left
        0,0,2,  1,-1,2, 0,-1,2,  0,0,2,   1,-1,2,  1,0,2,//0,0,2,  1,-1,2, 1,0,2,   0,0,2,   1,-1,2,  0,-1,2,//top
        0,0,0,  1,-1,0, 0,-1,0,  0,0,0,   1,-1,0,  1,0,0,//0,0,0,  0,-1,0, 1,0,0,   0,-1,0,  1,0,0,  1,-1,0,//bottom
    };
    for(int i=0;i<36*3;i++)
        vertex_buffer_data[i]=vertex_buffer_data[i]/2.0;

    static const GLfloat color_buffer_data [] = {
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,   
        0.0f, 1.0f, 0.0f,   
        0.0f, 1.0f, 0.0f,   
        0.0f, 1.0f, 0.0f,   
        0.0f, 1.0f, 0.0f,   
        0.0f, 1.0f, 0.0f,   
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    block = create3DObject(GL_TRIANGLES, 12*3, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void create_spl_Base ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static GLfloat vertex_buffer_data [] = {
        0,-1,0, 1,-1,2, 0,-1,2,  0,-1,0,  1,-1,2,  1,-1,0,  // 0,-1,0, 1,-1,0, 1,-1,2,  1,-1,2,  0,-1,0, 0,-1,2,//forward
        1,-1,0, 1,0,2,  1,-1,2,  1,-1,0,  1,0,2,   1,0,0,//1,0,0,  1,0,2,  1,-1,2,  1,-1,2,  1,0,0,  1,-1,0,//right
        1,0,2,  0,0,0,  1,0,0,   1,0,2,   0,0,0,   0,0,2,//0,0,2,  1,0,2,  1,0,0,   0,0,2,   1,0,0,   0,0,0,//backword
        0,0,2,  0,-1,0, 0,0,0,   0,0,2,   0,-1,0,  0,-1,2,//0,0,0,  0,0,2,  0,-1,0,  0,-1,0,  0,0,2,   0,-1,2,//left
        0,0,2,  1,-1,2, 0,-1,2,  0,0,2,   1,-1,2,  1,0,2,//0,0,2,  1,-1,2, 1,0,2,   0,0,2,   1,-1,2,  0,-1,2,//top
        0,0,0,  1,-1,0, 0,-1,0,  0,0,0,   1,-1,0,  1,0,0,//0,0,0,  0,-1,0, 1,0,0,   0,-1,0,  1,0,0,  1,-1,0,//bottom
    };
    for(int i=0;i<36*3;i++)
        vertex_buffer_data[i]=vertex_buffer_data[i]/2.0;
    for(int i=2;i<36*3;i+=3)
        vertex_buffer_data[i]=vertex_buffer_data[i]/2.0;
    static GLfloat color_buffer_data[36*3];
    for(int i=0;i<6;i++)
    {
        for(int j=0;j<3;j++)
            color_buffer_data[i*18+j]=50.0/255.0;
        for(int j=10;j<13;j++)
            color_buffer_data[i*18+j]=128.0/255.0;
        //color_buffer_data[i*3+0]=74.0/255.0;
        //color_buffer_data[i*3+1]=14.0/255.0;
        //color_buffer_data[i*3+2]=14.0/255.0;
    }
    spl_base = create3DObject(GL_TRIANGLES, 12*3, vertex_buffer_data, color_buffer_data, GL_FILL); 
}
void createBase ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static GLfloat vertex_buffer_data [] = {
        0,-1,0, 1,-1,2, 0,-1,2,  0,-1,0,  1,-1,2,  1,-1,0,  // 0,-1,0, 1,-1,0, 1,-1,2,  1,-1,2,  0,-1,0, 0,-1,2,//forward
        1,-1,0, 1,0,2,  1,-1,2,  1,-1,0,  1,0,2,   1,0,0,//1,0,0,  1,0,2,  1,-1,2,  1,-1,2,  1,0,0,  1,-1,0,//right
        1,0,2,  0,0,0,  1,0,0,   1,0,2,   0,0,0,   0,0,2,//0,0,2,  1,0,2,  1,0,0,   0,0,2,   1,0,0,   0,0,0,//backword
        0,0,2,  0,-1,0, 0,0,0,   0,0,2,   0,-1,0,  0,-1,2,//0,0,0,  0,0,2,  0,-1,0,  0,-1,0,  0,0,2,   0,-1,2,//left
        0,0,2,  1,-1,2, 0,-1,2,  0,0,2,   1,-1,2,  1,0,2,//0,0,2,  1,-1,2, 1,0,2,   0,0,2,   1,-1,2,  0,-1,2,//top
        0,0,0,  1,-1,0, 0,-1,0,  0,0,0,   1,-1,0,  1,0,0,//0,0,0,  0,-1,0, 1,0,0,   0,-1,0,  1,0,0,  1,-1,0,//bottom
    };
    for(int i=0;i<36*3;i++)
        vertex_buffer_data[i]=vertex_buffer_data[i]/2.0;
    for(int i=2;i<36*3;i+=3)
        vertex_buffer_data[i]=vertex_buffer_data[i]/2.0;
    static GLfloat color_buffer_data[36*3];
    for(int i=0;i<6;i++)
    {
        for(int j=0;j<3;j++)
            color_buffer_data[i*18+j]=1;
        for(int j=10;j<13;j++)
            color_buffer_data[i*18+j]=1;
    }
    base = create3DObject(GL_TRIANGLES, 12*3, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createcross()
{
    static const GLfloat vertex_buffer_data [] = {
        -0.125,-0.125,0, // vertex 1
        0.125,0.125,0, // vertex 2
    };

    static  GLfloat color_buffer_data []={
        0,1,0,
        0,1,0
    };
    cross = create3DObject(GL_LINES, 2, vertex_buffer_data, color_buffer_data, GL_LINE);
}
void createtriangle()
{
	static GLfloat vertex_buffer_data[]={
		0.5,0,0,
		-0.5,0,0,
		0,0.5,0
	};
	static GLfloat color_buffer_data[]={
		1,1,1,
		1,1,1,
		1,1,1
	};
	triangle = create3DObject(GL_TRIANGLES, 3 , vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createtriangle1()
{
	static GLfloat vertex_buffer_data[]={
		0.5,0,0,
		-0.5,0,0,
		0,0.5,0
	};
	static GLfloat color_buffer_data[]={
		1,1,0,
		1,1,0,
		1,1,0
	};
	triangle1 = create3DObject(GL_TRIANGLES, 3 , vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createCircle ()
{
    static GLfloat vertex_buffer_data[720*9+10]={0};
    float teta=M_PI/360.0;
    float teta1=0.5;
    for(int i=0;i<360.0/teta1;i++)
    {
        vertex_buffer_data[9*i+0]=0.2*cos(teta-(M_PI/360.0));
        vertex_buffer_data[9*i+1]=0.2*sin(teta-(M_PI/360.0));
        vertex_buffer_data[9*i+3]=0.2*cos(teta);
        vertex_buffer_data[9*i+4]=0.2*sin(teta);
        teta+=M_PI/360.0;
    }

    static GLfloat color_buffer_data[720*9+10]={0};
    for(int i=0;i<360.0/teta1;i++)
    {
        color_buffer_data[9*i+0]=1;
        for(int j=1;j<9;j++)
            color_buffer_data[9*i+j]=0;
    }
    circle = create3DObject(GL_TRIANGLES, (360.0/teta1)*3, vertex_buffer_data, color_buffer_data, GL_LINE);
}
// Creates the rectangle object used in this sample code
void function_segment()
{
    static  GLfloat vertex_buffer_data [] = {
        -0.18,0.05,0, 0.18,0.05,0,  -0.18, -0.05,0, 

        0.18,-0.05,0, 0.18, 0.05,0, -0.18,-0.05,0  
    };

    static  GLfloat color_buffer_data [] = {
        1,1,1, 1,1,1, 1,1,1, 

        1,1,1, 1,1,1, 1,1,1  
    };
    segment[0] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
    for(int i=1;i<18;i+=3)
        vertex_buffer_data[i]+=0.36;
    segment[1] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
    for(int i=1;i<18;i+=3)
        vertex_buffer_data[i]-=0.72;
    segment[2] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
    for(int i=0;i<18;i+=3)
        vertex_buffer_data[i]+=0.44;
    segment[7] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
    for(int i=1;i<18;i+=3)
        vertex_buffer_data[i]+=0.36;
    segment[8] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
    for(int i=1;i<18;i+=3)
        vertex_buffer_data[i]+=0.36;
    segment[9] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
    static GLfloat ertex_buffer_data[]={
        0.28,0.36,0, 0.28,0.05,0, 0.18,0.05,0,
        0.18,0.36,0, 0.28,0.36,0, 0.18,0.05,0
    };
    segment[3] = create3DObject(GL_TRIANGLES, 6, ertex_buffer_data, color_buffer_data, GL_FILL);
    for(int i=0;i<18;i+=3)
        ertex_buffer_data[i]-=0.46;
    segment[4] = create3DObject(GL_TRIANGLES, 6, ertex_buffer_data, color_buffer_data, GL_FILL);
    for(int i=1;i<18;i+=3)
        ertex_buffer_data[i]-=0.36;
    segment[5] = create3DObject(GL_TRIANGLES, 6, ertex_buffer_data, color_buffer_data, GL_FILL);
    for(int i=0;i<18;i+=3)
        ertex_buffer_data[i]+=0.46;
    segment[6] = create3DObject(GL_TRIANGLES, 6, ertex_buffer_data, color_buffer_data, GL_FILL);
    for(int i=0;i<18;i+=3)
        ertex_buffer_data[i]+=0.44;
    segment[10] = create3DObject(GL_TRIANGLES, 6, ertex_buffer_data, color_buffer_data, GL_FILL);
    for(int i=1;i<18;i+=3)
        ertex_buffer_data[i]+=0.36;
    segment[11] = create3DObject(GL_TRIANGLES, 6, ertex_buffer_data, color_buffer_data, GL_FILL);
    static GLfloat rtex_buffer_data[]={
        0.26,-0.31,0, 0.26,-0.41,0,                                              0.26+(0.51*cos(M_PI/4.0)),-0.31+(0.51*sin(M_PI/4.0)),0,
        0.26,-0.41,0, 0.26+(0.51*cos(M_PI/4.0)),-0.31+(0.51*sin(M_PI/4.0)),0,     0.26+(0.51*cos(M_PI/4.0)),-0.41+(0.51*sin(M_PI/4.0)),0
    };
    segment[12] = create3DObject(GL_TRIANGLES, 6, rtex_buffer_data, color_buffer_data, GL_FILL);
    for(int i=1;i<18;i+=3)
        rtex_buffer_data[i]+=0.36;
    segment[13] = create3DObject(GL_TRIANGLES, 6, rtex_buffer_data, color_buffer_data, GL_FILL);
    static GLfloat tex_buffer_data[]={
        0.20,-0.31,0, 0.20,-0.41,0,                                              0.20+(0.51*cos(3*M_PI/4.0)),-0.31+(0.51*sin(3*M_PI/4.0)),0,
        0.20,-0.41,0, 0.20+(0.51*cos(3*M_PI/4.0)),-0.31+(0.51*sin(3*M_PI/4.0)),0,    0.20+(0.51*cos(3*M_PI/4.0)),-0.41+(0.51*sin(3*M_PI/4.0)),0
    };
    segment[14] = create3DObject(GL_TRIANGLES, 6, tex_buffer_data, color_buffer_data, GL_FILL);
    for(int i=1;i<18;i+=3)
        tex_buffer_data[i]+=0.36;
    segment[15] = create3DObject(GL_TRIANGLES, 6, tex_buffer_data, color_buffer_data, GL_FILL);
}
void create(float xx,float yy,float zz,float angle,float rx,float ry,float rz,VAO *shape)
{
    glm::mat4 VP = Matrices.projection * Matrices.view;
    glm::mat4 MVP;
    Matrices.model = glm::mat4(1.0f);
    // glm::mat4 scaler = glm::scale(glm::vec3(2,3,5));
    glm::mat4 translate = glm::translate (glm::vec3(xx, yy , zz));        // glTranslatef
    glm::mat4 rotate = glm::rotate((float)(angle), glm::vec3(rx,ry,rz)); // rotate about vector (-1,1,1)
    Matrices.model *= (translate * rotate);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(shape);
}
void print_onscreen(char str[],int type)
{
    char list[16][23]={
        {' ',' ','2','3','4','5','6',' ','8','9','A',' ','E',' ',' ',' ',' ','P','R','S',' ',' ','-'},//0
        {'0',' ','2','3',' ','5','6','7','8','9','A','C','E','G',' ',' ','O','P','R','S',' ',' ',' '},//1
        {'0',' ','2','3',' ','5','6',' ','8',' ',' ','C','E','G','L',' ','O',' ',' ','S',' ',' ',' '},//2
        {'0','1','2','3','4',' ',' ','7','8','9','A',' ',' ',' ',' ',' ','O','P','R',' ',' ',' ',' '},//3
        {'0',' ',' ',' ','4','5','6',' ','8','9','A','C','E','G','L','M','O','P','R','S','V',' ',' '},//4
        {'0',' ','2',' ',' ',' ','6',' ','8',' ','A','C','E','G','L','M','O','P','R',' ',' ',' ',' '},//5
        {'0','1',' ','3','4','5','6','7','8','9','A',' ',' ','G',' ',' ','O',' ',' ','S',' ','Y',' '},//6
        {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},//7
        {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','G',' ',' ',' ',' ',' ',' ',' ',' ',' '},//8
        {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','G',' ',' ',' ',' ',' ',' ',' ',' ',' '},//9
        {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','G',' ','M',' ',' ',' ',' ',' ',' ',' '},//10
        {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','M',' ',' ',' ',' ','V',' ',' '},//11
        {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','V',' ',' '},//12
        {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','M',' ',' ',' ',' ',' ','Y',' '},//13
        {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','R',' ','V',' ',' '},//14
        {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','M',' ',' ',' ',' ',' ','Y',' '},//15
    };
    int len=strlen(str);
    float posy,posx;
    if(type==1)
        posx=2.0,posy=-2.5;
    else if(type==2)
        posy=3.5,posx=-3.0;
    else if(type==3)
        posx=3.4,posy=2.4;
    else if(type==4)
        posx=0.0,posy=-1.0;
    else if(type==5)
        posx=3.0,posy=-1.0;
    else if(type==6)
    	posx=2.5,posy=2.0;
    for(int i=len-1;i>=0;i--)
    {
        for(int j=0;j<16;j++)
        {
            bool flag=false;
            for(int k=0;k<23;k++)
            {
                if(list[j][k]==str[i] && str[i]!=' '){flag=true;break;}
            }
            if(flag )
            {
                create(posx,posy,0,0,1,0,0,segment[j]);
            }
        }
        if(type==2 || type==5 || type==6)posx-=0.7;
        else if((type==3 || type==4 || type==1) && i>0)
        {
            if(str[i-1]=='G' || str[i-1]=='M' || str[i-1]=='V' || str[i-1]=='Y')
                posx-=1.2;
            else
                posx-=0.7;
        }
    }
}
float camera_rotation_angle = 90;
void check_bridges()
{
    if(orientation==0)
    {
        for(int i=1;i<=3;i++)
        {		
            if(bridge_3_x[i]==block_x && bridge_3_y[i]==block_y)
                bridge_3[i]*=-1;
        }
        if(bridge_2_x[1]==block_x && bridge_2_y[1]==block_y)
            bridge_2[1]*=-1;
    }
    else if(orientation==1)
    {
        if((bridge_3_x[1]==block_x-0.25 || bridge_3_x[1]==block_x+0.25) && bridge_3_y[1]==block_y)
            bridge_3[1]*=-1;
        if((bridge_2_x[1]==block_x-0.25 || bridge_2_x[1]==block_x+0.25) && bridge_2_y[1]==block_y)
            bridge_2[1]*=-1;
    }
    else if(orientation==2)
    {
        if((bridge_3_y[1]==block_y-0.25 || bridge_3_y[1]==block_y+0.25) && bridge_3_x[1]==block_x)
            bridge_3[1]*=-1;
        if((bridge_2_y[1]==block_y-0.25 || bridge_2_y[1]==block_y+0.25) && bridge_2_x[1]==block_x)
            bridge_2[1]*=-1;
    }
}
void check_blockfall()
{
    if(orientation==0)
    {
        bool flag=false;
        for(int i=0;i<block_centers.size();i++)
        {
            if(block_x==block_centers[i].first && block_y==block_centers[i].second)
                flag=true;
        }
        if(flag==false)
        {
            if(level==1 && block_x==2 && block_y==0)
                level_up=true;
            else if(level==2 && block_x==3.0 && block_y==0)
                level_up=true;
            else if(level==3 && block_x==-2.5 && block_y==3.5)
                level_up=true;
            gameover=true;
        }
    }
    else
    {
        bool flag1=false,flag2=false;
        if(orientation==1)
        {
            //cout<<"hii\n";
            for(int i=0;i<block_centers.size();i++)
            {
                if((block_x+0.25==block_centers[i].first && block_y==block_centers[i].second))
                    flag1=true;
                else if((block_x-0.25==block_centers[i].first && block_y==block_centers[i].second))
                    flag2=true;
            }
            for(int i=0;i<spl_block_centers.size();i++)
            {
                if((block_x+0.25==spl_block_centers[i].first && block_y==spl_block_centers[i].second))
                    flag1=true;
                else if((block_x-0.25==spl_block_centers[i].first && block_y==spl_block_centers[i].second))
                    flag2=true;
            }
            if(level==1 && (block_x+0.25==2 || block_x-0.25==2) && block_y==0)
            {flag1=true;flag2=true;}
            if(level==3 && (block_x+0.25==-2.5 || block_x-0.25==-2.5) && block_y==3.5)
            {flag1=true;flag2=true;}
            if(level==2 && (block_x+0.25==3.0 || block_x-0.25==3.0) && block_y==0)
            {flag1=true;flag2=true;}
        }
        else if(orientation==2)
        {
            for(int i=0;i<block_centers.size();i++)
            {
                if((block_x==block_centers[i].first && block_y+0.25==block_centers[i].second))
                    flag1=true;
                else if((block_x==block_centers[i].first && block_y-0.25==block_centers[i].second))
                    flag2=true;
            }
            for(int i=0;i<spl_block_centers.size();i++)
            {
                if((block_x==spl_block_centers[i].first && block_y+0.25==spl_block_centers[i].second))
                    flag1=true;
                else if((block_x==spl_block_centers[i].first && block_y-0.25==spl_block_centers[i].second))
                    flag2=true;
            }
            if(level==1 && (block_y+0.25==0 || block_y-0.25==0) && block_x==2.0)
            {flag1=true;flag2=true;}
            if(level==3 && (block_y+0.25==3.5 || block_y-0.25==3.5) && block_x==-2.5)
            {flag1=true;flag2=true;}
            if(level==2 && (block_y+0.25==0 || block_y-0.25==0) && block_x==3.0)
            {flag1=true;flag2=true;}
        }
        if(flag1==false || flag2==false)
        {
            gameover=true;
        }
    }
}
void check_gameover()
{
    if(gameover==true)
    {
        int i=0;
        while(i<1e8)
            i++;
        gameover_sound=1;
        block_pos = glm::vec3(0.25,-0.25,0.5);
        rotation_angle=0;
        transition=false;
        right_key=false;left_key=false;up_key=false;down_key=false;
        right1_key=false;left1_key=false;up1_key=false;down1_key=false;
        History = glm::mat4(1.0f);
        gameover=false;
        orientation=0;
        block_x=0;block_y=0;
        bridge_2[1]=-1;
        block_view_pos=glm::vec3(0.5,-0.25,0.25);
        target_blk_view_pos=glm::vec3(2,0,0);
        focus_block_view_pos=glm::vec3(-0.25,-0.25,3.0);
        press_b=false;press_f=false;
        if(level_up==true)
            level++;
        level_up=false;
        for(int i=1;i<4;i++)
            bridge_3[i]=-1;
    }
}

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw_objects(glm:: mat4 VP,glm:: vec3 x,int angle,int rot_x,int rot_y,int rot_z,VAO *shape)
{
    glm::mat4 MVP;  // MVP = Projection * View * Model

    // Load identity to model matrix
    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translateRectangle = glm::translate (x);        // glTranslatef
    glm::mat4 rotateRectangle = glm::rotate((float)(angle*M_PI/180.0f), glm::vec3(rot_x,rot_y,rot_z));
    Matrices.model *= (translateRectangle * rotateRectangle);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(shape);


}

void createtiles(glm::mat4 VP)
{
    block_centers.clear();
    spl_block_centers.clear();
    if(level==1)
    {
        base_pos=glm::vec3(0,0,-0.5);
        for(int i=0;i<3;i++)
        {
            draw_objects(VP,base_pos,0,0,1,0,base);block_centers.push_back(make_pair(base_pos.x,base_pos.y));
            base_pos.x+=0.5;
        }
        base_pos.x=1.5;
        for(int i=0;i<4;i++)
        {
            base_pos.y=-0.5;
            for(int j=0;j<4;j++)
            {
                if(!(i==1 && j==1))
                {draw_objects(VP,base_pos,0,0,1,0,base);block_centers.push_back(make_pair(base_pos.x,base_pos.y));}
                base_pos.y+=0.5;
            }
            base_pos.x+=0.5;
        }
    }
    else if(level==2)
    {
        float x=-0.5,y=-0.5;
        for(int i=0;i<3;i++)
        {
            y=-0.5;
            for(int j=0;j<4;j++)
            {
                if(!(i==1 && j==1))
                {draw_objects(VP,glm::vec3(x,y,-0.5),0,0,1,0,spl_base);spl_block_centers.push_back(make_pair(x,y));}
                y+=0.5;
            }
            x+=0.5;
        }
        draw_objects(VP,glm::vec3(0,0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(0,0));
        y=0;
        for(int i=0;i<3;i++)
        {
            draw_objects(VP,glm::vec3(x,y,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(x,y));
            y+=0.5;
        }
        draw_objects(VP,glm::vec3(1+0.25,0-0.25,0.05),0,0,1,0,circle);//circle
        if(bridge_2[1]==1)
        {
            draw_objects(VP,glm::vec3(1.5,1,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(1.5,1.0));
            draw_objects(VP,glm::vec3(2.0,1,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(2.0,1.0));
        }
        x=2.5;
        y=1.0;
        for(int i=0;i<3;i++)
        {
            y=1.0;
            for(int j=0;j<4;j++)
            {
                if(!(i==1 && j==2))
                {draw_objects(VP,glm::vec3(x,y,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(x,y));}
                y-=0.5;
            }
            x+=0.5;
        }
    }
    else if(level==3)
    {
        base_pos=glm::vec3(0,0,-0.5);
        for(int i=0;i<4;i++)
        {
            draw_objects(VP,base_pos,0,0,1,0,base);
            block_centers.push_back(make_pair(base_pos.x,base_pos.y));
            base_pos.y+=0.5;	
        }
        //upspecial
        draw_objects(VP,glm::vec3(0,2.0,-0.5),0,0,1,0,spl_base);spl_block_centers.push_back(make_pair(0,2.0));
        draw_objects(VP,glm::vec3(0,2.5,-0.5),0,0,1,0,spl_base);spl_block_centers.push_back(make_pair(0,2.5));
        draw_objects(VP,glm::vec3(0.5,2.0,-0.5),0,0,1,0,spl_base);spl_block_centers.push_back(make_pair(0.5,2.0));
        draw_objects(VP,glm::vec3(0.5,2.5,-0.5),0,0,1,0,spl_base);spl_block_centers.push_back(make_pair(0.5,2.5));


        draw_objects(VP,glm::vec3(1,2.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(1,2.0));
        draw_objects(VP,glm::vec3(1,2.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(1,2.5));

        draw_objects(VP,glm::vec3(1.5,1.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(1.5,1.5));
        draw_objects(VP,glm::vec3(1.5,2.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(1.5,2.0));
        draw_objects(VP,glm::vec3(1.5,2.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(1.5,2.5));

        draw_objects(VP,glm::vec3(2.0,1.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(2.0,1.5));
        draw_objects(VP,glm::vec3(2.0,2.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(2.0,2.0));
        draw_objects(VP,glm::vec3(2.0,2.5,-0.5),0,0,1,0,spl_base);spl_block_centers.push_back(make_pair(2.0,2.5));

        draw_objects(VP,glm::vec3(2.5,1.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(2.5,1.5));
        draw_objects(VP,glm::vec3(2.5,2.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(2.5,2.0));
        draw_objects(VP,glm::vec3(2.5,2.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(2.5,2.5));
        draw_objects(VP,glm::vec3(3.0,2.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(3.0,2.0));//cross
        draw_objects(VP,glm::vec3(3.0+0.25,2.0-0.25,0.05),0,0,1,0,cross);//cross
        draw_objects(VP,glm::vec3(3.0+0.25,2.0-0.25,0.05),180,0,1,0,cross);//cross
        draw_objects(VP,glm::vec3(3.02+0.25,2.0-0.25,0.05),0,0,1,0,cross);//cross
        draw_objects(VP,glm::vec3(3.02+0.25,2.0-0.25,0.05),180,0,1,0,cross);//cross
        draw_objects(VP,glm::vec3(-0.5,0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-0.5,0));
        draw_objects(VP,glm::vec3(-1.0,0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-1.0,0));
        if(bridge_3[1]==1)
        {
            draw_objects(VP,glm::vec3(-1.5,0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-1.5,0));
        }

        draw_objects(VP,glm::vec3(-0.5,-0.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-0.5,-0.5));
        draw_objects(VP,glm::vec3(-1.0,-0.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-1.0,-0.5));
        if(bridge_3[1]==1)
        {
            draw_objects(VP,glm::vec3(-1.5,-0.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-1.5,-0.5));
        }

        draw_objects(VP,glm::vec3(-0.5,-1.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-0.5,-1.0));
        draw_objects(VP,glm::vec3(-1.0,-1.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-1.0,-1.0));
        draw_objects(VP,glm::vec3(-1.5,-1.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-1.5,-1.0));//circle
        draw_objects(VP,glm::vec3(-1.5+0.25,-1.0-0.25,0.05),0,0,1,0,circle);//circle
        if(bridge_3[2]==1)
        {
            draw_objects(VP,glm::vec3(-1.5,0.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-1.5,0.5));
            draw_objects(VP,glm::vec3(-1.5,1.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-1.5,1.0));
        }	
        draw_objects(VP,glm::vec3(-2.5,1.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-2.5,1.0));//cross
        draw_objects(VP,glm::vec3(-2.5+0.25,1.0-0.25,0.05),0,0,1,0,cross);//cross
        draw_objects(VP,glm::vec3(-2.5+0.26,1.0-0.25,0.05),0,0,1,0,cross);//cross
        draw_objects(VP,glm::vec3(-2.5+0.25,1.0-0.25,0.05),180,0,1,0,cross);//cross
        draw_objects(VP,glm::vec3(-2.5+0.26,1.0-0.25,0.05),180,0,1,0,cross);//cross

        draw_objects(VP,glm::vec3(-1.5,1.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-1.5,1.5));
        draw_objects(VP,glm::vec3(-1.5,2.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-1.5,2.0));
        draw_objects(VP,glm::vec3(-2.0,1.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-2.0,1.5));
        draw_objects(VP,glm::vec3(-2.0,2.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-2.0,2.0));
        draw_objects(VP,glm::vec3(-2.5,1.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-2.5,1.5));
        draw_objects(VP,glm::vec3(-2.5,2.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-2.5,2.0));
        if(bridge_3[3]==1)
        {
            draw_objects(VP,glm::vec3(-2.5,2.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-2.5,2.5));
            draw_objects(VP,glm::vec3(-2.5,3.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-2.5,3.0));
        }    	
        draw_objects(VP,glm::vec3(-2.5,4.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-2.5,4.0));
        draw_objects(VP,glm::vec3(-2.0,3.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-2.0,3.5));
        draw_objects(VP,glm::vec3(-3.0,3.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-3.0,3.5));
        draw_objects(VP,glm::vec3(-2.0,4.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-2.0,4.0));
        draw_objects(VP,glm::vec3(-1.5,3.5,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-1.5,3.5));
        draw_objects(VP,glm::vec3(-1.5,4.0,-0.5),0,0,1,0,base);block_centers.push_back(make_pair(-1.5,4.0));
    }
}
void create(glm::mat4 VP,float xx,float yy,float zz,float angle,float rx,float ry,float rz,VAO *shape)
{
    glm::mat4 MVP;
    Matrices.model = glm::mat4(1.0f);
    // glm::mat4 scaler = glm::scale(glm::vec3(2,3,5));
    glm::mat4 translate = glm::translate (glm::vec3(xx, yy , zz));        // glTranslatef
    glm::mat4 rotate = glm::rotate((float)((angle*M_PI)/180.0), glm::vec3(rx,ry,rz)); // rotate about vector (-1,1,1)
    Matrices.model *= (translate * rotate);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(shape);
}
void draw (GLFWwindow* window, float x, float y, float w, float h, int doM, int doV, int doP)
{
    int fbwidth, fbheight;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);
    glViewport((int)(x*fbwidth), (int)(y*fbheight), (int)(w*fbwidth), (int)(h*fbheight));


    // use the loaded shader program
    // Don't change unless you know what you are doing

    if(press_b==true)
        eye_pos=block_view_pos;
    else if(press_f==true)
        eye_pos=focus_block_view_pos;
    else if(press_h==true)
        eye_pos=heli_block_view_pos;
    else if(press_w==true)
        eye_pos=glm::vec3(2,-2,5);
    else
        eye_pos=glm::vec3(0,0,5);
    glUseProgram(programID);

    //   cout<<"eye_pos"<<" "<<eye_pos.x<<" "<<eye_pos.y<<" "<<eye_pos.z<<endl;

    // Eye - Location of camera. Don't change unless you are sure!!
    glm::vec3 eye (eye_pos);
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    if(press_b==true || press_f==true)
        target=target_blk_view_pos;
    else 
        target=glm::vec3(0,0,0);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    if(press_h==true)
        up=glm::vec3(0, 0, 1);
    else 
        up=glm::vec3(0,1,0);

    // Compute Camera matrix (view)
    if(doV)
        Matrices.view = glm::lookAt(eye, target, up); // Fixed camera for 2D (ortho) in XY plane
    else
        Matrices.view = glm::mat4(1.0f);

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    glm::mat4 VP;
    if (doP)
        VP = Matrices.projection * Matrices.view;
    else
        VP = Matrices.view;
    if(level<4)
    {
        check_blockfall();
        check_gameover();
        createtiles(VP);
        // Send our transformation to the currently bound shader, in the "MVP" uniform
        // For each model you render, since the MVP will be different (at least the M part)
        glm::mat4 MVP;	// MVP = Projection * View * Model

        // Load identity to model matrix
        Matrices.model = glm::mat4(1.0f);
        if(transition==true)
        {
            if(right_key==true || down_key==true)
                rotation_angle+=0.024;
            else if(left_key==true || up_key==true)
                rotation_angle-=0.024;
        }
        glm::mat4 ca_translate = glm::translate(rotation_axis);
        glm::mat4 rotation=glm::mat4 (1.0f);
        if(right_key || left_key)
            rotation=glm::rotate((float)(rotation_angle*M_PI/180.0f), glm::vec3(0,1,0));
        else if(up_key || down_key)
            rotation=glm::rotate((float)(rotation_angle*M_PI/180.0f), glm::vec3(1,0,0));
        glm::mat4 ca_rev_translate = glm::translate(glm::vec3(-1*rotation_axis.x,-1*rotation_axis.y,-1*rotation_axis.z));

        Matrices.model*=ca_translate * rotation * ca_rev_translate * History;
        if(doM)
            MVP = VP * Matrices.model;
        else
            MVP = VP;
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

        // draw3DObject draws the VAO given to it using current MVP matrix
        if(gameover==false)
            draw3DObject(block);

        if(fabs(fabs(rotation_angle)-90)<0.1)
        {
            transition=false;
            right_key=false;left_key=false;up_key=false;down_key=false;
            right1_key=false;left1_key=false;up1_key=false;down1_key=false;
            rotation_angle=0;
            History=ca_translate*rotation*ca_rev_translate*History;
        }
        if(fabs(fabs(rotation_angle)-60)<0.1)
        {
            block_sound=1;
        }
        
      //  cout<<xpos<<" "<<ypos<<endl;
        if(flag_left==1 && press_h==true)
        {
            double xpos,ypos;
            glfwGetCursorPos(window,&xpos,&ypos);
            // xpos=((xpos*(8.0))/700.0)-4.0;
            // screenxn+=-xpos+prvxpos;screenxp+=-xpos+prvxpos;
            phi+=((prvxpos-xpos)*18.0)/600.0;
            getprvxpos();
            //if(xpos>prvxpos){flag_screenpan=-1;getprvxpos ();}
            //else if(xpos<prvxpos){flag_screenpan=1;getprvxpos ();}
        }

        //new___
        glm::mat4 DUP = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
    	Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

        VP = DUP * Matrices.view;
     //   MVP = glm::mat4(1.0f);
        if(up1_key)
 		create(VP,0,3.5,0,0,0,0,1,triangle1);//top
 		else
 		create(VP,0,3.5,0,0,0,0,1,triangle);//top
 		if(down1_key)
 		create(VP,0,2.5,0,180,0,0,1,triangle1);//down
 		else
 		create(VP,0,2.5,0,180,0,0,1,triangle);//down
 		if(right1_key)
 		create(VP,0.6,3.0,0,-90,0,0,1,triangle1);//right
 		else
 		create(VP,0.6,3.0,0,-90,0,0,1,triangle);//right
 		if(left1_key)
 		create(VP,-0.6,3.0,0,90,0,0,1,triangle1);//left
 		else
 		create(VP,-0.6,3.0,0,90,0,0,1,triangle);//left
 		char *xx=itoa(score);
        print_onscreen(xx,6);
    }
    if(level==4)
    {
        char gameover[]="GAMEOVER";
        print_onscreen(gameover,3);
        char sscore[]="MOVES-";
        print_onscreen(sscore,4);
        char *xx=itoa(score);
        print_onscreen(xx,5);
        char replay[]="REPLAY-R";
        print_onscreen(replay,1);
        if(press_r==true)
        {
            level=1;score=0;
        }
        press_r=false;
    }

}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height){

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        exit(EXIT_FAILURE);
        glfwTerminate();
    }

    glfwMakeContextCurrent(window);
    //    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);
    glfwSetWindowCloseCallback(window, quit);
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
    // Create the models
    createBlock();
    createBase();
    create_spl_Base();
    createcross();
    createCircle();
    function_segment();
    createtriangle();
    createtriangle1();


    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
    // Get a handle for our "MVP" uniform
    Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


    reshapeWindow (window, width, height);

    // Background color of the scene
    glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
    glClearDepth (1.0f);

    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);

    // cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    // cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    // cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    // cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

void *gamefunction(void *)
{
    int width = 600;
    int height = 600;
    rect_pos = glm::vec3(0, 0, 0);
    floor_pos = glm::vec3(0, 0, 0);
    block_pos = glm::vec3(0.5,-0.25,0.5);
    eye_pos = glm::vec3(0,0,5);
    block_view_pos=glm::vec3(0.5,-0.25,0.25);
    focus_block_view_pos=glm::vec3(-0.25,-0.25,3.0);
    target_blk_view_pos=glm::vec3(2,0,0);

    GLFWwindow* window = initGLFW(width, height);
    initGLEW();
    initGL (window, width, height);

    last_update_time = glfwGetTime();
    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        heli_block_view_pos=glm::vec3(rad*sin(teta)*cos(phi),rad*sin(teta)*sin(phi),rad*cos(teta));
        // clear the color and depth in the frame buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // OpenGL Draw commands
        current_time = glfwGetTime();
        last_update_time = current_time;
        draw(window, 0, 0, 1, 1, 1, 1, 1);
        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();
    }

    glfwTerminate();
    //    exit(EXIT_SUCCESS);
}

void *sound_1(void *)
{
    while(1)
    {
        if(flag_exit==true)break;
        if(block_sound==0)
            continue;
        ao_device* device;
        ao_sample_format format;
        int defaultDriver;
        WavHeader header;
        std::ifstream file;
        file.open("brick1.wav", std::ios::binary | std::ios::in);
        file.read(header.id, sizeof(header.id));
        assert(!std::memcmp(header.id, "RIFF", 4)); //is it a WAV file?
        file.read((char*)&header.totalLength, sizeof(header.totalLength));
        file.read(header.wavefmt, sizeof(header.wavefmt)); //is it the right format?
        assert(!std::memcmp(header.wavefmt, "WAVEfmt ", 8));
        file.read((char*)&header.format, sizeof(header.format));
        file.read((char*)&header.pcm, sizeof(header.pcm));
        file.read((char*)&header.channels, sizeof(header.channels));
        file.read((char*)&header.frequency, sizeof(header.frequency));
        file.read((char*)&header.bytesPerSecond, sizeof(header.bytesPerSecond));
        file.read((char*)&header.bytesByCapture, sizeof(header.bytesByCapture));
        file.read((char*)&header.bitsPerSample, sizeof(header.bitsPerSample));
        file.read(header.data, sizeof(header.data));
        file.read((char*)&header.bytesInData, sizeof(header.bytesInData));

        ao_initialize();

        defaultDriver = ao_default_driver_id();

        memset(&format, 0, sizeof(format));
        format.bits = header.format;
        format.channels = header.channels;
        format.rate = header.frequency;
        format.byte_format = AO_FMT_LITTLE;

        device = ao_open_live(defaultDriver, &format, NULL);
        if (device == NULL) {
            std::cout << "Unable to open driver" << std::endl;
        }


        char* buffer = (char*)malloc(BUF_SIZE * sizeof(char));

        // determine how many BUF_SIZE chunks are in file
        int fSize = header.bytesInData;
        int bCount = fSize / BUF_SIZE;

        for (int i = 0; i < bCount; ++i) {
            file.read(buffer, BUF_SIZE);
            ao_play(device, buffer, BUF_SIZE);
        }

        int leftoverBytes = fSize % BUF_SIZE;
        // std::cout << leftoverBytes;
        file.read(buffer, leftoverBytes);
        memset(buffer + leftoverBytes, 0, BUF_SIZE - leftoverBytes);
        ao_play(device, buffer, BUF_SIZE);

        ao_close(device);
        ao_shutdown();
        block_sound=0;
        gameover_sound=0;
    }
}

int main(int argc,char **argv)
{
    pthread_t threads[5];
    pthread_create(&threads[0], NULL, gamefunction, NULL);
    pthread_create(&threads[1], NULL,sound_1 , NULL);
    //pthread_create(&threads[2], NULL,sound_2 , NULL);
    while(flag_exit==false)
    {
        //cout<<"hii\n";
    }
    exit(0);
    return 0;
}
