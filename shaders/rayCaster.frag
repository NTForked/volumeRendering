#version 130

uniform sampler2D u_frontFaceTexture;
uniform sampler2D u_backFaceTexture;
uniform sampler3D u_volumeTexture;

uniform int u_steps;
uniform vec4 u_transferColor1;
uniform vec4 u_transferColor2;
uniform vec4 u_transferColor3;
uniform vec4 u_transferColor4;
uniform float u_transferPoint1;
uniform float u_transferPoint2;
uniform float u_transferPoint3;
uniform float u_transferPoint4;

uniform int u_invert;
uniform int u_MIP;
uniform vec4 u_MIP_Color;

in vec2 quadTex;

vec4 mapToColor(float intensity)
{
    vec4 transferColor1 = u_transferColor1;
    vec4 transferColor2 = u_transferColor2;
    vec4 transferColor3 = u_transferColor3;
    vec4 transferColor4 = u_transferColor4;
    float transferPoint1 = u_transferPoint1;
    float transferPoint2 = u_transferPoint2;
    float transferPoint3 = u_transferPoint3;
    float transferPoint4 = u_transferPoint4;

    if (intensity <= transferPoint1)
    {
        return transferColor1;
    }
    else if (intensity <= transferPoint2)
    {
        intensity = intensity / (transferPoint2 - transferPoint1);
        return mix(transferColor1,transferColor2,intensity);
    }
    else if (intensity <= transferPoint3)
    {
        intensity = intensity / (transferPoint3 - transferPoint2);
        return mix(transferColor2,transferColor3,intensity);
    }
    else if (intensity <= transferPoint4)
    {
        intensity = intensity / (transferPoint4 - transferPoint3);
        return mix(transferColor3,transferColor4,intensity);
    }
    else
    {
        return transferColor4;
    }
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
    vec4 maximumColor;
    vec4 finalColor;
    float depth;

    float samples[1000];
    float depths[1000];


    for(int i = 0; i < numSteps; i++){
        ray = ray + stepSize;
        currentColor = texture(u_volumeTexture, ray.xyz);
        currentIntensity = currentColor.r;
        samples[i] = currentIntensity;
        depth = distance(ray.xyz, startingPoint.xyz) / distance(rayDirection.xyz, startingPoint.xyz);
        depths[i] = depth;

        if(maximumIntensity < currentIntensity){
            maximumIntensity = currentIntensity;
            maximumColor = vec4(u_MIP_Color.xyz,depth);
        }
    }
    /*mix(one,second,alpha);*/
    
    vec4 mixedColor = vec4(0.0,0.0,0.0,0.0);

    for(int i=0;i<numSteps;i++)
    {
        mixedColor = mix(mixedColor, mapToColor(samples[i]), samples[i]);
    }

    if(u_MIP > 0)
        finalColor = maximumColor;
    else
        finalColor = mixedColor;

    if(u_invert > 0)
        gl_FragColor = 1 - finalColor;
    else
        gl_FragColor = finalColor;

}
