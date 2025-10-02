#version 330 core
uniform vec2 u_center;
uniform float u_scale;
uniform int u_maxIter;
in vec2 uv;
out vec4 FragColor;

vec3 pal(float t){ return vec3(0.6*vec3(sin(6.0*t), sin(5.0*t+1.0), sin(4.0*t+2.0))+0.4); }

void main(){
    vec2 c = (uv - vec2(0.5))*u_scale*2.0 + u_center;
    vec2 z = vec2(0.0);
    int i;
    for(i=0;i<u_maxIter;i++){
        // Celtic variant: use absolute of real part in iteration
        z = vec2(abs(z.x*z.x - z.y*z.y), 2.0*z.x*z.y) + c;
        if(dot(z,z) > 4.0) break;
    }
    if(i==u_maxIter) FragColor = vec4(0.0);
    else {
        float mu = float(i) + 1.0 - log(log(length(z)))/log(2.0);
        FragColor = vec4(pal(mu/float(u_maxIter)),1.0);
    }
}

