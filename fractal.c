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
int maxIter = 2;  // can increase for stills
POINT lastMouse; int dragging=0;

char* loadFile(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        MessageBoxA(NULL, filename, "Failed to open file", MB_OK);
        ExitProcess(1);
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    char* buffer = (char*)malloc(len + 1);
    if (!buffer) {
        MessageBoxA(NULL, "Out of memory", "Error", MB_OK);
        ExitProcess(1);
    }
    fread(buffer, 1, len, f);
    buffer[len] = '\0';
    fclose(f);
    return buffer;
}

char* chooseShaderFile() {
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile("*.frag", &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        MessageBoxA(NULL, "No .frag files found", "Error", MB_OK);
        ExitProcess(1);
    }

    char files[64][MAX_PATH]; // up to 64 shaders
    int count = 0;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            strcpy(files[count++], fd.cFileName);
            if (count >= 64) break;
        }
    } while (FindNextFile(hFind, &fd));
    FindClose(hFind);

    printf("Available fragment shaders:\n");
    for (int i = 0; i < count; i++) {
        printf("  %d) %s\n", i + 1, files[i]);
    }

    printf("Choose shader [1-%d]: ", count);
    int choice = 1;
    scanf("%d", &choice);
    if (choice < 1 || choice > count) choice = 1;

    return _strdup(files[choice - 1]); // caller frees
}

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
    printf("how many iterations? ");
    scanf("%d", &maxIter);
    char* fragSource = loadFile(chooseShaderFile());    


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

    GLuint program = createProgram(vertexShaderSource, fragSource);
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
    free(fragSource);
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
