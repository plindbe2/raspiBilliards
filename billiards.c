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
#include <sys/time.h>
#include "defines.h"
#include <time.h>

#define NUM_PARTICLES	16
#define PARTICLE_SIZE   4 // Has velocity.
#define PARTICLE_QUAD_SIZE 24 // Doesn't have velocity.  Has texture coords.
#define RENDER_TO_TEX_WIDTH 256
#define RENDER_TO_TEX_HEIGHT 256
#define POINT_RADIUS 0.024f
#define POINT_ACCELERATION -0.2f
#define PATICLES_QUAD_HALF_SIDELENGTH .03f
#define TEXTURE_ATLAS_SIDE_LENGTH 256
#define TEXTURE_ATLAS_IMAGE_SIZE 64
#define SMALL_TIME_STEP 0.02f

#define TABLE_SIDE_LENGTH 0.75f

#define BALL_SIZE 0.04f
#define POINT_SIZE 30.0f

// TODO: Use a texture for all but collision.
#define TABLE_MODEL "model/table.obj"
#define RAILS_MODEL "model/rails.obj"
#define HOLES_MODEL "model/holes.obj"
#define TICKS_MODEL "model/ticks.obj"
#define COLLISION_MODEL "model/collision.obj"

#define RAILS_INNER_HEIGHT 0.74189f

#define H_TICK 0.38203f
#define V_TICK 0.39463f

#define WIDTH  1.48378f
#define HEIGHT 0.74189f

struct ball
{
    GLint number;
    GLfloat *position;
    GLfloat *velocity;
    GLfloat *quad;
};

struct player
{
    GLuint isSolid;
    GLuint score;
};

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

struct Table
{
    GLint tableElementsSize;
    GLint railsElementsSize;
    GLint holesElementsSize;
    GLint ticksElementsSize;
    GLint collisionElementsSize;

    GLfloat *vTable;
    GLfloat *vRails;
    GLfloat *vHoles;
    GLfloat *vTicks;
    GLfloat *vCollision;

    GLushort *eTable;
    GLushort *eRails;
    GLushort *eHoles;
    GLushort *eTicks;
    GLushort *eCollision;

    GLfloat *nCollision;
};

typedef struct
{
    // ===========Particles=========== //

    // Particles Handle to a program object
    GLuint particlesProgram;

    // Particles Attribute locations
    GLint particlesStartPositionLoc;
    GLint particlesQuadTexLoc;

    // Particles Uniform location
    GLint particlesTimeLoc;
    GLint particlesColorLoc;
    GLint particlesSamplerLoc;

    // Particles Texture handle
    GLuint particlesTextureId;

    // Particles vertex data
    float particleData[ NUM_PARTICLES * PARTICLE_SIZE ];
    float particleQuadData[ NUM_PARTICLES * PARTICLE_QUAD_SIZE ];
    struct ball balls[ NUM_PARTICLES ];
    struct player players[ 2 ];

    // Current time
    float time;

    // Particles color
    float particlesColor[4];

    // Particles perspective matrix
    ESMatrix particlesMVP;
    GLint particlesMVPLoc;

    GLuint depthRenderbuffer;

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

    // ============Table============ //
    GLuint tableProgram;
    struct Table * table;
    GLuint tableStartPositionLoc;
    //GLuint tableTimeLoc;
    GLuint tableColorLoc;

    float pauseTime;
    ESMatrix tableMVP;
    GLint tableMVPLoc;

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

GLuint LoadPngTexture ( char *fileName )
{
    int *width,
        *height;
    unsigned char *buffer = PngTexture( fileName, &width, &height );
    GLuint texId;

    if ( buffer == NULL )
    {
        esLogMessage ( "Error loading (%s) image.\n", fileName );
        return 0;
    }

    glGenTextures ( 1, &texId );
    glBindTexture ( GL_TEXTURE_2D, texId );

    glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, *width, *height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    free ( buffer );
    free ( width );
    free ( height );

    return texId;
}

void LoadBallTextures ( GLuint *textures )
{
    int i = 0;
    for ( ; i < 16 ; ++i) {
        char str[15];
        sprintf(str, "texture/%d.png", i);
        textures[i] = LoadPngTexture( str );
    }
}

GLuint LoadWhiteTex( void )
{
    GLuint whiteTexHandle;
    GLubyte whiteTex[] = { 0, 0, 0, 0 };
    //glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &whiteTexHandle);
    glBindTexture(GL_TEXTURE_2D, whiteTexHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whiteTex);
    return whiteTexHandle;
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

void ParticleToQuad( const GLfloat * particle, GLfloat * quad )
{
    quad[0] = particle[0] + PATICLES_QUAD_HALF_SIDELENGTH; // bottom right.
    quad[1] = particle[1] - PATICLES_QUAD_HALF_SIDELENGTH;

    quad[4] = particle[0] - PATICLES_QUAD_HALF_SIDELENGTH; // top left
    quad[5] = particle[1] + PATICLES_QUAD_HALF_SIDELENGTH;

    quad[8] = particle[0] - PATICLES_QUAD_HALF_SIDELENGTH; // bottom left.
    quad[9] = particle[1] - PATICLES_QUAD_HALF_SIDELENGTH;

    // Sadly, due to lack of primitive restart, we have to duplicate vertices.

    quad[12] = particle[0] - PATICLES_QUAD_HALF_SIDELENGTH; // top left
    quad[13] = particle[1] + PATICLES_QUAD_HALF_SIDELENGTH;

    quad[16] = particle[0] + PATICLES_QUAD_HALF_SIDELENGTH; // bottom right.
    quad[17] = particle[1] - PATICLES_QUAD_HALF_SIDELENGTH;

    quad[20] = particle[0] + PATICLES_QUAD_HALF_SIDELENGTH; // top right.
    quad[21] = particle[1] + PATICLES_QUAD_HALF_SIDELENGTH;

}

// Texture points are strided.
void AddTextureToQuad(GLfloat *particleQuadData, int ballNumber) {
    GLfloat startX = (ballNumber % 4) * TEXTURE_ATLAS_IMAGE_SIZE;
    GLfloat startY = (ballNumber / 4) * TEXTURE_ATLAS_IMAGE_SIZE;
    GLfloat particlesTex[] = {
        (startX + TEXTURE_ATLAS_IMAGE_SIZE) / TEXTURE_ATLAS_SIDE_LENGTH,
        (startY + TEXTURE_ATLAS_IMAGE_SIZE) / TEXTURE_ATLAS_SIDE_LENGTH, // Bottom right

                                     startX / TEXTURE_ATLAS_SIDE_LENGTH,
                                     startY / TEXTURE_ATLAS_SIDE_LENGTH, // Top left

                                     startX / TEXTURE_ATLAS_SIDE_LENGTH,
        (startY + TEXTURE_ATLAS_IMAGE_SIZE) / TEXTURE_ATLAS_SIDE_LENGTH, // Bottom left

                                     startX / TEXTURE_ATLAS_SIDE_LENGTH,
                                     startY / TEXTURE_ATLAS_SIDE_LENGTH, // Top left

        (startX + TEXTURE_ATLAS_IMAGE_SIZE) / TEXTURE_ATLAS_SIDE_LENGTH,
        (startY + TEXTURE_ATLAS_IMAGE_SIZE) / TEXTURE_ATLAS_SIDE_LENGTH, // Bottom right

        (startX + TEXTURE_ATLAS_IMAGE_SIZE) / TEXTURE_ATLAS_SIDE_LENGTH,
                                     startY / TEXTURE_ATLAS_SIDE_LENGTH  // Top right
    };
    GLfloat *pt = &particlesTex[0];
    int j = 1;
    for ( ; j <= (PARTICLE_QUAD_SIZE / PARTICLE_SIZE) ; ++j ) {
        particleQuadData[4*j-2] = (*pt++);
        particleQuadData[4*j-1] = (*pt++);
    }
}

int InitBalls( ESContext *esContext )
{
    UserData *userData = esContext->userData;
    srand ( time(NULL) );
    GLint ballOrder[ NUM_PARTICLES ];
    GLint i;
    for( i = 0 ; i < NUM_PARTICLES ; ++i ) {
        ballOrder[i] = i;
    }
    // 8 ball needs to go in position 5.  A stipe and solid must compose the
    // back corners.
    GLint stripeBallPos = NUM_PARTICLES - 1;
    GLint solidBallPos = NUM_PARTICLES - 1;
    int eightBallPos = NUM_PARTICLES - 1;
    // shuffle
    for( i = 1 ; i < NUM_PARTICLES-1 ; ++i ) {
        int j = i + rand() / (RAND_MAX / (NUM_PARTICLES - i) + 1);
        int t = ballOrder[j];
        if( ballOrder[j] == 8 ) {
            eightBallPos = i;
        }
        if ( ballOrder[j] > 8 && i != 5 ) {
            stripeBallPos = i;
        }
        if ( ballOrder[j] < 8 && i != 5 ) {
            solidBallPos = i;
        }
        ballOrder[j] = ballOrder[i];
        ballOrder[i] = t;
    }

    // This is where the 8 ball must go.
    int t = ballOrder[5];
    ballOrder[5] = ballOrder[eightBallPos];
    ballOrder[eightBallPos] = t;

    if ( ballOrder[11] < 8 && ballOrder[15] < 8 ) {
        t = ballOrder[11];
        ballOrder[11] = ballOrder[stripeBallPos];
        ballOrder[stripeBallPos] = t;
    } else if ( ballOrder[11] > 8 && ballOrder[15] > 8 ) {
        t = ballOrder[11];
        ballOrder[11] = ballOrder[solidBallPos];
        ballOrder[solidBallPos] = t;
    }

    for ( i = 0; i < NUM_PARTICLES; ++i )
    {
        GLfloat *particleData = &userData->particleData[i * PARTICLE_SIZE];
        GLfloat *particleQuadData = &userData->particleQuadData[i * PARTICLE_QUAD_SIZE];
        userData->balls[ballOrder[i]].position = particleData;
        userData->balls[ballOrder[i]].velocity = particleData + 2;
        userData->balls[ballOrder[i]].quad = particleQuadData;
        AddTextureToQuad(particleQuadData, ballOrder[i]);
    }
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
    userData->particlesStartPositionLoc = glGetAttribLocation ( userData->particlesProgram, "a_startPosition" );
    userData->particlesQuadTexLoc = glGetAttribLocation ( userData->particlesProgram, "a_texCoords" );

    // Get the uniform locations
    userData->particlesTimeLoc = glGetUniformLocation ( userData->particlesProgram, "u_time" );
    userData->particlesColorLoc = glGetUniformLocation ( userData->particlesProgram, "u_color" );
    userData->particlesSamplerLoc = glGetUniformLocation ( userData->particlesProgram, "s_texture" );
    // Fill in particle data array
    //srand ( 0 );
    float poolPts [] = {
              //-4 * H_TICK - (2*BALL_SIZE),    0.0f, // white ball

              0.0f,        0.0f, // row 1

          BALL_SIZE, -BALL_SIZE, // row 2
          BALL_SIZE,  BALL_SIZE,

        2*BALL_SIZE, -2*BALL_SIZE, // row 3
        2*BALL_SIZE,         0.0f,
        2*BALL_SIZE,  2*BALL_SIZE,

        3*BALL_SIZE, -3*BALL_SIZE, // row 4
        3*BALL_SIZE,   -BALL_SIZE,
        3*BALL_SIZE,    BALL_SIZE,
        3*BALL_SIZE,  3*BALL_SIZE,

        4*BALL_SIZE, -4*BALL_SIZE, // row 5
        4*BALL_SIZE, -2*BALL_SIZE,
        4*BALL_SIZE,         0.0f,
        4*BALL_SIZE,  2*BALL_SIZE,
        4*BALL_SIZE,  4*BALL_SIZE,
    };
    GLfloat *pt = &poolPts[0];
    for ( i = 1; i < NUM_PARTICLES; i++ )
    {
        GLfloat *particleData = &userData->particleData[i * PARTICLE_SIZE];
        GLfloat *particleQuadData = &userData->particleQuadData[i * PARTICLE_QUAD_SIZE];

        // Start position of particle
        GLfloat *ptr = particleData;

        (*ptr++) = (*pt++) + ((2 * H_TICK) + (2*BALL_SIZE));
        (*ptr++) = (*pt++);

        // Velocities
        (*ptr++) = 0.0f;
        (*ptr++) = 0.0f;

        ParticleToQuad(particleData, particleQuadData);
    }

    //userData->particlesTextureId = LoadTexture ( "texture/smoke.tga" );
    userData->particlesTextureId = LoadPngTexture( "texture/balls.png" );
    //LoadBallTextures( userData->particlesTextures );
    //userData->particlesTextureId = LoadWhiteTex();
    //if ( userData->particlesTextureId <= 0 )
    //{
    //    return FALSE;
    //}

    // Set up the perspective matrix.
    userData->particlesMVPLoc = glGetUniformLocation ( userData->particlesProgram, "u_MVP" );

    // Generate a model view matrix to rotate/translate the cube
    ESMatrix modelview;
    ESMatrix perspective;

    esMatrixLoadIdentity( &modelview );

    // Translate away from the viewer
    esTranslate( &modelview, 0.0, 0.0, -1.9999 );

    esMatrixLoadIdentity( &perspective );
    esPerspective(&perspective, 60.0f, (float)esContext->width /
                  (float)esContext->height, 1.0f, 20.0f);
    esMatrixMultiply( &userData->particlesMVP, &modelview, &perspective );

    glUseProgram ( userData->particlesProgram );

    //float centerPos[2];
    float color[4];

    // Pick a new start location and color
    //centerPos[0] = ( (float)(rand() % 10000) / 10000.0f ) - 0.5f;
    //centerPos[1] = ( (float)(rand() % 10000) / 10000.0f ) - 0.5f;


    // Random color
    color[0] = ( (float)(rand() % 10000) / 20000.0f ) + 0.5f;
    color[1] = ( (float)(rand() % 10000) / 20000.0f ) + 0.5f;
    color[2] = ( (float)(rand() % 10000) / 20000.0f ) + 0.5f;
    color[3] = 1.0;
    memcpy(&userData->particlesColor[0], &color[0], sizeof(float) * 4);

    glUniform4fv ( userData->particlesColorLoc, 1, &color[0] );

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

int InitTable( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    userData->table->tableElementsSize = loadObj(TABLE_MODEL,
            &userData->table->vTable, &userData->table->eTable);

    // Generate a model view matrix to rotate/translate the cube
    ESMatrix modelview;
    ESMatrix perspective;

    esMatrixLoadIdentity( &modelview );

    // Translate away from the viewer
    esTranslate( &modelview, 0.0, 0.0, -2.0 );

    esMatrixLoadIdentity( &perspective );
    esPerspective(&perspective, 60.0f, (float)esContext->width /
                  (float)esContext->height, 1.0f, 20.0f);
    esMatrixMultiply( &userData->tableMVP, &modelview, &perspective );
    return TRUE;
}

int InitRails( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    userData->table->railsElementsSize = loadObj(RAILS_MODEL,
            &userData->table->vRails, &userData->table->eRails);
    return TRUE;
}

int InitHoles( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    userData->table->holesElementsSize = loadObj(HOLES_MODEL,
            &userData->table->vHoles, &userData->table->eHoles);
    return TRUE;
}

int InitTicks( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    userData->table->ticksElementsSize = loadObj(TICKS_MODEL,
            &userData->table->vTicks, &userData->table->eTicks);
    return TRUE;
}

int InitCollision( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    userData->table->collisionElementsSize = loadObj(COLLISION_MODEL,
            &userData->table->vCollision, &userData->table->eCollision);
    userData->table->nCollision =
            ComputeSurfaceNormals(userData->table->vCollision,
            userData->table->eCollision, userData->table->collisionElementsSize);
    return TRUE;
}

int InitBilliardsTable( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    char * vShaderStr = loadShader( "shader/table.vert" );
    char * fShaderStr = loadShader( "shader/table.frag" );

    // Load the shaders and get a linked program object
    userData->tableProgram = esLoadProgram ( vShaderStr, fShaderStr );
    free(vShaderStr);
    free(fShaderStr);

    // Get the attribute locations
    userData->tableStartPositionLoc = glGetAttribLocation ( userData->tableProgram, "a_startPosition" );

    // Get the uniform locations
    //userData->tableTimeLoc = glGetUniformLocation ( userData->tableProgram, "u_time" );
    userData->tableColorLoc = glGetUniformLocation ( userData->tableProgram, "u_color" );
    userData->tableMVPLoc = glGetUniformLocation ( userData->particlesProgram, "u_MVP" );

    if ( !InitTable(esContext) ) {
        return FALSE;
    }
    if ( !InitRails(esContext) ) {
        return FALSE;
    }
    if ( !InitHoles(esContext) ) {
        return FALSE;
    }
    if ( !InitTicks(esContext) ) {
        return FALSE;
    }
    if ( !InitCollision(esContext) ) {
        return FALSE;
    }
    return TRUE;
}

///
// Initialize the shader and program object
//
int Init ( ESContext *esContext )
{
    UserData *userData = esContext->userData;
    if ( !InitBalls(esContext) ) {
        return FALSE;
    }
    if ( !InitParticles(esContext) ) {
        return FALSE;
    }
    if ( !InitBilliardsTable(esContext) ) {
        return FALSE;
    }
    GLfloat x = 0.0f, y = 0.0f;
    Draw(esContext);
    eglSwapBuffers(esContext->eglDisplay, esContext->eglSurface);
    while(x < -WIDTH || x > -2*H_TICK || y < -HEIGHT || y > 2*HEIGHT) {
        printf("Enter white ball start position [%.3f, %.3f], [%.3f, %.3f]: ", -WIDTH, -2*H_TICK, -HEIGHT, HEIGHT);
        scanf("%f %f", &x, &y);
    }
    GLfloat *particle = &userData->particleData[ 0 ];
    particle[0] = x;
    particle[1] = y;
    //if ( !InitQuad(esContext) ) {
    //    return FALSE;
    //}
    //if ( !InitFBO(esContext) ) {
    //    return FALSE;
    //}
    glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );

    userData->time = 0.0f;
    glEnable( GL_DEPTH_TEST );
    return TRUE;
}

void RewindToImpact(GLfloat *pos1, GLfloat *pos2, GLfloat *posQuad1, GLfloat
        *posQuad2, GLfloat *particleData, GLfloat *particleQuadData, GLuint
        recursionLevel)
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

    GLfloat initialRewindFactor = 0.003f; // arbitrary small step.

    // Take the first small step.
    scale(&tmpVec[0], &pos1[2], initialRewindFactor, 2);
    UMinusV(&tmpPos1[0], &pos1[0], &tmpVec[0], 2);
    scale(&tmpVec[0], &pos2[2], initialRewindFactor, 2);
    UMinusV(&tmpPos2[0], &pos2[0], &tmpVec[0], 2);

    GLfloat secondDiff;
    distanceSquared(&secondDiff, &tmpPos1[0], &tmpPos2[0], 2);

    // Calculate how many more steps to take.
    GLfloat numSteps = 100.0f;
    if (impactDistanceSquared > 0.0f) {
        numSteps = (1.0f - (secondDiff / impactDistanceSquared)) / ((secondDiff
                - initialDiff) / impactDistanceSquared);
    }

    //numSteps += 350.0f; // TODO: bad bad fudge factor
    numSteps += numSteps;

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
    ParticleToQuad(pos1, posQuad1);
    ParticleToQuad(pos2, posQuad2);
    // This is mainly for the break when all balls are close together.
    // Rewinding tends to get into another's space.
    unsigned int i;
    if(recursionLevel < 2) {
        for ( i = 0 ; i < NUM_PARTICLES ; ++i ) {
            GLfloat *pt = &particleData[i * PARTICLE_SIZE];
            GLfloat *ptQuad = &particleQuadData[ i * PARTICLE_QUAD_SIZE ];
            GLfloat distance;
            if (pos1 != pt) {
                distanceSquared(&distance, &pos1[0], &pt[0], 2);
                if (distance <= 4*POINT_RADIUS*POINT_RADIUS ) {
                    RewindToImpact(pos1, pt, posQuad1, ptQuad, particleData,
                            particleQuadData, recursionLevel+1);
                }
            }
            if (pos2 != pt) {
                distanceSquared(&distance, &pos2[0], &pt[0], 2);
                if (distance <= 4*POINT_RADIUS*POINT_RADIUS ) {
                    RewindToImpact(pos2, pt, posQuad2, ptQuad, particleData,
                            particleQuadData, recursionLevel+1);
                }
            }
        }
    }
}

void ParticleCollision(GLfloat *pos1, GLfloat *pos2)
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

void CheckForParticleCollisions ( GLfloat *particleData, GLfloat *particleQuadData )
{
    int i;
    for ( i = 0 ; i < NUM_PARTICLES ; ++i ) {
        int j;
        GLfloat *point1 = &particleData[i * PARTICLE_SIZE];
        GLfloat *pointQuad1 = &particleQuadData[ i * PARTICLE_QUAD_SIZE ];
        for ( j = i+1 ; j < NUM_PARTICLES ; ++j ) {
            GLfloat *point2 = &particleData[j * PARTICLE_SIZE];
            GLfloat *pointQuad2 = &particleQuadData[ j * PARTICLE_QUAD_SIZE  ];
            GLfloat diff1 = point1[0] - point2[0];
            GLfloat diff2 = point1[1] - point2[1];

            if (diff1*diff1 + diff2*diff2 <= 4*POINT_RADIUS*POINT_RADIUS) {
                //printf("Collision: (%f, %f), (%f, %f)\n", point1[0], point1[1],
                //        point2[0], point2[1]);
                RewindToImpact(point1, point2, pointQuad1, pointQuad2,
                        particleData, particleQuadData, 0);
                ParticleCollision(point1, point2);
            }
        }
    }
}

void CheckForBoundaryCollisions( GLfloat *particleData, GLfloat *v, GLushort
        *e, GLint elementsSize, GLfloat *n )
{
    // boundaryPoints is counter-clockwise starting at the lower left
    int i;
    for( i = 0 ; i < NUM_PARTICLES ; ++i ) {
        GLfloat *point = &particleData[i * PARTICLE_SIZE];
        if ( point[0] == INFINITY )
            continue;
        unsigned int i;
        for ( i = 1 ; i < elementsSize ; i+=2 ) {
            GLfloat v1[2], v2[2], v3[2], v4[2], v5[2];
            GLfloat point2[2];
            // TODO: maybe precompute this.
            v1[0] = v[2*e[i]] - v[2*e[i-1]];
            v1[1] = v[2*e[i]+1] - v[2*e[i-1]+1];
            normalize2f(&v1[0]);

            v2[0] = point[0] - v[2*e[i-1]];
            v2[1] = point[1] - v[2*e[i-1]+1];
            normalize2f(&v2[0]);

            v3[0] = point[0] - v[2*e[i]];
            v3[1] = point[1] - v[2*e[i]+1];
            normalize2f(&v3[0]);

            // point2 is just point1 moved along its velocity a small time step.
            point2[0] = point[0] + (SMALL_TIME_STEP * point[2]);
            point2[1] = point[1] + (SMALL_TIME_STEP * point[3]);

            v4[0] = point2[0] - v[2*e[i-1]];
            v4[1] = point2[1] - v[2*e[i-1]+1];

            v5[0] = point2[0] - v[2*e[i]];
            v5[1] = point2[1] - v[2*e[i]+1];

            GLfloat result1, result2, result3, result4, result5;
            UDotV(&result1, &v1[0], &v2[0], 2);
            result1 = acos(result1);

            scalarU(&v1[0], &v1[0], -1.0f, 2);
            UDotV(&result2, &v1, &v3, 2);
            result2 = acos(result2);

            GLfloat normal[2];

            normal[0] = v1[1];
            normal[1] = -v1[0];

            UDotV(&result3, &point[2], &normal[0], 2);
            GLfloat size;

            UDotV(&result4, &v4[0], &normal[0], 2);
            UDotV(&result5, &v5[0], &normal[0], 2);

            distanceSquared(&size, &v[2*e[i]], &v[2*e[i-1]], 2);
            size = sqrt(size);

            if ( result3 < 0.0f && result1 < HALFPI && result2 < HALFPI ) {
                if( ((result1 <= -0.14707f * size + 0.21279f && result2 < HALFPI)
                        || (result2 <= -0.14707f * size + 0.21279f && result1 <
                        HALFPI)) || result4 < 0.0f || result5 < 0.0f ) {
                    if(((i - 1) / 2) % 4 == 1) {
                        point[0] = INFINITY;
                        point[1] = INFINITY;
                        point[2] = 0.0f;
                        point[3] = 0.0f;
                    } else {
                        reflectAboutNormal2f(&point[2], &point[2], &normal[0]);
                    }
                }
            }
        }
    }
}

int CheckForMovement( GLfloat *particleData )
{
    int i;
    for ( i=0 ; i < NUM_PARTICLES ; ++i ) {
        GLfloat *point = &particleData[i * PARTICLE_SIZE];
        if (fabs(point[2]) > 0.0f || fabs(point[3]) > 0.0f) {
            return 1;
        }
    }
    return 0;
}

void UpdatePositions ( ESContext *esContext, float deltaTime )
{
    UserData *userData = esContext->userData;
    GLfloat *particleData = &userData->particleData[0];
    GLfloat *particleQuadData = &userData->particleQuadData[0];
    CheckForParticleCollisions( particleData, particleQuadData );
    CheckForBoundaryCollisions( particleData, userData->table->vCollision,
            userData->table->eCollision, userData->table->collisionElementsSize,
            userData->table->nCollision );
    {
        int i;
        for ( i = 0 ; i < NUM_PARTICLES ; ++i ) {
            particleData = &userData->particleData[i * PARTICLE_SIZE];
            particleQuadData = &userData->particleQuadData[i * PARTICLE_QUAD_SIZE];

            particleData[0] += particleData[2] * deltaTime;
            particleData[1] += particleData[3] * deltaTime;
            ParticleToQuad(particleData, particleQuadData);

            GLfloat tmpAccelVec[2];
            scale(&tmpAccelVec[0], &particleData[2], POINT_ACCELERATION * deltaTime, 2);
            UPlusV(&particleData[2], &particleData[2], &tmpAccelVec[0], 2);
            if(fabs(particleData[2]) < 0.01f) {
                particleData[2] = 0.0f;
            }
            if(fabs(particleData[3]) < 0.01f) {
                particleData[3] = 0.0f;
            }
        }
    }
}

///
//  Update time-based variables
//
void Update ( ESContext *esContext, float deltaTime )
{
    UserData *userData = esContext->userData;
    GLfloat *particleData = &userData->particleData[0];

    userData->time += deltaTime;
    // Load uniform time variable

    glUseProgram ( userData->particlesProgram );
    glUniform1f ( userData->particlesTimeLoc, userData->time );
    //glUseProgram ( userData->tableProgram );
    //glUniform1f ( userData->tableTimeLoc, userData->time );
    float scanfTime = 0.0f;
    if (!CheckForMovement( particleData )) {
        printf("Enter Velocity: ");
        float x, y;
        struct timeval t1, t2;
        struct timezone tz;
        gettimeofday ( &t1 , &tz );
        scanf("%f %f", &x, &y);
        gettimeofday( &t2, &tz );
        scanfTime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
        particleData[2] = (GLfloat) x;
        particleData[3] = (GLfloat) y;
    }
    UpdatePositions( esContext, deltaTime - userData->pauseTime );
    userData->pauseTime = scanfTime;
}

void DrawParticles ( ESContext *esContext )
{
    UserData *userData = esContext->userData;
    //glEnable( GL_DEPTH_TEST );
    //glClearDepthf(1.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );


    // Use the program object
    glUseProgram ( userData->particlesProgram );

    //glVertexAttribPointer ( userData->particlesStartPositionLoc, 2, GL_FLOAT,
    //        GL_FALSE, PARTICLE_SIZE * sizeof(GLfloat),
    //        &userData->particleData[0] );
    glVertexAttribPointer( userData->particlesStartPositionLoc,
                           2,
                           GL_FLOAT,
                           GL_FALSE,
                           PARTICLE_SIZE * sizeof(GLfloat),
                           &userData->particleQuadData[0]
                         );
    glVertexAttribPointer( userData->particlesQuadTexLoc,
                           2,
                           GL_FLOAT,
                           GL_FALSE,
                           PARTICLE_SIZE * sizeof(GLfloat),
                           &userData->particleQuadData[2]
                         );

    glEnableVertexAttribArray ( userData->particlesStartPositionLoc );
    glEnableVertexAttribArray ( userData->particlesQuadTexLoc );
    // Blend particles
    //glEnable ( GL_BLEND );
    //glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

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
    glUniform4fv ( userData->particlesColorLoc, 1, &userData->particlesColor[0] );
    glDrawArrays( GL_TRIANGLES, 0, NUM_PARTICLES * (PARTICLE_QUAD_SIZE / PARTICLE_SIZE) );
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

void DrawTable( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    glUseProgram ( userData->tableProgram );
    GLfloat color[] = { 0.0f, 0.2f, 0.0f, 1.0f };
    glUniform4fv ( userData->tableColorLoc, 1, color );

    glVertexAttribPointer ( userData->tableStartPositionLoc, 2, GL_FLOAT,
            GL_FALSE, 0, &userData->table->vTable[0] );
    glEnableVertexAttribArray ( userData->tableStartPositionLoc );
    //glDisable( GL_DEPTH_TEST );
    glEnable ( GL_BLEND );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
    glDrawElements ( GL_TRIANGLES, userData->table->tableElementsSize,
            GL_UNSIGNED_SHORT, &userData->table->eTable[0] );
}

void DrawRails( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    glUseProgram ( userData->tableProgram );
    GLfloat color[] = { 0.0f, 0.2f, 0.0f, 1.0f };
    glUniform4fv ( userData->tableColorLoc, 1, color );

    glVertexAttribPointer ( userData->tableStartPositionLoc, 2, GL_FLOAT,
            GL_FALSE, 0, &userData->table->vRails[0] );
    glEnableVertexAttribArray ( userData->tableStartPositionLoc );
    glDisable( GL_DEPTH_TEST );
    glEnable ( GL_BLEND );
    //glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
    glDrawElements ( GL_TRIANGLES, userData->table->railsElementsSize,
            GL_UNSIGNED_SHORT, &userData->table->eRails[0] );
}

void DrawHoles( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    glUseProgram ( userData->tableProgram );
    GLfloat color[] = { 0.0f, 0.2f, 0.0f, 1.0f };
    glUniform4fv ( userData->tableColorLoc, 1, color );

    glVertexAttribPointer ( userData->tableStartPositionLoc, 2, GL_FLOAT,
            GL_FALSE, 0, &userData->table->vHoles[0] );
    glEnableVertexAttribArray ( userData->tableStartPositionLoc );
    //glDisable( GL_DEPTH_TEST );
    //glDisable ( GL_BLEND );
    glDrawElements ( GL_TRIANGLES, userData->table->holesElementsSize,
            GL_UNSIGNED_SHORT, &userData->table->eHoles[0] );
}

void DrawTicks( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    glUseProgram ( userData->tableProgram );
    GLfloat color[] = { 0.4f, 0.2f, 0.4f, 1.0f };
    glUniform4fv ( userData->tableColorLoc, 1, color );

    glVertexAttribPointer ( userData->tableStartPositionLoc, 2, GL_FLOAT,
            GL_FALSE, 0, &userData->table->vTicks[0] );
    glEnableVertexAttribArray ( userData->tableStartPositionLoc );
    //glDisable( GL_DEPTH_TEST );
    glEnable ( GL_BLEND );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
    glDrawElements ( GL_TRIANGLES, userData->table->ticksElementsSize,
            GL_UNSIGNED_SHORT, &userData->table->eTicks[0] );
}

void DrawBilliardsTable( ESContext *esContext )
{
    UserData *userData = esContext->userData;
    glUseProgram (userData->tableProgram);

    glUniformMatrix4fv(userData->tableMVPLoc, 1, GL_FALSE,
                       &userData->tableMVP.m[0][0]);
    DrawTable(esContext);
    DrawRails(esContext);
    DrawHoles(esContext);
    DrawTicks(esContext);
    //UserData *userData = esContext->userData;

    //glUseProgram ( userData->particlesProgram );
    //glUniform1i ( userData->particlesUseTexture, 0 );

    //glVertexAttribPointer ( userData->particlesStartPositionLoc, 2, GL_FLOAT,
    //        GL_FALSE, 0, &userData->table->v[0] );
    //glDrawElements ( GL_TRIANGLES, userData->table->elementsSize,
    //        GL_UNSIGNED_SHORT, &userData->table->e[0] );
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
    glClearDepthf( 1.0f );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DrawBilliardsTable( esContext );
    DrawParticles( esContext );
    //int hasRedPix = ReadPixels( esContext );
    //printf("%d\n", hasRedPix);
    //DrawQuad( esContext );
}

///
// Cleanup
//
void FreeTable( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    free(userData->table->vTable);
    free(userData->table->vRails);
    free(userData->table->vHoles);
    free(userData->table->vTicks);
    free(userData->table->vCollision);

    free(userData->table->eTable);
    free(userData->table->eRails);
    free(userData->table->eHoles);
    free(userData->table->eTicks);
    free(userData->table->eCollision);

    free(userData->table->nCollision);
}

void ShutDown ( ESContext *esContext )
{
    UserData *userData = esContext->userData;

    // Delete texture object
    glDeleteTextures ( 1, &userData->particlesTextureId );
    glDeleteTextures ( 1, &userData->quadTextureId );

    // Delete program object
    glDeleteProgram ( userData->particlesProgram );
    FreeTable( esContext );
}

int main ( int argc, char *argv[] )
{
    ESContext esContext;
    UserData  userData;

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
    quad.v = vQuad;

    GLushort eQuad[] = {
        0, 1, 2, 0, 2, 3
    };
    quad.e = eQuad;

    userData.quad = &quad;

    struct Table table;
    //GLuint *sizes = loadObj("model/table.obj", &table.v, &table.e, &table.n);
    //table.elementsSize = sizes[1];
    //free(sizes);
    /*
    table.elementsSize = 8;
    GLfloat vTable[] = {
        -2*TABLE_SIDE_LENGTH, -TABLE_SIDE_LENGTH,
         2*TABLE_SIDE_LENGTH, -TABLE_SIDE_LENGTH,
         2*TABLE_SIDE_LENGTH,  TABLE_SIDE_LENGTH,
        -2*TABLE_SIDE_LENGTH,  TABLE_SIDE_LENGTH,
    };
    table.v = &vTable;

    GLushort eTable[] = {
        0, 1,
        1, 2,
        2, 3,
        3, 0,
    };
    table.e = &eTable;
    */
    userData.table = &table;

    // Setup renderToTexTexWidth/Height
    userData.renderToTexTexWidth = RENDER_TO_TEX_WIDTH;
    userData.renderToTexTexHeight = RENDER_TO_TEX_HEIGHT;

    esInitContext ( &esContext );
    esContext.userData = &userData;

    esCreateWindow ( &esContext, "ParticleSystem", 1920, 1080, ES_WINDOW_RGB | ES_WINDOW_DEPTH | ES_WINDOW_ALPHA );

    if ( !Init ( &esContext ) )
        return 0;

    esRegisterDrawFunc ( &esContext, Draw );
    esRegisterUpdateFunc ( &esContext, Update );

/*
    glGenRenderbuffers(1, &userData.depthRenderbuffer);

    glBindRenderbuffer(GL_RENDERBUFFER, userData.depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
            1920, 1080);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_RENDERBUFFER, userData.depthRenderbuffer);

*/
    esMainLoop ( &esContext );

    ShutDown ( &esContext );
}
