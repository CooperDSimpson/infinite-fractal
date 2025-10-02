#version 330 core
uniform vec2 u_center;
uniform float u_scale;
uniform int u_maxIter;
in vec2 uv;
out vec4 FragColor;

vec3 pal(float t){ return vec3(t*t, t, 1.0 - t*t); }

void main(){
    vec2 c = (uv - vec2(0.5))*u_scale*2.0 + u_center;
    vec2 z = vec2(0.0);
    int i;
    for(i=0;i<u_maxIter;i++){
        // conjugate square: z = conj(z)^2 + c
        vec2 conjz = vec2(z.x, -z.y);
        vec2 nz = vec2(conjz.x*conjz.x - conjz.y*conjz.y, 2.0*conjz.x*conjz.y) + c;
        z = nz;
        if(dot(z,z) > 4.0) break;
    }
    if(i==u_maxIter) FragColor = vec4(0.0);
    else {
        float mu = float(i) + 1.0 - log(log(length(z)))/log(2.0);
        FragColor = vec4(pal(mu/float(u_maxIter)),1.0);
    }
}

