#version 330 core
uniform vec2 u_center;
uniform float u_scale;
uniform int u_maxIter;
in vec2 uv;
out vec4 FragColor;

vec3 pal(float t){ return vec3(0.5+0.5*cos(6.28318*(t+vec3(0.0,0.2,0.5)))); }

vec2 c_mul(vec2 a, vec2 b){
    return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}

vec2 c_pow3(vec2 z){
    // (x+iy)^3 = x^3 + 3ix^2y - 3xy^2 - i y^3
    float x = z.x, y = z.y;
    return vec2(x*x*x - 3.0*x*y*y, 3.0*x*x*y - y*y*y);
}

void main(){
    vec2 c = (uv - vec2(0.5))*u_scale*2.0 + u_center;
    vec2 z = vec2(0.0);
    int i;
    for(i=0;i<u_maxIter;i++){
        z = c_pow3(z) + c;
        if(dot(z,z) > 4.0) break;
    }
    if(i==u_maxIter) FragColor = vec4(0.0);
    else {
        float mu = float(i) + 1.0 - log(log(length(z)))/log(2.0);
        FragColor = vec4(pal(mu/float(u_maxIter)),1.0);
    }
}

