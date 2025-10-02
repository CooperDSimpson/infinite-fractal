// Fast interactive Mandelbrot using float shaders
#include <windows.h>
#include "glad.h"
#include <stdio.h>
#include <stdlib.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// --- Globals ---
HDC hDC;
double cx=-0.5, cy=0.0, scale=3.0;
int width=800, height=600;
int maxIter = 10000;  // can increase for stills
POINT lastMouse; int dragging=0;

// --- Shaders ---
const char* vertexShaderSource = R"(
#version 330 core
layout(location=0) in vec2 aPos;
out vec2 uv;
void main() {
    uv = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec2 uv;
out vec4 FragColor;

uniform vec2 u_center;
uniform float u_scale;
uniform int u_maxIter;

vec3 tarletonPurple = vec3(0.36, 0.0, 0.58); // RGB for Tarleton purple

void main() {
    vec2 c = u_center + (uv*2.0 - 1.0) * u_scale;
    vec2 z = vec2(0.0);

    int iter = 0;
    for(int i=0; i<u_maxIter; i++) {
        z = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
        if(dot(z,z) > 4.0) break;
        iter++;
    }

    if(iter == u_maxIter) {
        FragColor = vec4(tarletonPurple, 1.0); // inside set = purple
    } else {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0); // outside = white
    }
}

)";

// --- Helpers ---
GLuint compileShader(GLenum type,const char* src){
    GLuint shader=glCreateShader(type);
    glShaderSource(shader,1,&src,NULL);
    glCompileShader(shader);
    GLint ok; glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok){ char log[1024]; glGetShaderInfoLog(shader,1024,NULL,log); MessageBoxA(NULL,log,"Shader error",MB_OK); ExitProcess(1);}
    return shader;
}

GLuint createProgram(const char* vsSrc, const char* fsSrc){
    GLuint vs=compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs=compileShader(GL_FRAGMENT_SHADER, fsSrc);
    GLuint prog=glCreateProgram();
    glAttachShader(prog,vs); glAttachShader(prog,fs);
    glLinkProgram(prog);
    GLint ok; glGetProgramiv(prog,GL_LINK_STATUS,&ok);
    if(!ok){ char log[1024]; glGetProgramInfoLog(prog,1024,NULL,log); MessageBoxA(NULL,log,"Link error",MB_OK); ExitProcess(1);}
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

// --- Main ---
int main(){
    WNDCLASSA wc={0}; wc.lpfnWndProc=WndProc;
    wc.hInstance=GetModuleHandle(NULL); wc.lpszClassName="FractalWindow";
    RegisterClassA(&wc);
    HWND hwnd=CreateWindowA("FractalWindow","Mandelbrot",
                            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                            100,100,width,height,NULL,NULL,wc.hInstance,NULL);

    hDC=GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd={sizeof(pfd),1};
    pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType=PFD_TYPE_RGBA; pfd.cColorBits=32;
    int pf=ChoosePixelFormat(hDC,&pfd); SetPixelFormat(hDC,pf,&pfd);

    HGLRC hRC=wglCreateContext(hDC); wglMakeCurrent(hDC,hRC);
    if(!gladLoadGL()){ MessageBoxA(NULL,"GLAD failed","Error",MB_OK); return 1; }

    GLuint program = createProgram(vertexShaderSource, fragmentShaderSource);
    glUseProgram(program);

    // Quad
    float vertices[]={-1,-1,1,-1,1,1,-1,1};
    unsigned int indices[]={0,1,2,2,3,0};
    GLuint VAO,VBO,EBO;
    glGenVertexArrays(1,&VAO); glGenBuffers(1,&VBO); glGenBuffers(1,&EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO); glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO); glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0); glEnableVertexAttribArray(0);

    GLint loc_center=glGetUniformLocation(program,"u_center");
    GLint loc_scale=glGetUniformLocation(program,"u_scale");
    GLint loc_maxIter=glGetUniformLocation(program,"u_maxIter");

    MSG msg;
    while(1){
        while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)){
            if(msg.message==WM_QUIT) goto end;
            TranslateMessage(&msg); DispatchMessage(&msg);
        }
        

        glClear(GL_COLOR_BUFFER_BIT);
        glUniform2f(loc_center,(float)cx,(float)cy);
        glUniform1f(loc_scale,(float)scale);
        glUniform1i(loc_maxIter,maxIter);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
        SwapBuffers(hDC);
    }

end:
    wglMakeCurrent(NULL,NULL); wglDeleteContext(hRC); ReleaseDC(hwnd,hDC);
    return 0;
}

// --- Input ---
LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam){
    switch(msg){
        case WM_LBUTTONDOWN: dragging=1; lastMouse.x=LOWORD(lParam); lastMouse.y=HIWORD(lParam); break;
        case WM_LBUTTONUP: dragging=0; break;
        case WM_MOUSEMOVE:
            if(dragging){
                int x=LOWORD(lParam),y=HIWORD(lParam);
                cx-=(x-lastMouse.x)/(double)(width)*scale*2;
                cy+=(y-lastMouse.y)/(double)(height)*scale*2;
                lastMouse.x=x; lastMouse.y=y;
            }
            break;
        case WM_MOUSEWHEEL:
            if(GET_WHEEL_DELTA_WPARAM(wParam)>0) scale*=0.9;
            else scale/=0.9;
            break;
        case WM_DESTROY: PostQuitMessage(0); break;
        default: return DefWindowProc(hwnd,msg,wParam,lParam);
    }
    return 0;
}
