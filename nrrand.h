
#ifndef NRRAND_H
#define NRRAND_H

#include "assert.h"
#include <math.h>

// Random number generators adapted from Numerical Recipes 3rd edition, 2007

struct NRRandom {
	unsigned long long u, v, w;

	NRRandom( unsigned long long seed=1 ) {
		setSeed( seed );
	}

	void setSeed( unsigned long long seed ) {
		v = 4101842887655102017LL;
		w = 1;

		u = seed ^ v;
		randInt64();

		v = u;
		randInt64();

		w = v;
		randInt64();
	}

	inline unsigned long long randFastestInt64() {
		v ^= v >> 21;
		v ^= v << 35;
		v ^= v >> 4;
		return v;
	}

	inline unsigned long long randFasterInt64() {
		v ^= v >> 21;
		v ^= v << 35;
		v ^= v >> 4;
		return v * 2685821657736338717LL;
	}

	inline double randFastestDouble() {
		return 5.42101086242752217e-20 * randFastestInt64();
	}

	inline double randFastDouble() {
		return 5.42101086242752217e-20 * randFasterInt64();
	}


	inline unsigned long long randInt64() {
		u = u * 2862933555777941757LL + 7046029254386353087LL;
		v ^= v >> 17;
		v ^= v << 31;
		v ^= v >> 8;
		w = 4294957665U * ( w & 0xFFFFFFFF) + ( w >> 32 );
		unsigned long long x = u ^ ( u << 21 );
		x ^= x >> 35;
		x ^= x << 4;
		return (x + v) ^ w;

	}

	inline double randDouble() {
		return 5.42101086242752217e-20 * randInt64();
	}

	inline unsigned int randInt32() {
		return (unsigned int)randInt64();
	}
};


struct NRRandomNormal : NRRandom {
	double mu, sig;

	NRRandomNormal( double _mu, double _sig, unsigned long long seed ) : NRRandom( seed ) {
		mu = _mu;
		sig = _sig;
	}

	double randNormal() {
		double u, v, x, y, q;
		do {
			u = randDouble();
			v = 1.7156 * ( randDouble() - 0.5 );
			x = u - 0.449871;
			y = fabs(v) + 0.386595;
			q = x*x + y * (0.19600*y - 0.25472*x);
		} while(
			q > 0.27597
			&& ( // Rarely evaluated
				q > 0.27846 || v*v > -4 * log(u) * u*u
			)
		);
		return mu + sig * v / u;
	}
};

struct NRRandomGamma : NRRandomNormal {
	double alph, oalph, bet;
	double a1, a2;

	NRRandomGamma( double _alph, double _bet, unsigned long long seed ) : NRRandomNormal( 0.0, 1.0, seed ) {
		alph = _alph;
		oalph = _alph;
		bet = _bet;
		assert( alph > 0.0 );
		if( alph < 1.0 ) alph += 1.0;
		a1 = alph - 1.0 / 3.0;
		a2 = 1.0 / sqrt( 9.0 * a1 );
	}

	double randGamma() {
		double u, v, x;
		do {
			do {
				x = randNormal();
				v = 1.0 + a2 * x;
			} while( v <= 0.0 );
			v = v*v*v;
			u = randDouble();
		} while(
			u > 1.0 - 0.331 * x*x*x*x
			&& ( // Rarely evaluated
				log(u) > 0.5*x*x + a1*(1.0 - v + log(v))
			)
		);

		if( alph == oalph ) {
			return a1 * v / bet;
		}
		else {
			do {
				u = randDouble();
			} while( u == 0.0 );
			return pow( u, 1.0 / oalph) * a1 * v / bet;
		}
	}

	double randGamma( double _alph, double _bet ) {
		// This duplicate function allows a single driver to be called with a different alpha each time
	
		alph = _alph;
		oalph = _alph;
		bet = _bet;
		assert( alph > 0.0 );
		if( alph < 1.0 ) alph += 1.0;
		a1 = alph - 1.0 / 3.0;
		a2 = 1.0 / sqrt( 9.0 * a1 );

		double u, v, x;
		do {
			do {
				x = randNormal();
				v = 1.0 + a2 * x;
			} while( v <= 0.0 );
			v = v*v*v;
			u = randDouble();
		} while(
			u > 1.0 - 0.331 * x*x*x*x
			&& ( // Rarely evaluated
				log(u) > 0.5*x*x + a1*(1.0 - v + log(v))
			)
		);

		if( alph == oalph ) {
			return a1 * v / bet;
		}
		else {
			do {
				u = randDouble();
			} while( u == 0.0 );
			return pow( u, 1.0 / oalph) * a1 * v / bet;
		}
	}

};

#endif
