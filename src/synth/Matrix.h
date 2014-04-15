/*
 * Copyright 2013 Xavier Hosxe
 *
 * Author: Xavier Hosxe (xavier . hosxe (at) gmail . com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef MATRIX_H_
#define MATRIX_H_

#define MATRIX_SIZE 12

#include "Common.h"



class Matrix  {
	friend class Timbre;
public:
    Matrix();
    ~Matrix();

	void init(struct MatrixRowParams* matrixRows);

	void resetSources() {
        for (int k=0; k< MATRIX_SOURCE_MAX; k++) {
        	setSource((SourceEnum)k, 0);
        }
	}

    void resetAllDestination() {
        for (int k=0; k< DESTINATION_MAX; k++) {
            currentDestinations[k] = 0;
            futurDestinations[k] = 0;
        }
    }

    void resetDestination(int k) {
    	currentDestinations[k] = 0;
		futurDestinations[k] = 0;
    }


    void resetUsedFuturDestination() {
        for (int k=0; k< MATRIX_SIZE; ) {
        	futurDestinations[(int)rows[k++].destination] = 0;
            futurDestinations[(int)rows[k++].destination] = 0;
            futurDestinations[(int)rows[k++].destination] = 0;
            futurDestinations[(int)rows[k++].destination] = 0;
        }
    }

    void computeAllFutureDestintationAndSwitch() {
        float mul;
		/*
		resetUsedFuturDestination();
		mul = rows[0].mul + currentDestinations[MTX1_MUL];
		futurDestinations[(int)rows[0].destination] += sources[(int)rows[0].source] * mul;
		mul = rows[1].mul + currentDestinations[MTX2_MUL];
        futurDestinations[(int)rows[1].destination] += sources[(int)rows[1].source] * mul;
        mul = rows[2].mul + currentDestinations[MTX3_MUL];
        futurDestinations[(int)rows[2].destination] += sources[(int)rows[2].source] * mul;
        mul = rows[3].mul + currentDestinations[MTX4_MUL];
        futurDestinations[(int)rows[3].destination] += sources[(int)rows[3].source] * mul;
        futurDestinations[(int)rows[4].destination] += sources[(int)rows[4].source] * rows[4].mul;
        futurDestinations[(int)rows[5].destination] += sources[(int)rows[5].source] * rows[5].mul;
        futurDestinations[(int)rows[6].destination] += sources[(int)rows[6].source] * rows[6].mul;
        futurDestinations[(int)rows[7].destination] += sources[(int)rows[7].source] * rows[7].mul;
        futurDestinations[(int)rows[8].destination] += sources[(int)rows[8].source] * rows[8].mul;
        futurDestinations[(int)rows[9].destination] += sources[(int)rows[9].source] * rows[9].mul;
        futurDestinations[(int)rows[10].destination] += sources[(int)rows[10].source] * rows[10].mul;
        futurDestinations[(int)rows[11].destination] += sources[(int)rows[11].source] * rows[11].mul;
		*/

		//		resetUsedFuturDestination();
		float *dest = futurDestinations;
		const float *cur = currentDestinations + MTX1_MUL;
		const float *src = sources;
		const struct MatrixRowParams *row = rows;

#if 0
		mul = rows[0].mul + currentDestinations[MTX1_MUL];
		futurDestinations[(int)rows[0].destination] = sources[(int)rows[0].source] * mul;
		mul = rows[1].mul + currentDestinations[MTX2_MUL];
        futurDestinations[(int)rows[1].destination] = sources[(int)rows[1].source] * mul;
        mul = rows[2].mul + currentDestinations[MTX3_MUL];
        futurDestinations[(int)rows[2].destination] = sources[(int)rows[2].source] * mul;
        mul = rows[3].mul + currentDestinations[MTX4_MUL];
        futurDestinations[(int)rows[3].destination] = sources[(int)rows[3].source] * mul;
#else
		for ( int r=0; r < 4; ++r ) {
			asm volatile( "\n\t"
						  "vldmia %[row]!, {s0-s3}" "\n\t" // struct MatrixRowParams = { float source, float mul, float destination, float }
						  "vcvt.s32.f32 s0, s0" "\n\t"
						  "vcvt.s32.f32 s2, s2" "\n\t"
						  "vmov r4, s0" "\n\t"
						  "vmov r5, s2" "\n\t"
						  "add r4, %[source], r4, lsl #2" "\n\t" // r4 = %[source] + row.source * 4
						  "add r5, %[dest], r5, lsl #2" "\n\t" // r5 = %[dest] + row.destination * 4
						  "vldr.32 s4, [%[cur]]" "\n\t" // s4 = *currentDestinations++
						  "vldr.32 s0, [r4]" "\n\t" // sources[(int)rows[0].source]
						  "vadd.f32 s1, s1, s4" "\n\t" // row.mul + currentDestinations[MTX1_MUL + r]
						  "add %[cur], %[cur], #4" "\n\t"
						  "vmul.f32 s2, s0, s1" "\n\t" // sources[(int)rows[0].source] * mul
						  "vstr.f32 s0, [r5]" "\n\t"
						  : [row] "+r" (row), [cur] "+r" (cur)
						  : [source] "r" (src), [dest] "r" (dest)
						  : "r4", "r5", "s0", "s1", "s2", "s3", "s4"
						  );
		}
#endif

		/*
        futurDestinations[(int)rows[4].destination] = sources[(int)rows[4].source] * rows[4].mul;
        futurDestinations[(int)rows[5].destination] = sources[(int)rows[5].source] * rows[5].mul;
        futurDestinations[(int)rows[6].destination] = sources[(int)rows[6].source] * rows[6].mul;
        futurDestinations[(int)rows[7].destination] = sources[(int)rows[7].source] * rows[7].mul;
        futurDestinations[(int)rows[8].destination] = sources[(int)rows[8].source] * rows[8].mul;
        futurDestinations[(int)rows[9].destination] = sources[(int)rows[9].source] * rows[9].mul;
        futurDestinations[(int)rows[10].destination] = sources[(int)rows[10].source] * rows[10].mul;
        futurDestinations[(int)rows[11].destination] = sources[(int)rows[11].source] * rows[11].mul;
		*/
		for ( int r=4; r < MATRIX_SIZE; ++r ) {
			asm volatile( "\n\t"
						  "vldmia %[row]!, {s0-s3}" "\n\t" // struct MatrixRowParams = { float source, float mul, float destination, float }
						  "vcvt.s32.f32 s0, s0" "\n\t"
						  "vcvt.s32.f32 s2, s2" "\n\t"
						  "vmov r4, s0" "\n\t"
						  "vmov r5, s2" "\n\t"
						  "add r4, %[source], r4, lsl #2" "\n\t" // r4 = %[source] + row.source * 4
						  "add r5, %[dest], r5, lsl #2" "\n\t" // r5 = %[dest] + row.destination * 4
						  "vldr.32 s0, [r4]" "\n\t"
						  "vmul.f32 s0, s0, s1" "\n\t"
						  "vstr.f32 s0, [r5]" "\n\t"
						  : [row] "+r" (row)
						  : [source] "r" (src), [dest] "r" (dest)
						  : "r4", "r5", "s0", "s1", "s2", "s3"
						  );
		}

        useNewValues();
    }

    void setSource(SourceEnum source, float value) __attribute__((always_inline)) {
        this->sources[source] = value;
    }

    float getDestination(DestinationEnum destination)   __attribute__((always_inline))  {
        return this->currentDestinations[destination];
    }

    void useNewValues() {
        if (currentDestinations == destinations1) {
            currentDestinations = destinations2;
            futurDestinations = destinations1;
        } else {
            currentDestinations = destinations1;
            futurDestinations = destinations2;
        }
    }

private:
    float sources[MATRIX_SOURCE_MAX];
    float destinations1[DESTINATION_MAX];
    float destinations2[DESTINATION_MAX];
    float *currentDestinations;
    float *futurDestinations;
    struct MatrixRowParams* rows;
};

#endif /* MATRIX_H_ */
