#include <stdio.h>
#include <GLES2/gl2.h>
#include "glesVMath.h"
#include <math.h>

void normalize2f( GLfloat* v )
{
    GLfloat magnitude = sqrt(pow(v[0], 2) + pow(v[1], 2));
    v[0] = v[0] / magnitude;
    v[1] = v[1] / magnitude;
}

void UMinusV( GLfloat *_result, const GLfloat *_u, const GLfloat *_v, const
        GLuint _dimension )
{
    //if ( _result == _u || _result == _v ) {
    //    fprintf( stderr, "Error.  Result points to a parameter\n" );
    //    return;
    //}
    GLuint i;
    for ( i = 0 ; i < _dimension ; ++i ) {
        _result[i] = _u[i] - _v[i];
    }
}

void UPlusV( GLfloat *_result, const GLfloat *_u, const GLfloat *_v, const
        GLuint _dimension )
{
    //if ( _result == _u || _result == _v ) {
    //    fprintf( stderr, "Error.  Result points to a parameter\n" );
    //    return;
    //}
    GLuint i;
    for ( i = 0 ; i < _dimension ; ++i ) {
        _result[i] = _u[i] + _v[i];
    }
}

void UDotV( GLfloat *_result, const GLfloat *_u, const GLfloat *_v, const
        GLuint _dimension )
{
    GLuint i;
    *_result = 0;
    for ( i = 0 ; i < _dimension ; ++i ) {
        *_result += _u[i] * _v[i];
    }
}

void scale( GLfloat *_result, const GLfloat *_vec, const GLfloat _a, const
        GLuint dimension )
{
    if ( _result == _vec ) {
        fprintf( stderr, "Error.  Result points to a parameter\n" );
        return;
    }
    GLuint i;
    for ( i = 0 ; i < dimension ; ++i ) {
        _result[i] = _vec[i] * _a;
    }
}

void projectUonV2f(GLfloat *_result, const GLfloat *_u, const GLfloat *_v)
{
    GLfloat UdotV;
    GLfloat VdotV;
    UDotV(&UdotV, _u, _v, 2);
    UDotV(&VdotV, _v, _v, 2);
    scale(_result, _v, UdotV / VdotV, 2);
}

void scalarU(GLfloat *_result, const GLfloat *_vec, GLfloat _scalar, const GLuint
        _dimension)
{
    int i;
    for ( i = 0 ; i < _dimension ; ++i) {
        _result[i] = _scalar * _vec[i];
    }
}
void distanceSquared(GLfloat *_result, GLfloat *_point1, GLfloat *_point2,
        GLuint _dimension)
{
    int i;
    *_result = 0.0f;
    for ( i = 0 ; i < _dimension ; ++i ) {
        GLfloat tmp = _point1[i] - _point2[i];
        *_result += tmp * tmp;
    }
}

void magnitudeSquaredU( GLfloat *_result, const GLfloat *_u, const GLuint
        _dimension)
{
    int i;
    GLfloat tmp = 0.0f;
    for ( i = 0 ; i < _dimension ; ++i ) {
        tmp += _u[i] * _u[i];
    }
    *_result = tmp;
}

void magnitudeSquaredComponents( GLfloat *_result, const GLfloat
        _magnitudeSquared, GLfloat *_vector, GLuint _dimension )
{
    int i;
    GLfloat sum = 0;
    for ( i = 0 ; i < _dimension ; ++i ) {
        sum += _vector[i] * _vector[i];
    }
    *_result = _magnitudeSquared / sum;
}

void reflectAboutNormal2f( GLfloat *_result, const GLfloat *_u, const GLfloat
        *_n )
{
    GLfloat magnitude;
    magnitudeSquaredU(&magnitude, _u, 2);

    GLfloat tmp_u[2], tmp_n[2], tmpVec[2], tmpFloat;
    memcpy(&tmp_u[0], &_u[0], sizeof(GLfloat) * 2);
    memcpy(&tmp_n[0], &_n[0], sizeof(GLfloat) * 2);
    normalize2f(&tmp_n[0]);

    UDotV(&tmpFloat, &tmp_u[0], &tmp_n[0], 2);
    tmpFloat *= 2;
    scalarU(&tmpVec[0], &tmp_n[0], tmpFloat, 2);
    UMinusV(&tmpVec[0], &tmp_u[0], &tmpVec[0], 2);
    memcpy(&_result[0], &tmpVec[0], sizeof(GLfloat) * 2);
}
