//
// Book:      OpenGL(R) ES 2.0 Programming Guide
// Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
// ISBN-10:   0321502795
// ISBN-13:   9780321502797
// Publisher: Addison-Wesley Professional
// URLs:      http://safari.informit.com/9780321563835
//            http://www.opengles-book.com
//

// ParticleSystem.c
//
//    This is an example that demonstrates rendering a particle system
//    using a vertex shader and point sprites.
//
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "esUtil.h"
#include "glesTools.h"
#include "glesVMath.h"

#define NUM_PARTICLES	16
#define PARTICLE_SIZE   4
#define RENDER_TO_TEX_WIDTH 256
#define RENDER_TO_TEX_HEIGHT 256
#define POINT_RADIUS 0.02f

struct Ship
{
    GLint numVertices;
    GLfloat *v;
};

struct Quad
{
    GLint elementsSize;
    GLfloat *v;
    GLshort *e;
};

typedef struct
{
    // ===========Particles=========== //

    // Particles Handle to a program object
    GLuint particlesProgram;

    // Particles Attribute locations
    GLint  particlesLifetimeLoc;
    GLint  particlesStartPositionLoc;
    //GLint  particlesVelPositionLoc;

    // Particles Uniform location
    GLint particlesTimeLoc;
    GLint particlesColorLoc;
    GLint particlesCenterPositionLoc;
    GLint particlesSamplerLoc;
    GLint particlesPointSize;

    // Particles Texture handle
    GLuint particlesTextureId;

    // Particles vertex data
    float particleData[ NUM_PARTICLES * PARTICLE_SIZE ];

    // Current time
    float time;

    // Particles perspective matrix
    ESMatrix particlesMVP;
    GLint particlesMVPLoc;

    // =============Ship============= //

    // Ship Handle to a program object
    GLint shipProgram;

    // Ship Attribute locations
    GLint shipPositionLoc;

    // Ship Unifrom location
    GLint shipStartPositionLoc;
    GLint shipColorLoc;

    // Ship perspective matrix
    ESMatrix shipMVP;
    GLint shipMVPLoc;

    // Ship struct
    struct Ship * ship;

    // =========renderToTex========= //

    GLuint renderToTexFramebuffer;
    GLuint renderToTexDepthRenderBuffer;
    GLuint renderToTexTexture;
    GLint renderToTexTexWidth;
    GLint renderToTexTexHeight;

    // ============Quad============ //

    // Quad Handle to a program object
    GLint quadProgram;

    // Quad Attribute locations
    GLint quadPositionLoc;
    GLint quadTexCoord;

    // Quad Unifrom locations
    GLint quadSamplerLoc;

    // Quad Texture handle
    GLuint quadTextureId;

    // Quad struct
    struct Quad * quad;

} UserData;

///
// Load texture from disk
//
GLuint LoadTexture ( char *fileName )
{
    int width,
        height;
    char *buffer = esLoadTGA ( fileName, &width, &height );
    GLuint texId;

    if ( buffer == NULL )
    {
        esLogMessage ( "Error loading (%s) image.\n", fileName );
        return 0;
    }

    glGenTextures ( 1, &texId );
    glBindTexture ( GL_TEXTURE_2D, texId );

    glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    free ( buffer );

    return texId;
}

int InitFBO ( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    GLint maxRenderBufferSize;

    glGetIntegerv ( GL_MAX_RENDERBUFFER_SIZE, &maxRenderBufferSize );
    if ( (maxRenderBufferSize <= userData->renderToTexTexWidth) ||
         (maxRenderBufferSize <= userData->renderToTexTexHeight) ) {
        fprintf(stderr, "Error.  GL_MAX_RENDERBUFFER_SIZE = %d\n", maxRenderBufferSize);
        return FALSE;
    }

    glGenFramebuffers(1, &userData->renderToTexFramebuffer);
    //glGenRenderbuffers(1, &userData->renderToTexDepthRenderBuffer);
    glGenTextures(1, &userData->renderToTexTexture);

    glBindTexture(GL_TEXTURE_2D, userData->renderToTexTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
            userData->renderToTexTexWidth,
            userData->renderToTexTexHeight, 0, GL_RGB,
            GL_UNSIGNED_SHORT_5_6_5, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    return TRUE;
}

int InitParticles ( ESContext *esContext )
{
    UserData *userData = esContext->userData;
    int i;

    char * vShaderStr = loadShader( "shader/billiards.vert" );
    char * fShaderStr = loadShader( "shader/billiards.frag" );

    // Load the shaders and get a linked program object
    userData->particlesProgram = esLoadProgram ( vShaderStr, fShaderStr );
    free(vShaderStr);
    free(fShaderStr);

    // Get the attribute locations
    userData->particlesLifetimeLoc = glGetAttribLocation ( userData->particlesProgram, "a_lifetime" );
    userData->particlesStartPositionLoc = glGetAttribLocation ( userData->particlesProgram, "a_startPosition" );
    //userData->particlesVelPositionLoc = glGetAttribLocation ( userData->particlesProgram, "a_endPosition" );

    // Get the uniform locations
    userData->particlesTimeLoc = glGetUniformLocation ( userData->particlesProgram, "u_time" );
    userData->particlesCenterPositionLoc = glGetUniformLocation ( userData->particlesProgram, "u_centerPosition" );
    userData->particlesColorLoc = glGetUniformLocation ( userData->particlesProgram, "u_color" );
    userData->particlesSamplerLoc = glGetUniformLocation ( userData->particlesProgram, "s_texture" );
    userData->particlesPointSize = glGetUniformLocation ( userData->particlesProgram, "u_pointSize" );
    // Fill in particle data array
    srand ( 0 );
    float ballSize = 0.03f;
    float poolPts [] = {
              -0.5,        0.0f, // white ball

              0.0f,        0.0f, // row 1

          ballSize,   -ballSize, // row 2
          ballSize,    ballSize,

        2*ballSize, -2*ballSize, // row 3
        2*ballSize,        0.0f,
        2*ballSize,  2*ballSize,

        3*ballSize, -3*ballSize, // row 4
        3*ballSize,   -ballSize,
        3*ballSize,    ballSize,
        3*ballSize,  3*ballSize,

        4*ballSize, -4*ballSize, // row 5
        4*ballSize, -2*ballSize,
        4*ballSize,        0.0f,
        4*ballSize,  2*ballSize,
        4*ballSize,  4*ballSize,
    };
    GLfloat *pt = &poolPts[0];
    for ( i = 0; i < NUM_PARTICLES; i++ )
    {
        GLfloat *particleData = &userData->particleData[i * PARTICLE_SIZE];

        // Start position of particle
        //(*particleData++) = ( (float)(rand() % 10000) / 40000.0f ) - 0.125f;
        //(*particleData++) = ( (float)(rand() % 10000) / 40000.0f ) - 0.125f;
        (*particleData++) = (*pt++);
        (*particleData++) = (*pt++);

        // Velocities
        //float v[2] = { 2 * randFloat() - 1, 2 * randFloat() - 1 };
        //normalize2f(&v[0]);
        //(*particleData++) = v[0];
        //(*particleData++) = v[1];
        if ( i == 0 ) {
            (*particleData++) = 0.2f;
            (*particleData++) = 0.0f;
        } else {
            (*particleData++) = 0.0f;
            (*particleData++) = 0.0f;
        }
    }

    userData->particlesTextureId = LoadTexture ( "texture/smoke.tga" );
    if ( userData->particlesTextureId <= 0 )
    {
        return FALSE;
    }

    // Set up the perspective matrix.
    userData->particlesMVPLoc = glGetUniformLocation ( userData->particlesProgram, "u_MVP" );

    // Generate a model view matrix to rotate/translate the cube
    ESMatrix modelview;
    ESMatrix perspective;

    esMatrixLoadIdentity( &modelview );

    // Translate away from the viewer
    esTranslate( &modelview, 0.0, 0.0, -2.0 );

    esMatrixLoadIdentity( &perspective );
    esPerspective(&perspective, 60.0f, (float)esContext->width /
                  (float)esContext->height, 1.0f, 20.0f);
    esMatrixMultiply( &userData->particlesMVP, &modelview, &perspective );

    glUseProgram ( userData->particlesProgram );

    float centerPos[2];
    float color[4];

    // Pick a new start location and color
    centerPos[0] = ( (float)(rand() % 10000) / 10000.0f ) - 0.5f;
    centerPos[1] = ( (float)(rand() % 10000) / 10000.0f ) - 0.5f;

    glUniform3fv ( userData->particlesCenterPositionLoc, 1, &centerPos[0] );

    // Random color
    color[0] = ( (float)(rand() % 10000) / 20000.0f ) + 0.5f;
    color[1] = ( (float)(rand() % 10000) / 20000.0f ) + 0.5f;
    color[2] = ( (float)(rand() % 10000) / 20000.0f ) + 0.5f;
    color[3] = 1.0;

    glUniform4fv ( userData->particlesColorLoc, 1, &color[0] );
    // TODO: Make this a function of resolution and remove POINT_RADIUS
    glUniform1f ( userData->particlesPointSize, 20.0f );

    return TRUE;
}

int InitQuad( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    // Make the program
    char *vShaderStr = loadShader("shader/quad.vert");
    char *fShaderStr = loadShader("shader/quad.frag");
    userData->quadProgram = esLoadProgram ( vShaderStr, fShaderStr );
    free(vShaderStr);
    free(fShaderStr);

    // Get uniform locations
    userData->quadSamplerLoc = glGetUniformLocation( userData->quadProgram,
            "s_texture" );

    // Get attribute locations
    userData->quadPositionLoc = glGetAttribLocation ( userData->quadProgram,
            "a_position" );
    userData->quadTexCoord = glGetAttribLocation ( userData->quadProgram,
            "a_texCoord" );
    //userData->quadTextureId = CreateSimpleTexture2D();

    return TRUE;
}

///
// Initialize the shader and program object
//
int Init ( ESContext *esContext )
{
    UserData *userData = esContext->userData;
    if ( !InitParticles(esContext) ) {
        return FALSE;
    }
    //if ( !InitQuad(esContext) ) {
    //    return FALSE;
    //}
    //if ( !InitFBO(esContext) ) {
    //    return FALSE;
    //}
    glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );

    userData->time = 0.0f;
    return TRUE;
}

void RewindToImpact(GLfloat *pos1, GLfloat *pos2)
{
    if ( pos1 == pos2 ) {
        fprintf(stderr, "RewindToImpact Error: pos1 == pos2\n");
        return;
    }

    GLfloat tmpPos1[2];
    GLfloat tmpPos2[2];

    GLfloat tmpVec[2];

    GLfloat initialDiff;
    GLfloat impactDistanceSquared = 4*POINT_RADIUS*POINT_RADIUS;
    distanceSquared(&initialDiff, &pos1[0], &pos2[0], 2);

    // Copy positions from points to tmpPos.
    memcpy(&tmpPos1[0], &pos1[0], sizeof(GLfloat) * 2);
    memcpy(&tmpPos2[0], &pos2[0], sizeof(GLfloat) * 2);

    GLfloat initialRewindFactor = 0.001f; // arbitrary small step.

    // Take the first small step.
    scale(&tmpVec[0], &pos1[2], initialRewindFactor, 2);
    UMinusV(&tmpPos1[0], &pos1[0], &tmpVec[0], 2);
    scale(&tmpVec[0], &pos2[2], initialRewindFactor, 2);
    UMinusV(&tmpPos2[0], &pos2[0], &tmpVec[0], 2);

    GLfloat secondDiff;
    distanceSquared(&secondDiff, &tmpPos1[0], &tmpPos2[0], 2);
    //printf("initial diff:          %f\n", initialDiff / impactDistanceSquared);
    //printf("second diff :          %f\n", secondDiff / impactDistanceSquared);
    //printf("impactDistanceSquared: %f\n", impactDistanceSquared);
    //printf("stepPercent:           %f\n", (secondDiff - initialDiff) / impactDistanceSquared);
    //printf("%f\n\n", (1.0f - (secondDiff / impactDistanceSquared)) / ((secondDiff - initialDiff) / impactDistanceSquared));

    // Calculate how many more steps to take.
    GLfloat numSteps = (1.0f - (secondDiff / impactDistanceSquared)) /
            ((secondDiff - initialDiff) / impactDistanceSquared);

    numSteps += 400.0f; // TODO: bad bad fudge factor

    scale(&tmpVec[0], &pos1[2], numSteps * (fabs(secondDiff - initialDiff)),
            2);
    UMinusV(&tmpPos1[0], &pos1[0], &tmpVec[0], 2);
    scale(&tmpVec[0], &pos2[2], numSteps * (fabs(secondDiff - initialDiff)),
            2);
    UMinusV(&tmpPos2[0], &pos2[0], &tmpVec[0], 2);

    //printf("Moving (%f, %f) to (%f, %f)\n", pos1[0], pos1[1], tmpPos1[0], tmpPos1[1]);
    //printf("Moving (%f, %f) to (%f, %f)\n\n", pos2[0], pos2[1], tmpPos2[0], tmpPos2[1]);
    memcpy(&pos1[0], &tmpPos1[0], sizeof(GLfloat) * 2);
    memcpy(&pos2[0], &tmpPos2[0], sizeof(GLfloat) * 2);
}

void Collision(GLfloat *pos1, GLfloat *pos2)
{
    if (pos1 == pos2) {
        fprintf(stderr, "Collision Error: pos1 == pos2\n");
        return;
    }
    GLfloat newVel1[2];
    GLfloat newVel2[2];
    GLfloat dir1[2];
    GLfloat dir2[2];
    GLfloat tmpVec[2];

    UMinusV(&dir1[0], &pos2[0], &pos1[0], 2);
    UMinusV(&dir2[0], &pos1[0], &pos2[0], 2);

    memcpy(&newVel1[0], &pos1[2], sizeof(GLfloat) * 2);
    memcpy(&newVel2[0], &pos2[2], sizeof(GLfloat) * 2);

    projectUonV2f(&tmpVec[0], &pos2[2], &dir1[0]);
    UPlusV(&newVel1[0], &newVel1[0], &tmpVec[0], 2);
    projectUonV2f(&tmpVec[0], &pos1[2], &dir2[0]);
    UMinusV(&newVel1[0], &newVel1[0], &tmpVec[0], 2);

    projectUonV2f(&tmpVec[0], &pos1[2], &dir1[0]);
    UPlusV(&newVel2[0], &newVel2[0], &tmpVec[0], 2);
    projectUonV2f(&tmpVec[0], &pos2[2], &dir2[0]);
    UMinusV(&newVel2[0], &newVel2[0], &tmpVec[0], 2);

    memcpy(&pos1[2], &newVel1[0], sizeof(GLfloat) * 2);
    memcpy(&pos2[2], &newVel2[0], sizeof(GLfloat) * 2);
}

void CheckForCollisions ( GLfloat *particleData )
{
    int i;
    for ( i = 0 ; i < NUM_PARTICLES ; ++i ) {
        int j;
        GLfloat *point1 = &particleData[i * PARTICLE_SIZE];
        for ( j = i+1 ; j < NUM_PARTICLES ; ++j ) {
            GLfloat *point2 = &particleData[j * PARTICLE_SIZE];
            GLfloat diff1 = point1[0] - point2[0];
            GLfloat diff2 = point1[1] - point2[1];

            if (diff1*diff1 + diff2*diff2 <= 4*POINT_RADIUS*POINT_RADIUS) {
                //printf("Collision: (%f, %f), (%f, %f)\n", point1[0], point1[1],
                //        point2[0], point2[1]);
                RewindToImpact(point1, point2);
                Collision(point1, point2);
            }
        }
    }
}

void UpdatePositions ( ESContext *esContext, float deltaTime )
{
    UserData *userData = esContext->userData;
    GLfloat *particleData = &userData->particleData[0];
    CheckForCollisions( particleData );
    {
        int i;
        for ( i = 0 ; i < NUM_PARTICLES ; ++i ) {
            particleData = &userData->particleData[i * PARTICLE_SIZE];

            particleData[0] += particleData[2] * deltaTime;
            //particleData[0] = particleData[0] > 1.0f ? -1.0f : particleData[0];
            //particleData[0] = particleData[0] < -1.0f ? 1.0f : particleData[0];

            particleData[1] += particleData[3] * deltaTime;
            //particleData[1] = particleData[1] > 1.0f ? -1.0f : particleData[1];
            //particleData[1] = particleData[1] < -1.0f ? 1.0f : particleData[1];
        }
    }
}

///
//  Update time-based variables
//
void Update ( ESContext *esContext, float deltaTime )
{
    UserData *userData = esContext->userData;

    userData->time += deltaTime;
    // Load uniform time variable

    glUseProgram ( userData->particlesProgram );
    glUniform1f ( userData->particlesTimeLoc, userData->time );
    UpdatePositions( esContext, deltaTime );
}

void DrawParticles ( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    // Use the program object
    glUseProgram ( userData->particlesProgram );

    //glVertexAttribPointer ( userData->particlesVelPositionLoc, 2, GL_FLOAT,
    //        GL_FALSE, PARTICLE_SIZE * sizeof(GLfloat),
    //        &userData->particleData[0] );

    glVertexAttribPointer ( userData->particlesStartPositionLoc, 2, GL_FLOAT,
            GL_FALSE, PARTICLE_SIZE * sizeof(GLfloat),
            &userData->particleData[0] );


    glEnableVertexAttribArray ( userData->particlesLifetimeLoc );
    //glEnableVertexAttribArray ( userData->particlesVelPositionLoc );
    glEnableVertexAttribArray ( userData->particlesStartPositionLoc );
    // Blend particles
    glEnable ( GL_BLEND );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

    // Bind the texture
    glActiveTexture ( GL_TEXTURE0 );
    glBindTexture ( GL_TEXTURE_2D, userData->particlesTextureId );
    glEnable ( GL_TEXTURE_2D );

    // Set the sampler texture unit to 0
    glUniform1i ( userData->particlesSamplerLoc, 0 );

    glUniformMatrix4fv(userData->particlesMVPLoc, 1, GL_FALSE,
                       &userData->particlesMVP.m[0][0]);

    //glBindFramebuffer( GL_FRAMEBUFFER, userData->renderToTexFramebuffer );
    //glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
    //        GL_TEXTURE_2D, userData->renderToTexTexture, 0 );
    //glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
    //        GL_RENDERBUFFER, userData->renderToTexDepthRenderBuffer );

    //GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    //if ( status == GL_FRAMEBUFFER_COMPLETE ) {
    //    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //    glViewport ( 0, 0, RENDER_TO_TEX_WIDTH, RENDER_TO_TEX_HEIGHT );
    //    glDrawArrays( GL_POINTS, 0, NUM_PARTICLES );
    //    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    //    glViewport ( 0, 0, esContext->width, esContext->height );
    //}
    glDrawArrays( GL_POINTS, 0, NUM_PARTICLES );
}

void DrawQuad( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    glUseProgram( userData->quadProgram );

    glEnableVertexAttribArray ( userData->quadPositionLoc );
    glEnableVertexAttribArray ( userData->quadTexCoord );

    glVertexAttribPointer ( userData->quadPositionLoc, 2, GL_FLOAT, GL_FALSE,
            4 * sizeof(GLfloat), &userData->quad->v[0] );

    glVertexAttribPointer ( userData->quadTexCoord, 2, GL_FLOAT, GL_FALSE, 4 *
            sizeof(GLfloat), &userData->quad->v[2] );

    // TODO: replace with renderToTexTexture.
    glActiveTexture ( GL_TEXTURE0 );
    glBindTexture ( GL_TEXTURE_2D, userData->renderToTexTexture );
    glEnable ( GL_TEXTURE_2D );

    // Set the sampler texture unit to 0
    glUniform1i ( userData->quadSamplerLoc, 0 );

    //glDrawArrays( GL_TRIANGLES, 0, userData->quad->numVertices );
    glDrawElements ( GL_TRIANGLES, userData->quad->elementsSize,
            GL_UNSIGNED_SHORT, &userData->quad->e[0] );
}

int ReadPixels( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    GLenum readType, readFormat;
    GLubyte *pixels;

    glBindFramebuffer( GL_FRAMEBUFFER, userData->renderToTexFramebuffer );
    glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE, &readType);
    glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT, &readFormat);

    unsigned int bytesPerPixel = 0;

    switch (readType) {
        case GL_UNSIGNED_BYTE:
            switch (readFormat) {
                case GL_RGBA:
                    bytesPerPixel = 4;
                    break;
                case GL_RGB:
                    bytesPerPixel = 3;
                    break;
                case GL_LUMINANCE_ALPHA:
                    bytesPerPixel = 2;
                    break;
                case GL_ALPHA:
                case GL_LUMINANCE:
                    bytesPerPixel = 1;
                    break;
            }
            break;
            //case GL_UNSIGNED_SHORT_4444:
            //case GL_UNSIGNED_SHORT_555_1:
            //case GL_UNSIGNED_SHORT_565:
            default:
                bytesPerPixel = 2;
                break;
    }
    pixels = (GLubyte*) malloc(RENDER_TO_TEX_WIDTH * RENDER_TO_TEX_HEIGHT *
            bytesPerPixel);
    glReadPixels(0, 0, RENDER_TO_TEX_WIDTH, RENDER_TO_TEX_HEIGHT, readFormat,
            readType, pixels );
    int retval = FALSE;
    {
        int i;
        for( i = 0 ; i < (RENDER_TO_TEX_WIDTH * RENDER_TO_TEX_HEIGHT) / 4 ; ++i ){
            //printf("%d %d %d %d\n", pixels[4*i], pixels[4*i+1], pixels[4*i+2],
            //        pixels[4*i+3]);
            if (pixels[4*i] == 255 && pixels[4*i+1] == 0 && pixels[4*i+2] == 0) {
                retval = TRUE;
            }
        }
    }
    free(pixels);
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    return retval;
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw ( ESContext *esContext )
{
    //UserData *userData = esContext->userData;

    // Set the viewport for Particles
    glViewport ( 0, 0, esContext->width, esContext->height );

    // Clear the color buffer
    glClear ( GL_COLOR_BUFFER_BIT );

    DrawParticles( esContext );
    //int hasRedPix = ReadPixels( esContext );
    //printf("%d\n", hasRedPix);
    //DrawQuad( esContext );
}

///
// Cleanup
//
void ShutDown ( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    // Delete texture object
    glDeleteTextures ( 1, &userData->particlesTextureId );
    glDeleteTextures ( 1, &userData->quadTextureId );

    // Delete program object
    glDeleteProgram ( userData->particlesProgram );
}

int main ( int argc, char *argv[] )
{
    ESContext esContext;
    UserData  userData;

    // Setup the Ship struct
    struct Ship ship;
    ship.numVertices = 2 * 3;
    GLfloat vShip[] = {
        -0.025f, -0.025f,
         0.025f, -0.025f,
           0.0f,  0.05f
    };
    ship.v = &vShip;
    userData.ship = &ship;

    // Setup the Quad struct
    struct Quad quad;
    quad.elementsSize = 6;
    GLfloat vQuad[] = {
         -1.0f, -1.0f,
          0.0f,  0.0f,

          1.0f, -1.0f,
          1.0f,  0.0f,

          1.0f,  1.0f,
          1.0f,  1.0f,

         -1.0f,  1.0f,
          0.0f,  1.0f,
    };
    quad.v = &vQuad;

    GLushort eQuad[] = {
        0, 1, 2, 0, 2, 3
    };
    quad.e = &eQuad;

    userData.quad = &quad;

    // Setup renderToTexTexWidth/Height
    userData.renderToTexTexWidth = RENDER_TO_TEX_WIDTH;
    userData.renderToTexTexHeight = RENDER_TO_TEX_HEIGHT;

    esInitContext ( &esContext );
    esContext.userData = &userData;

    esCreateWindow ( &esContext, "ParticleSystem", 1920, 1080, ES_WINDOW_RGB );

    if ( !Init ( &esContext ) )
        return 0;

    esRegisterDrawFunc ( &esContext, Draw );
    esRegisterUpdateFunc ( &esContext, Update );

    esMainLoop ( &esContext );

    ShutDown ( &esContext );
}
