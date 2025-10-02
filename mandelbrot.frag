#version 330 core
uniform vec2 u_center;
uniform float u_scale;
uniform int u_maxIter;
in vec2 uv;
out vec4 FragColor;

vec3 palette(float t){
    return vec3(0.5 + 0.5*cos(6.28318*(t+vec3(0.0,0.33,0.67))));
}

void main(){
    vec2 c = (uv - vec2(0.5))*u_scale*2.0 + u_center;
    vec2 z = vec2(0.0);
    int i;
    for(i=0;i<u_maxIter;i++){
        // z = z^2 + c
        vec2 zz = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
        z = zz;
        if(dot(z,z) > 4.0) break;
    }
    float iter = float(i);
    if(i < u_maxIter){
        // smooth iteration count
        float mu = iter + 1.0 - log(log(length(z)))/log(2.0);
        FragColor = vec4(palette(mu / float(u_maxIter)), 1.0);
    } else {
        FragColor = vec4(0.0,0.0,0.0,1.0);
    }
}

