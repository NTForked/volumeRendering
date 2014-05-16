#version 130

uniform sampler2D u_frontFaceTexture;
uniform sampler2D u_backFaceTexture;
uniform sampler3D u_volumeTexture;
uniform int u_steps;

in vec4 v_position;
in vec2 quadTex;

vec4 fromColor(vec4 tex)
{
    tex = (tex * 0.5) + 0.5;
    /*tex = (tex / 0.5) - 0.5;*/
    return tex;
}

vec4 mapToColor(float intensity)
{
    if (intensity < 0.3)
        return mix(vec4(1.0,0.0,0.0,0.3),vec4(1.0,1.0,1.0,0.3),intensity);
    if (intensity > 0.7)
        return vec4(1.0,1.0,1.0,1.0);
    else
        return mix(vec4(1.0,0.0,0.0,1.0),vec4(1.0,1.0,1.0,1.0),intensity);

}

void main()
{
    vec2 tex = 0.5 * quadTex + 0.5;

    vec4 color;
    vec4 startingPoint = texture(u_frontFaceTexture,tex);
    vec4 endingPoint = texture(u_backFaceTexture,tex);
    vec4 rayDirection = endingPoint - startingPoint;
    vec4 ray = startingPoint;
    float numSteps = u_steps;
    vec4 stepSize = rayDirection / numSteps; 

    float maximumIntensity = 0.0;
    float currentIntensity;

    vec4 currentColor;
    vec4 maximumColor = vec4(0.0,0.0,0.0,0.0);

    float samples[1000];


    float depth;

    for(int i = 0; i < numSteps; i++){
        ray = ray + stepSize;
        currentColor = texture(u_volumeTexture, ray.xyz);
        currentIntensity = currentColor.r;

        samples[i] = currentIntensity;

        depth = distance(ray.xyz, startingPoint.xyz) / distance(rayDirection.xyz, startingPoint.xyz);

        if(maximumIntensity < currentIntensity){
            maximumIntensity = currentIntensity;
            maximumColor = vec4(currentIntensity, 0.0,0.0,depth);
        }
    }

    /*mix(one,second,alpha);*/
    
    vec4 mixedColor = vec4(0.0,0.0,0.0,1.0);

    for(int i=0;i<numSteps;i++)
    {
        mixedColor = mix(mixedColor, mapToColor(samples[i]), samples[i]);
    }

    gl_FragColor = mixedColor;

}
