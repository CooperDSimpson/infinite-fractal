// infinite_mandelbrot.c
#include <windows.h>
#include "glad.h"
#include <stdio.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 fragCoord;
void main() {
    fragCoord = aPos;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 400 core
in vec2 fragCoord;
out vec4 FragColor;

uniform dvec2 u_center;   // High-precision center
uniform float u_scale;    // Float offset per pixel
uniform int u_maxIter;

void main() {
    // Map [-1,1] to per-pixel float offset
    vec2 offset = fragCoord * u_scale;
    dvec2 c = u_center + dvec2(offset);

    dvec2 z = dvec2(0.0, 0.0);
    int iter = 0;

    while (dot(z,z) <= 4.0 && iter < u_maxIter) {
        z = dvec2(z.x*z.x - z.y*z.y + c.x,
                  2.0*z.x*z.y + c.y);
        iter++;
    }

float t = float(iter)/float(u_maxIter);
vec3 tarletonPurple = vec3(0.302, 0.0, 0.6);
vec3 color = tarletonPurple * t * (0.5 + 0.5*sin(3.1415*t*10.0)); 
FragColor = vec4(color, 1.0);

}
)";

GLuint compileShader(GLenum type, const char* src){
    GLuint shader = glCreateShader(type);
    glShaderSource(shader,1,&src,NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success){
        char log[512];
        glGetShaderInfoLog(shader,512,NULL,log);
        MessageBoxA(NULL, log, "Shader compile error", MB_OK);
        ExitProcess(1);
    }
    return shader;
}

GLuint createProgram(){
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint prog = glCreateProgram();
    glAttachShader(prog,vs);
    glAttachShader(prog,fs);
    glLinkProgram(prog);
    GLint success;
    glGetProgramiv(prog,GL_LINK_STATUS,&success);
    if(!success){
        char log[512];
        glGetProgramInfoLog(prog,512,NULL,log);
        MessageBoxA(NULL, log, "Program link error", MB_OK);
        ExitProcess(1);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

HDC hDC;
double cx = -0.5, cy = 0.0, scale = 3.0;
int maxIter = 500;
POINT lastMouse;
int dragging = 0;

int main(){
    // Win32 window setup
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "FractalWindow";
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA("FractalWindow","Infinite Mandelbrot",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT,CW_USEDEFAULT,800,600,
        NULL,NULL,wc.hInstance,NULL);

    hDC = GetDC(hwnd);

    // Pixel format
    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR),1 };
    pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    int pf = ChoosePixelFormat(hDC,&pfd);
    SetPixelFormat(hDC,pf,&pfd);

    // OpenGL context
    HGLRC hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC,hRC);

    if(!gladLoadGL()) {
        MessageBoxA(NULL,"Failed to initialize GLAD","Error",MB_OK);
        return 1;
    }

    GLuint program = createProgram();
    glUseProgram(program);

    // Fullscreen quad
    float vertices[] = {
        -1.0f,-1.0f,
         1.0f,-1.0f,
         1.0f, 1.0f,
        -1.0f, 1.0f
    };
    unsigned int indices[] = {0,1,2, 2,3,0};
    GLuint VAO,VBO,EBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glGenBuffers(1,&EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    int u_center = glGetUniformLocation(program,"u_center");
    int u_scale = glGetUniformLocation(program,"u_scale");
    int u_maxIter = glGetUniformLocation(program,"u_maxIter");

    MSG msg;
    while(1){
        while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)){
            if(msg.message==WM_QUIT) goto end;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glUniform2d(u_center, cx, cy); // high-precision center
        glUniform1f(u_scale, (float)scale); // small float offsets
        glUniform1i(u_maxIter, maxIter);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
        SwapBuffers(hDC);
    }

end:
    wglMakeCurrent(NULL,NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd,hDC);
    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
        case WM_LBUTTONDOWN:
            dragging = 1;
            lastMouse.x = LOWORD(lParam);
            lastMouse.y = HIWORD(lParam);
            break;
        case WM_LBUTTONUP:
            dragging = 0;
            break;
        case WM_MOUSEMOVE:
            if(dragging){
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                RECT rect;
                GetClientRect(hwnd,&rect);
                int w = rect.right-rect.left;
                int h = rect.bottom-rect.top;
                cx -= (x-lastMouse.x)/(double)w*scale*2;
                cy += (y-lastMouse.y)/(double)h*scale*2;
                lastMouse.x = x;
                lastMouse.y = y;
            }
            break;
        case WM_MOUSEWHEEL: {
            short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if(delta>0) scale *= 0.9;
            else scale /= 0.9;
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd,msg,wParam,lParam);
    }
    return 0;
}

