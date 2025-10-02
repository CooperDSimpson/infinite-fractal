#version 330 core
uniform vec2 u_center; // optionally used as c
uniform float u_scale;
uniform int u_maxIter;
in vec2 uv;
out vec4 FragColor;

vec3 pal(float t){ return vec3(t, t*t, 1.0 - t); }

void main(){
    vec2 c = vec2(-0.4, 0.6); // tweakable
    vec2 z = (uv - vec2(0.5))*u_scale*2.0 + u_center;
    int i;
    for(i=0;i<u_maxIter;i++){
        // perpendicular Julia: z = (|Re(z)| + i*Im(z))^2 + c
        vec2 w = vec2(abs(z.x), z.y);
        vec2 nz = vec2(w.x*w.x - w.y*w.y, 2.0*w.x*w.y) + c;
        z = nz;
        if(dot(z,z) > 4.0) break;
    }
    if(i==u_maxIter) FragColor = vec4(0.0);
    else {
        float mu = float(i) + 1.0 - log(log(length(z)))/log(2.0);
        FragColor = vec4(pal(mu/float(u_maxIter)),1.0);
    }
}

