#include <windows.h>
#include "glad.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// --- Double-double arithmetic ---
typedef struct { double hi, lo; } dd_t;

dd_t dd_add(dd_t a, dd_t b) {
    double s = a.hi + b.hi;
    double v = s - a.hi;
    double t = ((b.hi - v) + (a.hi - (s - v))) + a.lo + b.lo;
    return (dd_t){s + t, t - (s + t - s)};
}

dd_t dd_sub(dd_t a, dd_t b) {
    double s = a.hi - b.hi;
    double v = s - a.hi;
    double t = ((-b.hi - v) + (a.hi - (s - v))) + a.lo - b.lo;
    return (dd_t){s + t, t - (s + t - s)};
}

inline dd_t dd_mul(dd_t a, dd_t b) {
    double p = a.hi * b.hi;
    double e = fma(a.hi,b.hi,-p) + a.hi*b.lo + a.lo*b.hi + a.lo*b.lo;
    return (dd_t){p + e, e - (p + e - p)};
}

dd_t dd_from_double(double x) { return (dd_t){x,0.0}; }

// --- Shader sources ---
const char* vertexShaderSource = R"(
#version 330 core
layout(location=0) in vec2 aPos;
out vec2 fragCoord;
void main() {
    fragCoord = aPos;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 450 core
in vec2 fragCoord;
out vec4 FragColor;

uniform dvec2 u_center;
uniform double u_scale;
uniform int u_maxIter;

struct dd { double hi; double lo; };

dd dd_add(dd a, dd b) {
    double s = a.hi + b.hi;
    double v = s - a.hi;
    double t = ((b.hi - v) + (a.hi - (s - v))) + a.lo + b.lo;
    return dd(s+t, t-(s+t-s));
}

dd dd_sub(dd a, dd b) {
    double s = a.hi - b.hi;
    double v = s - a.hi;
    double t = ((-b.hi - v) + (a.hi - (s - v))) + a.lo - b.lo;
    return dd(s+t, t-(s+t-s));
}

dd dd_mul(dd a, dd b) {
    double p = a.hi * b.hi;
    double e = fma(a.hi,b.hi,-p) + a.hi*b.lo + a.lo*b.hi + a.lo*b.lo;
    return dd(p+e, e-(p+e-p));
}

dd dd_from_double(double x){ return dd(x,0.0); }

void main() {
    // Map fragCoord (-1..1) to complex plane
    dd cx = dd_add(dd_from_double(u_center.x), dd_mul(dd_from_double(fragCoord.x), dd_from_double(u_scale)));
    dd cy = dd_add(dd_from_double(u_center.y), dd_mul(dd_from_double(fragCoord.y), dd_from_double(u_scale)));

    dd zx = dd_from_double(0.0);
    dd zy = dd_from_double(0.0);

    int iter = 0;
    while(iter < u_maxIter) {
        dd zx2 = dd_mul(zx,zx);
        dd zy2 = dd_mul(zy,zy);
        dd zxzy = dd_mul(zx,zy);

        dd zx_new = dd_add(dd_sub(zx2, zy2), cx);
        dd zy_new = dd_add(dd_mul(dd_from_double(2.0), zxzy), cy);

        zx = zx_new;
        zy = zy_new;

        if(zx.hi*zx.hi + zy.hi*zy.hi > 4.0) break;
        iter++;
    }

    float t = float(iter)/float(u_maxIter);
    vec3 color = vec3(0.302,0.0,0.6) * t * (0.5 + 0.5*sin(3.1415*t*10.0));
    FragColor = vec4(color,1.0);
}
)";

// --- OpenGL helpers ---
GLuint compileShader(GLenum type,const char* src){
    GLuint shader=glCreateShader(type);
    glShaderSource(shader,1,&src,NULL);
    glCompileShader(shader);
    GLint success; glGetShaderiv(shader,GL_COMPILE_STATUS,&success);
    if(!success){
        char log[1024]; glGetShaderInfoLog(shader,1024,NULL,log);
        MessageBoxA(NULL,log,"Shader compile error",MB_OK);
        ExitProcess(1);
    }
    return shader;
}

GLuint createProgram(){
    GLuint vs = compileShader(GL_VERTEX_SHADER,vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER,fragmentShaderSource);
    GLuint prog = glCreateProgram();
    glAttachShader(prog,vs); glAttachShader(prog,fs);
    glLinkProgram(prog);
    GLint success; glGetProgramiv(prog,GL_LINK_STATUS,&success);
    if(!success){
        char log[1024]; glGetProgramInfoLog(prog,1024,NULL,log);
        MessageBoxA(NULL,log,"Program link error",MB_OK);
        ExitProcess(1);
    }
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

// --- Globals ---
HDC hDC;
double cx=-0.5, cy=0.0, scale=3.0;
int maxIter=1000;
POINT lastMouse; int dragging=0;

// --- Main ---
int main(){
    WNDCLASSA wc={0}; wc.lpfnWndProc=WndProc;
    wc.hInstance=GetModuleHandle(NULL); wc.lpszClassName="FractalWindow";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("FractalWindow","Infinite Mandelbrot",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,800,600,
        NULL,NULL,wc.hInstance,NULL);

    hDC=GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd={sizeof(PIXELFORMATDESCRIPTOR),1};
    pfd.dwFlags = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits=32;
    int pf = ChoosePixelFormat(hDC,&pfd); SetPixelFormat(hDC,pf,&pfd);

    HGLRC hRC = wglCreateContext(hDC); wglMakeCurrent(hDC,hRC);
    if(!gladLoadGL()){ MessageBoxA(NULL,"Failed to initialize GLAD","Error",MB_OK); return 1; }

    GLuint program = createProgram(); glUseProgram(program);

    float vertices[] = {-1,-1, 1,-1, 1,1, -1,1};
    unsigned int indices[] = {0,1,2, 2,3,0};
    GLuint VAO,VBO,EBO; glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO); glGenBuffers(1,&EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO); glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO); glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0); glEnableVertexAttribArray(0);

    GLint loc_center = glGetUniformLocation(program,"u_center");
    GLint loc_scale  = glGetUniformLocation(program,"u_scale");
    GLint loc_maxIter= glGetUniformLocation(program,"u_maxIter");

    MSG msg;
    while(1){
        while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)){
            if(msg.message==WM_QUIT) goto end;
            TranslateMessage(&msg); DispatchMessage(&msg);
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glUniform2d(loc_center,cx,cy);
        glUniform1d(loc_scale,scale);
        glUniform1i(loc_maxIter,maxIter);

        glBindVertexArray(VAO); glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
        SwapBuffers(hDC);
    }

end:
    wglMakeCurrent(NULL,NULL); wglDeleteContext(hRC); ReleaseDC(hwnd,hDC);
    return 0;
}

// --- Input handling ---
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
        case WM_LBUTTONDOWN: dragging=1; lastMouse.x=LOWORD(lParam); lastMouse.y=HIWORD(lParam); break;
        case WM_LBUTTONUP: dragging=0; break;
        case WM_MOUSEMOVE:
            if(dragging){
                int x=LOWORD(lParam), y=HIWORD(lParam);
                RECT r; GetClientRect(hwnd,&r);
                int w=r.right-r.left, h=r.bottom-r.top;
                cx -= (x-lastMouse.x)/(double)w*scale*2;
                cy += (y-lastMouse.y)/(double)h*scale*2;
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



