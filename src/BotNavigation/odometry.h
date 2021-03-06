
#ifndef Odometry_h
#define Odometry_h



/*
 * Christopher Marisco 
 * 3/5/19
 * Original Implimentation
 * Spencer Sawyer
 * 3/16/19
 * Refactored into a class 
 *
 * Odometry Implementation file.
 */
#define FUDGEDIST .6
#include <math.h>		/* trig function header */

#include "Encoder.h"    /* read from encoders */


//wheel base 205 or 221.5 mm (depends if from mid of wheels or just between wheels)
//rwheel diameter = 82.14 mm
//lwheel diameter = 80.51 mm
//right_clicks_per_mm = 18.74 or 19.23
//left_clicks_per_mm = 18.85 or 19.36

/* ----------------------------------------------------------------------- */

//IF We need adjust turns for theta accuracy., use the wheel_base
//Higher wheel base = smaller theta calculated per turn.
//WARNING THIS IMPACTS POSITION CALCULATIONS.

//MINIMIZE NUMBER OF TURNS COMMANDED TO REDUCE COMPOUNDING ERRORS!!!!
//#define WHEEL_BASE 221.5
#define LEFT_CLICKS_PER_MM 19.36
#define RIGHT_CLICKS_PER_MM 19.23
float WHEEL_BASE = 221.5;
#define TWOPI 6.2831853070		/* nice to have float precision */
#define RADS 57.2958			/* radians to degrees conversion */

/* ----------------------------------------------------------------------- */
/* odometers() maintains these global accumulator variables: */
namespace NavOdometeryFunc
{
	//commented sections go together, Either the top or bottom should be active, not both (obviously) 
	//the bottom is if the rotation in the wrong direction.
	double rotateCoordsX(double x, double y, double thetaRads)
	{
		return x * cos(-thetaRads) - y * sin(-thetaRads);
		//return x * cos(thetaRads) + y * sin(thetaRads);
	}
	double rotateCoordsY(double x, double y, double thetaRads)
	{
		return y * cos(-thetaRads) + x * sin(-thetaRads);
		//return y * cos(thetaRads) - x * sin(thetaRads);
		
	}
}
class NavOdometery{
public:
	double degree;
	double overshoot;

	double dist;
	double theta = 0.0;                    /* bot heading */
	double theta_D = 0.0;
	double X_pos = 0.0;                    /* bot X position in mm */
	double Y_pos = 0.0;                    /* bot Y position in mm */

	
	/* using these local variables */
	
	double left_mm = 0.0;
	double right_mm = 0.0;
	
	
	/* ----------------------------------------------------------------------- */
	 long lsamp = 0, rsamp = 0, L_ticks = 0, R_ticks = 0, last_left = 0,	last_right = 0;
	
	/* ----------------------------------------------------------------------- */
	/* locate_target() uses these global variables */
	
	 double X_target;                 /* X lateral target position from original orientation */
	 double Y_target;                 /* Y vertical target position from original orientation*/
	 double target_bearing = 0.0;           /* bearing in radians from current	position */
	
	 double target_distance = 0.0;         	/* distance in inches from position  */
	 double heading_error = 0.0;            /* heading error in degrees */
	
	/* ----------------------------------------------------------------------- */
	/*  calculate distance and bearing to target.
	
	        inputs are:  X_target, X_pos, and Y_target, Y_pos
	        outputs are: target_distance, heading_error
	*/
private:
	Encoder *  knobLeftptr;
	Encoder * knobRightptr;
	Motors * motors;
public:
	double totalMM=0;
	NavOdometery(Encoder * left, Encoder * right, Motors * MOTORS)
	{
		knobLeftptr = left;
		knobRightptr = right;
		motors = MOTORS;

	}

	#define  knobLeft (*knobLeftptr) 
	#define  knobRight (*knobRightptr)
	#define  motors (*motors)
	
	void resetOdometry()
	{
		theta = 0;
		X_pos = 0;
		Y_pos = 0;
		totalMM = 0;
	}
	
	void odometers()
	{
		double mm;
	        /* sample the left and right encoder counts as close together */
			/* in time as possible */
	                lsamp = knobLeft.read();
	                rsamp = knobRight.read(); 
	
			/* determine how many ticks since our last sampling? */
	                L_ticks = lsamp - last_left; 	
	                R_ticks = rsamp - last_right;
	
			/* and update last sampling for next time */
	                last_left = lsamp; 
	                last_right = rsamp; 
	
			/* convert longs to floats and ticks to mm */
	                left_mm = (double)L_ticks/LEFT_CLICKS_PER_MM;
	                right_mm = (double)R_ticks/RIGHT_CLICKS_PER_MM;
	
			/* calculate distance we have traveled since last sampling */
	                mm = (left_mm + right_mm) / 2.0;
		
			/* accumulate total rotation around our center */
	                theta += (left_mm - right_mm) / WHEEL_BASE;
	
			/* and clip the rotation to plus or minus 360 degrees */
	                theta -= (double)((int)(theta/TWOPI))*TWOPI;
					if (theta < (-PI))
					{
						theta = TWOPI - theta;
					}
					if (theta > (PI))
					{
						theta = theta - TWOPI;
					}
			            theta_D = theta * 180/PI;


			/* now calculate and accumulate our position in mm */
	                Y_pos += mm * cos(theta); 
	                X_pos += mm * sin(theta); 
					totalMM += mm;
	
	        
	}
	
	void locate_target() 
	{
	          double x = 0.0, y = 0.0;
	
	        	x = X_target - X_pos;
	        	y = Y_target - Y_pos;
	
	        	target_distance = sqrt((x*x)+(y*y));
				if (y == 0) y += .00001;
				/* no divide-by-zero allowed! */
				
				if (y > 0) target_bearing = (atan(x/y)*RADS);
				/*if(y < 0)*/
				else if(x<0)  target_bearing = (-PI+atan(x/y))*RADS;
				else target_bearing = (PI+atan(x / y))*RADS;
	        	

	        	heading_error = target_bearing - theta_D;
	        	if (heading_error > 180.0) heading_error -= 360.0;
			else if (heading_error < -180.0) heading_error += 360.0;
	
		
	}
	void turnToDegrees(double degree)
	{
		if (degree > 180)
		{
			degree -= 360;
		}
		else if(degree <-180)
		{
			degree += 180;
		}
		if (theta_D < degree)
		{
			turnRightToDegrees(degree);
		}
		else
		{
			turnLeftToDegrees(degree);
		}
	}
	void turnRightToDegrees(double degree)
	{
		while (theta_D <= degree )
		{
			
			odometers();
			WHEEL_BASE = 250;
			motors.right();
		}
		WHEEL_BASE = 221.5;
		motors.park();
	}

	void turnLeftToDegrees(double degree)
	{
		while (theta_D >= degree) 
		{
			odometers();
			WHEEL_BASE = 250;
			motors.left();
		}
		WHEEL_BASE = 221.5;
		motors.park();
	}

	void drive_dist(double drive_dist_mm) {
		double startMM = totalMM;
		drive_dist_mm = drive_dist_mm * FUDGEDIST;
		//since the origin might not be the start point of the drive_dist func, we need to account for that.
		while (abs(totalMM-startMM) <= drive_dist_mm)
		{
			motors.drive();
			odometers();
			
		}
		motors.park();
	}
	void rev_dist(double drive_dist_mm) {
		double startMM = totalMM;
		drive_dist_mm = drive_dist_mm * FUDGEDIST;
		//since the origin might not be the start point of the drive_dist func, we need to account for that.

		while (abs(totalMM - startMM) <= drive_dist_mm)
		{
			odometers();
			motors.reverse();
		}
		motors.park();
	}

	void go_and_get(double x,// x cordients. Goes perpindicular to the robot
		double y, //Y coordinants, goes parrelell with the robot
		float percentDistance //percent of the distance to travel before stoping, only defined behavior for between 0 and 1
	) {
		X_target = x;
		Y_target = y;
		motors.park(); //make sure that motors are off before starting
		locate_target();//calculate the degrees to turn and the distance to travel
		turnToDegrees(target_bearing); //turn to aling with the target
		drive_dist(target_distance*percentDistance); //drive toward the target
		
	}
};
#undef  knobLeft 

#undef  knobRight
#undef  motors
#undef FUDGEDIST
#endif // !Odometry_h