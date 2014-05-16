#version 130

uniform sampler2D u_frontFaceTexture;
uniform sampler2D u_backFaceTexture;
uniform sampler3D u_volumeTexture;

in vec4 v_position;
in vec2 quadTex;

vec4 fromColor(vec4 tex)
{
    tex = (tex * 0.5) + 0.5;
    /*tex = (tex / 0.5) - 0.5;*/
    return tex;
}

void main()
{
    vec2 tex = 0.5 * quadTex + 0.5;

    vec4 color;
    vec4 startingPoint = texture(u_frontFaceTexture,tex);
    vec4 endingPoint = texture(u_backFaceTexture,tex);
    vec4 rayDirection = endingPoint - startingPoint;
    vec4 ray = startingPoint;
    float numSteps = 10;

    float maximumIntensity = 0.0;
    float currentIntensity;

    vec4 currentColor;
    vec4 maximumColor = vec4(0.0,0.0,0.0,0.0);

    /*divide ray direction*/
    /*radiation attenuation*/
    /*houns field*/

    vec4 stepSize = rayDirection / numSteps; 

    for(int i = 0; i < numSteps; i++){
        ray = ray + stepSize;
        currentColor = texture(u_volumeTexture, ray.xyz);
        currentIntensity = currentColor.r;

        if(maximumIntensity < currentIntensity){
            maximumIntensity = currentIntensity;
            maximumColor = currentColor;
        }
    }

    /*gl_FragColor = texture(u_backFaceTexture,tex);*/
    gl_FragColor = maximumColor;

}

/*void main()*/
/*{*/
    /*vec2 tex = 0.5 * quadTex + 0.5;*/

    /*vec4 color;*/
    /*vec4 startingPoint = fromColor(texture(u_frontFaceTexture,tex));*/
    /*vec4 endingPoint = fromColor(texture(u_backFaceTexture,tex));*/
    /*vec4 rayDirection = endingPoint - startingPoint;*/
    /*vec4 ray = startingPoint;*/
    /*float numSteps = 20;*/

    /*divide ray direction*/

    /*vec4 stepSize = rayDirection / numSteps; */

    /*for(int i = 0; i < numSteps; i++){*/
        /*ray = ray + (rayDirection * stepSize);*/
        /*color = texture(u_volumeTexture, ray.xyz);*/
        /*if (color.x != 0.0 && color.y != 0.0 && color.z != 0.0)*/
            /*break;*/
    /*}*/

    /*gl_FragColor = color;*/
/*}*/
