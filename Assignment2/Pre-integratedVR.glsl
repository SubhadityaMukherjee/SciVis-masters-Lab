// Bounding box
const vec3 bbMin = vec3(-0.5, -0.5, -0.5);
const vec3 bbMax = vec3(0.5, 0.5, 0.5);

// Additional camera parameters
const float fovy = 45.0;
const float zNear = 0.1;

// Number of maximum raycasting samples per ray
const int sampleNum = 150;

// Width of one voxel
const float voxelWidth = 1.0 / 64.0;

// Epsilon for comparisons
const float EPS = 0.000001;
const float PI = 3.14159267;
const float period = 2.0;
// Sigma for the gaussian function
const float sig = 0.4;

// TODO: add pre-integrated transfer function that can be directly used
vec4 preintegratedTransferFunction(float value)
{
    return texture(iChannel0, vec2(value, value));
}

vec2 csqr(vec2 a)
{
    return vec2(a.x*a.x - a.y*a.y, 2.*a.x*a.y); 
}

float ABCvolume(vec3 texCoord)
{
    vec3 center1 = vec3(0.25);
    vec3 center2 = vec3(0.75);
    
    float dist1 = length(texCoord - center1);
    float dist2 = length(texCoord - center2);
    float phase1 = 0.5 * 3.14;
    float phase2 = 0.3 * 3.14;
    
    float intensity1 = 0.35 * (sin(14.0 * dist1 - 5.0 * iTime * 1.0 + phase1) + 1.0);
    float intensity2 = 0.25 * (sin(16.0 * dist2 - 5.0 * iTime * 1.0 + phase1) + 1.0);
    
    return intensity1 + intensity2;
}

/**
 * Samples the volume texture at a given position.
 *
 * @param volumeCoord The position one wants to retrieve the sample of (in world coordinates).
 * @return The sample value at the given position.
 */
float sampleVolume(vec3 texCoord)
{
    return ABCvolume(texCoord);
}

/**
 * Intersects a ray with the bounding box and returs the intersection points
 *  
 * @param rayOrig The origin of the ray
 * @param rayDir The direction of the ray
 * @param tNear OUT: The distance from the ray origin to the first intersection point
 * @param tFar OUT: The distance from the ray origin to the second intersection point
 * @return True if the ray intersects the bounding box, false otherwise.
 */
bool intersectBoundingBox(vec3 rayOrig, vec3 rayDir, out float tNear, out float tFar)
{
    vec3 invR = vec3(1.0) / rayDir;
    vec3 tbot = invR * (bbMin - rayOrig);
    vec3 ttop = invR * (bbMax - rayOrig);
    
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    
    float largestTMin = max(max(tmin.x, tmin.y), max(tmin.x, tmin.z));
    float smallestTMax = min(min(tmax.x, tmax.y), min(tmax.x, tmax.z));
    
    tNear = largestTMin;
    tFar = smallestTMax;
    
    return (smallestTMax > largestTMin);
}

/**
 * Correct opacity for the current sampling rate
 *
 * @param alpha: input opacity.
 * @param samplingRatio: the ratio between current sampling rate and the original.
 */
float opacityCorrection(in float alpha, in float samplingRatio)
{
    float a_corrected = 1.0 - pow(1.0 - alpha, samplingRatio);
    return a_corrected;
}

/**
 * Accumulation composition
 *
 * @param sample: current sample value.
 * @param samplingRatio: the ratio between current sampling rate and the original. (ray step)
 * @param composedColor: blended color (both input and output)
 */
void accumulation(float value, float sampleRatio, inout vec4 composedColor)
{
    vec4 color = preintegratedTransferFunction(value); // color_CUR
    color.a = opacityCorrection(color.a, sampleRatio); // alpha_CUR

    // DONE: Implement Front-to-back blending
    float alpha_i = composedColor.a;
    
    vec3 color_new = composedColor.xyz + (1.0 - alpha_i) * color.xyz * color.a;
    float alpha_new = alpha_i + (1.0 - alpha_i) * color.a;
    
    composedColor = vec4(color_new, alpha_new);
}

/**
 * Main Function: Computes the color for the given fragment.
 *
 * @param fragColor OUT: The color of the pixel / fragment.
 * @param fragCoord The coordinate of the fragment in screen space
 */
void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    float aspect = iResolution.x / iResolution.y;
    
    /******************** compute camera parameters ********************/
    
    // camera movement  
    float camSpeed = 0.5;
    vec3 camPos = 1.5 * vec3(cos(iTime*camSpeed), 0.5, sin(iTime*camSpeed));
    vec3 camDir = -normalize(camPos);
    vec3 camUp = vec3(0.0, 1.0, 0.0);
    vec3 camRight = normalize(cross(camDir, camUp));
    camUp = normalize(cross(camRight, camDir));
    
    /************ compute ray direction (OpenGL style) *****************/
    vec2 myUV = 2.0 * uv - 1.0;
    float fovx = 2.0 * atan(tan(fovy / 2.0) * aspect);
    
    vec3 uL = (tan(fovx*0.5)*zNear) * (-camRight) + (tan(fovy*0.5) * zNear) * camUp + camDir * zNear + camPos;
    vec3 lL = (tan(fovx*0.5)*zNear) * (-camRight) + (tan(fovy*0.5) * zNear) * (-camUp) + camDir * zNear + camPos;
    vec3 uR = (tan(fovx*0.5)*zNear) * camRight + (tan(fovy*0.5) * zNear) * camUp + camDir * zNear + camPos;
    vec3 lR = (tan(fovx*0.5)*zNear) * camRight + (tan(fovy*0.5) * zNear) * (-camUp) + camDir * zNear + camPos;
    
    vec3 targetL = mix(lL, uL, uv.y);
    vec3 targetR = mix(lR, uR, uv.y);
    vec3 target = mix(targetL, targetR, uv.x);
    
    vec3 rayDir = normalize(target - camPos);

    /******************* test against bounding box ********************/
    float tNear, tFar;
    bool hit = intersectBoundingBox(camPos, rayDir, tNear, tFar);
    vec4 background = vec4(0.1, 0.2, 0.4, 1.0);
    if (!hit)
    {
       fragColor = background;
        return;
    }

    float tstep = (bbMax.x - bbMin.x) / float(sampleNum);
    vec4 finalColor = vec4(0.0);
    vec3 finalGradient = vec3(0.0);
    // ratio between current sampling rate vs. the original sampling rate
    float sampleRatio = 1.0 / (float(sampleNum) * voxelWidth);
    /******************** main raycasting loop *******************/
    
    // For maximum intensity composition
    float maxIntense = 0.0;
    
    // For average intensity composition
    float sumIntense = 0.0;
    int   hitCount = 0;
    
    float t = tNear;
    int i = 0;
    while(t < tFar && i < sampleNum)
    {
        vec3 pos = camPos + t * rayDir;
        // Use normalized volume coordinate
        vec3 texCoord = vec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5);
        float value = sampleVolume(texCoord);
        
        // DONE: Modify to use the pre-integration table you generated in the previous sub-task.
        accumulation(value, sampleRatio, finalColor);
        
        t += tstep;
    }

    fragColor = finalColor * finalColor.a + (1.0 - finalColor.a) * background;
}
