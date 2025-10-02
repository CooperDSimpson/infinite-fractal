#version 330 core
uniform vec2 u_center; // re-used to pass the c parameter if you want
uniform float u_scale;
uniform int u_maxIter;
in vec2 uv;
out vec4 FragColor;

vec3 palette(float t){ return vec3(0.5+0.5*cos(6.28318*(t+vec3(0,0.33,0.67)))); }

void main(){
    // Use u_center as both center and also (optionally) the Julia parameter:
    vec2 c = vec2(-0.8, 0.156); // default Julia param; change or map to UI
    // if you want to use u_center as c: c = u_center;
    vec2 z = (uv - vec2(0.5))*u_scale*2.0 + u_center;
    int i;
    for(i=0;i<u_maxIter;i++){
        // z = z^2 + c
        vec2 nz = vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y) + c;
        z = nz;
        if(dot(z,z) > 4.0) break;
    }
    float it = float(i);
    if(i == u_maxIter) FragColor = vec4(0.0);
    else {
        float mu = it + 1.0 - log(log(length(z)))/log(2.0);
        FragColor = vec4(palette(mu/float(u_maxIter)),1.0);
    }
}

