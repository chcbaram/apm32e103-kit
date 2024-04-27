#include "madgwick.h"
#include <math.h>

#define sampleFreqDef   512.0f          // sample frequency in Hz
#define betaDef         0.1f            // 2 * proportional gain


static float invSqrt(float x);
static void computeAngles();

static float beta;				// algorithm gain
static float roll;
static float pitch;
static float yaw;
static char anglesComputed;

static float invSampleFreq;

static float q0;
static float q1;
static float q2;
static float q3;	// quaternion of sensor frame relative to auxiliary frame



bool madgwickInit(void)
{
	beta = betaDef;

	q0 = 1.0f;
	q1 = 0.0f;
	q2 = 0.0f;
	q3 = 0.0f;
	invSampleFreq = 1.0f / sampleFreqDef;
	anglesComputed = 0;

  roll  = 0;
  pitch = 0;
  yaw   = 0;

  return true;
}

void madgwickSetFreq(float freq_hz)
{
  invSampleFreq = 1.0f / freq_hz; 
}

void madgwickSetFreqTime(float freq_time)
{
  invSampleFreq = freq_time; 
}

void madgwickUpdate(float gx, float gy, float gz, float ax, float ay, float az)
{
  float recipNorm;
	float s0, s1, s2, s3;
	float qDot1, qDot2, qDot3, qDot4;
	float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2 ,_8q1, _8q2, q0q0, q1q1, q2q2, q3q3;

	// Convert gyroscope degrees/sec to radians/sec
	gx *= 0.0174533f;
	gy *= 0.0174533f;
	gz *= 0.0174533f;

	// Rate of change of quaternion from gyroscope
  qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
  qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
  qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
  qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);

  // Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
	if(!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) 
  {
		// Normalise accelerometer measurement
		recipNorm = invSqrt(ax * ax + ay * ay + az * az);
		ax *= recipNorm;
		ay *= recipNorm;
		az *= recipNorm;

		// Auxiliary variables to avoid repeated arithmetic
		_2q0 = 2.0f * q0;
		_2q1 = 2.0f * q1;
		_2q2 = 2.0f * q2;
		_2q3 = 2.0f * q3;
		_4q0 = 4.0f * q0;
		_4q1 = 4.0f * q1;
		_4q2 = 4.0f * q2;
		_8q1 = 8.0f * q1;
		_8q2 = 8.0f * q2;
		q0q0 = q0 * q0;
		q1q1 = q1 * q1;
		q2q2 = q2 * q2;
		q3q3 = q3 * q3;

		// Gradient decent algorithm corrective step
		s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
		s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
		s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
		s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;
		recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3); // normalise step magnitude
		s0 *= recipNorm;
		s1 *= recipNorm;
		s2 *= recipNorm;
		s3 *= recipNorm;

		// Apply feedback step
		qDot1 -= beta * s0;
		qDot2 -= beta * s1;
		qDot3 -= beta * s2;
		qDot4 -= beta * s3;
	}

	// Integrate rate of change of quaternion to yield quaternion
	q0 += qDot1 * invSampleFreq;
	q1 += qDot2 * invSampleFreq;
	q2 += qDot3 * invSampleFreq;
	q3 += qDot4 * invSampleFreq;

	// Normalise quaternion
	recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
	q0 *= recipNorm;
	q1 *= recipNorm;
	q2 *= recipNorm;
	q3 *= recipNorm;
	anglesComputed = 0;
}

bool madgwickGetInfo(madgwick_info_t *p_info)
{
  if (!anglesComputed)
    computeAngles();

  p_info->deg_roll  = roll * 57.29578f;
  p_info->deg_pitch = pitch * 57.29578f;
  p_info->deg_yaw   = yaw * 57.29578f + 180.0f;

  p_info->rad_roll  = roll;
  p_info->rad_pitch = pitch;
  p_info->rad_yaw   = pitch;

  p_info->quat[0] = q0;
  p_info->quat[1] = q1;
  p_info->quat[2] = q2;
  p_info->quat[3] = q3;

  return true;
}

// Fast inverse square-root
// See: http://en.wikipedia.org/wiki/Fast_inverse_square_root

float invSqrt(float x) 
{
  float halfx = 0.5f * x;
  float y     = x;
  long  i     = *(long *)&y;
  i           = 0x5f3759df - (i >> 1);
  y           = *(float *)&i;
  y           = y * (1.5f - (halfx * y * y));
  y           = y * (1.5f - (halfx * y * y));
  return y;
}

void computeAngles(void)
{
  roll           = atan2f(q0 * q1 + q2 * q3, 0.5f - q1 * q1 - q2 * q2);
  pitch          = asinf(-2.0f * (q1 * q3 - q0 * q2));
  yaw            = atan2f(q1 * q2 + q0 * q3, 0.5f - q2 * q2 - q3 * q3);
  anglesComputed = 1;
}