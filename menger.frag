#version 330 core
uniform vec2 u_center;   // can be reused for camera control (optional)
uniform float u_scale;   // zoom-like (distance scaling)
uniform int u_maxIter;   // used as raymarch steps
in vec2 uv;
out vec4 FragColor;

// --- camera / ray setup ---
vec3 cameraDir(vec2 uv, float zoom){
    // uv in [0,1] → [-1,1] range
    vec2 p = (uv - 0.5) * 2.0;
    return normalize(vec3(p, zoom));
}

// --- SDF for Menger sponge ---
float mengerSDF(vec3 p){
    float scale = 3.0;
    float d = length(p);
    float minDist = 1e9;
    for(int i=0; i<6; i++){
        p = abs(p);
        if(p.x < p.y) p.xy = p.yx;
        if(p.x < p.z) p.xz = p.zx;
        if(p.y < p.z) p.yz = p.zy;
        p = scale*p - vec3(scale-1.0);
        d = min(d, (length(p) - 1.0) / pow(scale,float(i+1)));
    }
    return d;
}

// --- normal via gradient ---
vec3 getNormal(vec3 p){
    float eps = 0.001;
    vec2 h = vec2(1.0,-1.0)*0.5773;
    return normalize( h.xyy*mengerSDF(p+h.xyy*eps) +
                      h.yyx*mengerSDF(p+h.yyx*eps) +
                      h.yxy*mengerSDF(p+h.yxy*eps) +
                      h.xxx*mengerSDF(p+h.xxx*eps) );
}

// --- main raymarch loop ---
void main(){
    // camera
    vec3 ro = vec3(0.0,0.0,4.0);         // ray origin
    vec3 rd = cameraDir(uv, 1.5);        // ray dir
    rd.xy *= u_scale;                    // crude zoom with u_scale

    float t = 0.0;
    float dist;
    bool hit = false;
    for(int i=0; i<u_maxIter; i++){
        vec3 pos = ro + rd*t;
        dist = mengerSDF(pos);
        if(dist < 0.001){ hit = true; break; }
        t += dist;
        if(t > 20.0) break; // escape
    }

    if(hit){
        vec3 pos = ro + rd*t;
        vec3 n = getNormal(pos);
        vec3 lightDir = normalize(vec3(0.6,0.7,0.5));
        float diff = max(dot(n,lightDir), 0.0);
        vec3 col = vec3(0.2,0.4,0.8)*diff + vec3(0.1);
        FragColor = vec4(col,1.0);
    } else {
        FragColor = vec4(0.0); // background
    }
}

