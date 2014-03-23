/*
 * Copyright 2013 Xavier Hosxe
 *
 * Author: Xavier Hosxe (xavier <.> hosxe < a t > gmail.com)
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


#include <math.h>
#include "Timbre.h"
#include "Voice.h"

enum ArpeggiatorDirection {
  ARPEGGIO_DIRECTION_UP = 0,
  ARPEGGIO_DIRECTION_DOWN,
  ARPEGGIO_DIRECTION_UP_DOWN,
  ARPEGGIO_DIRECTION_PLAYED,
  ARPEGGIO_DIRECTION_RANDOM
};

enum NewNoteType {
	NEW_NOTE_FREE = 0,
	NEW_NOTE_RELEASE,
	NEW_NOTE_OLD,
	NEW_NOTE_NONE
};


uint16_t lut_res_arpeggiator_patterns[]  = {
   21845,  62965,  46517,  54741,  43861,  22869,  38293,   2313,
   37449,  21065,  18761,  54553,  27499,  23387,  30583,  28087,
   22359,  28527,  30431,  43281,  28609,  53505,
};

const uint8_t midi_clock_tick_per_step[17]  = {
  192, 144, 96, 72, 64, 48, 36, 32, 24, 16, 12, 8, 6, 4, 3, 2, 1
};

extern float noise[32];


float panTable[] = {
		0.0000, 0.0007, 0.0020, 0.0036, 0.0055, 0.0077, 0.0101, 0.0128, 0.0156, 0.0186,
		0.0218, 0.0252, 0.0287, 0.0324, 0.0362, 0.0401, 0.0442, 0.0484, 0.0527, 0.0572,
		0.0618, 0.0665, 0.0713, 0.0762, 0.0812, 0.0863, 0.0915, 0.0969, 0.1023, 0.1078,
		0.1135, 0.1192, 0.1250, 0.1309, 0.1369, 0.1430, 0.1492, 0.1554, 0.1618, 0.1682,
		0.1747, 0.1813, 0.1880, 0.1947, 0.2015, 0.2085, 0.2154, 0.2225, 0.2296, 0.2369,
		0.2441, 0.2515, 0.2589, 0.2664, 0.2740, 0.2817, 0.2894, 0.2972, 0.3050, 0.3129,
		0.3209, 0.3290, 0.3371, 0.3453, 0.3536, 0.3619, 0.3703, 0.3787, 0.3872, 0.3958,
		0.4044, 0.4131, 0.4219, 0.4307, 0.4396, 0.4485, 0.4575, 0.4666, 0.4757, 0.4849,
		0.4941, 0.5034, 0.5128, 0.5222, 0.5316, 0.5411, 0.5507, 0.5604, 0.5700, 0.5798,
		0.5896, 0.5994, 0.6093, 0.6193, 0.6293, 0.6394, 0.6495, 0.6597, 0.6699, 0.6802,
		0.6905, 0.7009, 0.7114, 0.7218, 0.7324, 0.7430, 0.7536, 0.7643, 0.7750, 0.7858,
		0.7967, 0.8076, 0.8185, 0.8295, 0.8405, 0.8516, 0.8627, 0.8739, 0.8851, 0.8964,
		0.9077, 0.9191, 0.9305, 0.9420, 0.9535, 0.9651, 0.9767, 0.9883, 1.0000, 1.0000,
		1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
		1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
		1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
		1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
		1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
		1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
		1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
		1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
		1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
		1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
		1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
		1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
		1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000
} ;



// Static to all 4 timbres
unsigned int voiceIndex  __attribute__ ((section(".ccmnoload")));


Timbre::Timbre() {


    this->recomputeNext = true;
    this->currentGate = 0;
    this->sbMax = &this->sampleBlock[64];
    this->holdPedal = false;

    // arpegiator
    setNewBPMValue(90);
    arpegiatorStep = 0.0;
    idle_ticks_ = 96;
    running_ = 0;
    ignore_note_off_messages_ = 0;
    recording_ = 0;
    note_stack.Init();
    event_scheduler.Init();
    // Arpeggiator start
    Start();


    // Init FX variables
    v0L = v1L = v0R = v1R = 0.0f;
}

Timbre::~Timbre() {
}

void Timbre::init(int timbreNumber) {
    struct LfoParams* lfoParams[] = { &params.lfoOsc1, &params.lfoOsc2, &params.lfoOsc3};
    struct StepSequencerParams* stepseqparams[] = { &params.lfoSeq1, &params.lfoSeq2};
    struct StepSequencerSteps* stepseqs[] = { &params.lfoSteps1, &params.lfoSteps2};

    matrix.init(&params.matrixRowState1);

	env1.init(&matrix, &params.env1a,  &params.env1b, ENV1_ATTACK);
	env2.init(&matrix, &params.env2a,  &params.env2b, ENV2_ATTACK);
	env3.init(&matrix, &params.env3a,  &params.env3b, ENV3_ATTACK);
	env4.init(&matrix, &params.env4a,  &params.env4b, ENV4_ATTACK);
	env5.init(&matrix, &params.env5a,  &params.env5b, ENV5_ATTACK);
	env6.init(&matrix, &params.env6a,  &params.env6b, ENV6_ATTACK);

	osc1.init(&matrix, &params.osc1, OSC1_FREQ);
	osc2.init(&matrix, &params.osc2, OSC2_FREQ);
	osc3.init(&matrix, &params.osc3, OSC3_FREQ);
	osc4.init(&matrix, &params.osc4, OSC4_FREQ);
	osc5.init(&matrix, &params.osc5, OSC5_FREQ);
	osc6.init(&matrix, &params.osc6, OSC6_FREQ);

    // OSC
    for (int k = 0; k < NUMBER_OF_LFO_OSC; k++) {
        lfoOsc[k].init(lfoParams[k], &this->matrix, (SourceEnum)(MATRIX_SOURCE_LFO1 + k), (DestinationEnum)(LFO1_FREQ + k));
    }

    // ENV
    lfoEnv[0].init(&params.lfoEnv1 , &this->matrix, MATRIX_SOURCE_LFOENV1, (DestinationEnum)0);
    lfoEnv2[0].init(&params.lfoEnv2 , &this->matrix, MATRIX_SOURCE_LFOENV2, (DestinationEnum)LFOENV2_SILENCE);

    // Step sequencer
    for (int k = 0; k< NUMBER_OF_LFO_STEP; k++) {
        lfoStepSeq[k].init(stepseqparams[k], stepseqs[k], &matrix, (SourceEnum)(MATRIX_SOURCE_LFOSEQ1+k), (DestinationEnum)(LFOSEQ1_GATE+k));
    }
    this->timbreNumber = timbreNumber;
}

void Timbre::setVoiceNumber(int v, int n) {
	voiceNumber[v] = n;
	if (n >=0) {
		voices[n]->setCurrentTimbre(this);
	}
}


void Timbre::initVoicePointer(int n, Voice* voice) {
	voices[n] = voice;
}

void Timbre::noteOn(char note, char velocity) {
	if (params.engineApr1.clock) {
		arpeggiatorNoteOn(note, velocity);
	} else {
		preenNoteOn(note, velocity);
	}
}

void Timbre::noteOff(char note) {
	if (params.engineApr1.clock) {
		arpeggiatorNoteOff(note);
	} else {
		preenNoteOff(note);
	}
}

int cptHighNote = 0;

void Timbre::preenNoteOn(char note, char velocity) {
	if (unlikely(note < 10)) {
		return;
	}

	int iNov = (int) params.engine1.numberOfVoice;
	if (unlikely(iNov == 0)) {
		return;
	}

	unsigned int indexMin = (unsigned int)2147483647;
	int voiceToUse = -1;

	int newNoteType = NEW_NOTE_NONE;

	for (int k = 0; k < iNov; k++) {
		// voice number k of timbre
		int n = voiceNumber[k];

#ifdef DEBUG_VOICE
    	if (unlikely(n<0)) {
    		lcd.setRealTimeAction(true);
    		lcd.clear();
    		lcd.setCursor(0,0);
    		lcd.print("Timbre::NoteOn");
			lcd.setCursor(0,1);
			lcd.print("nov: ");
			lcd.print((int)params.engine1.numberOfVoice);
			lcd.setCursor(10,1);
			lcd.print("k: ");
			lcd.print(k);
			lcd.setCursor(0,2);
			lcd.print("n: ");
			lcd.print(n);
			lcd.setCursor(10,2);
			lcd.print("t: ");
			lcd.print(timbreNumber);
			lcd.setCursor(0,3);
			lcd.print("note: ");
			lcd.print((int)note);
//			while (1);
    	}
#endif

		// same note = priority 1 : take the voice immediatly
		if (unlikely(voices[n]->isPlaying() && voices[n]->getNote() == note)) {
#ifdef DEBUG_VOICE
		lcd.setRealTimeAction(true);
		lcd.setCursor(16,1);
		lcd.print(cptHighNote++);
		lcd.setCursor(16,2);
		lcd.print("S:");
		lcd.print(n);
#endif
			voices[n]->noteOnWithoutPop(note, velocity, voiceIndex++);
			return;
		}

		// unlikely because if it true, CPU is not full
		if (unlikely(newNoteType > NEW_NOTE_FREE)) {
			if (!voices[n]->isPlaying()) {
				voiceToUse = n;
				newNoteType = NEW_NOTE_FREE;
			}

			if (voices[n]->isReleased()) {
				int indexVoice = voices[n]->getIndex();
				if (indexVoice < indexMin) {
					indexMin = indexVoice;
					voiceToUse = n;
					newNoteType = NEW_NOTE_RELEASE;
				}
			}
		}
	}

	if (voiceToUse == -1) {
		newNoteType = NEW_NOTE_OLD;
		for (int k = 0; k < iNov; k++) {
			// voice number k of timbre
			int n = voiceNumber[k];
			int indexVoice = voices[n]->getIndex();
			if (indexVoice < indexMin && !voices[n]->isNewNotePending()) {
				indexMin = indexVoice;
				voiceToUse = n;
			}
		}
	}
	// All voices in newnotepending state ?
	if (voiceToUse != -1) {
#ifdef DEBUG_VOICE
		lcd.setRealTimeAction(true);
		lcd.setCursor(16,1);
		lcd.print(cptHighNote++);
		lcd.setCursor(16,2);
		switch (newNoteType) {
			case NEW_NOTE_FREE:
				lcd.print("F:");
				break;
			case NEW_NOTE_OLD:
				lcd.print("O:");
				break;
			case NEW_NOTE_RELEASE:
				lcd.print("R:");
				break;
		}
		lcd.print(voiceToUse);
#endif
		switch (newNoteType) {
		case NEW_NOTE_FREE:
			voices[voiceToUse]->noteOn(note, velocity, voiceIndex++);
			break;
		case NEW_NOTE_OLD:
		case NEW_NOTE_RELEASE:
			voices[voiceToUse]->noteOnWithoutPop(note, velocity, voiceIndex++);
			break;
		}

	}
}


void Timbre::preenNoteOff(char note) {
	int iNov = (int) params.engine1.numberOfVoice;
	for (int k = 0; k < iNov; k++) {
		// voice number k of timbre
		int n = voiceNumber[k];

		// Not playing = free CPU
		if (unlikely(!voices[n]->isPlaying())) {
			continue;
		}

		if (likely(voices[n]->getNextGlidingNote() == 0)) {
			if (voices[n]->getNote() == note) {
				if (unlikely(holdPedal)) {
					voices[n]->setHoldedByPedal(true);
					return;
				} else {
					voices[n]->noteOff();
					return;
				}
			}
		} else {
			// if gliding and releasing first note
			if (voices[n]->getNote() == note) {
				voices[n]->glideFirstNoteOff();
				return;
			}
			// if gliding and releasing next note
			if (voices[n]->getNextGlidingNote() == note) {
				voices[n]->glideToNote(voices[n]->getNote());
				voices[n]->glideFirstNoteOff();
				return;
			}
		}
	}
}


void Timbre::setHoldPedal(int value) {
	if (value <64) {
		holdPedal = false;
	    int numberOfVoices = params.engine1.numberOfVoice;
	    for (int k = 0; k < numberOfVoices; k++) {
	        // voice number k of timbre
	        int n = voiceNumber[k];
	        if (voices[n]->isHoldedByPedal()) {
	        	voices[n]->noteOff();
	        }
	    }
	    arpeggiatorSetHoldPedal(0);
	} else {
		holdPedal = true;
	    arpeggiatorSetHoldPedal(127);
	}
}




void Timbre::setNewBPMValue(float bpm) {
	ticksPerSecond = bpm * 24.0f / 60.0f;
	ticksEveryNCalls = calledPerSecond / ticksPerSecond;
	ticksEveyNCallsInteger = (int)ticksEveryNCalls;
}

void Timbre::setArpeggiatorClock(float clockValue) {
	if (clockValue == CLOCK_OFF) {
		FlushQueue();
		note_stack.Clear();
	}
	if (clockValue == CLOCK_INTERNAL) {
	    setNewBPMValue(params.engineApr1.BPM);
	}
	if (clockValue == CLOCK_EXTERNAL) {
		// Let's consider we're running
		running_ = 1;
	}
}


void Timbre::prepareForNextBlock() {

	// Apeggiator clock : internal
	if (params.engineApr1.clock == CLOCK_INTERNAL) {
		arpegiatorStep+=1.0f;
		if (unlikely((arpegiatorStep) > ticksEveryNCalls)) {
			arpegiatorStep -= ticksEveyNCallsInteger;

			Tick();
		}
	}

	this->lfoOsc[0].nextValueInMatrix();
	this->lfoOsc[1].nextValueInMatrix();
	this->lfoOsc[2].nextValueInMatrix();
	this->lfoEnv[0].nextValueInMatrix();
	this->lfoEnv2[0].nextValueInMatrix();
	this->lfoStepSeq[0].nextValueInMatrix();
	this->lfoStepSeq[1].nextValueInMatrix();

    this->matrix.computeAllFutureDestintationAndSwitch();

    updateAllModulationIndexes();
    updateAllMixOscsAndPans();

}

void Timbre::cleanNextBlock() {
	float *sp = this->sampleBlock;
	while (sp < this->sbMax) {
		*sp++ = 0;
		*sp++ = 0;
		*sp++ = 0;
		*sp++ = 0;
		*sp++ = 0;
		*sp++ = 0;
		*sp++ = 0;
		*sp++ = 0;
	}
}



#define GATE_INC 0.02f

void Timbre::fxAfterBlock(float ratioTimbres) {
    // Gate algo !!
    float gate = this->matrix.getDestination(MAIN_GATE);
    if (unlikely(gate > 0 || currentGate > 0)) {
		gate *=.72547132656922730694f; // 0 < gate < 1.0
		if (gate > 1.0f) {
			gate = 1.0f;
		}
		float incGate = (gate - currentGate) * .03125f; // ( *.03125f = / 32)
		// limit the speed.
		if (incGate > 0.002f) {
			incGate = 0.002f;
		} else if (incGate < -0.002f) {
			incGate = -0.002f;
		}

		float *sp = this->sampleBlock;
		float coef;
    	for (int k=0 ; k< BLOCK_SIZE ; k++) {
			currentGate += incGate;
			coef = 1.0f - currentGate;
			*sp = *sp * coef;
			sp++;
			*sp = *sp * coef;
			sp++;
		}
    //    currentGate = gate;
    }

    // LP Algo
    int effectType = params.effect.type;
    float gainTmp =  params.effect.param3 * numberOfVoiceInverse * ratioTimbres;
    mixerGain = 0.02f * gainTmp + .98  * mixerGain;

    switch (effectType) {
    case FILTER_LP:
    {
    	float fxParamTmp = params.effect.param1 + matrix.getDestination(FILTER_FREQUENCY);
    	fxParamTmp *= fxParamTmp;

    	// Low pass... on the Frequency
    	fxParam1 = (fxParamTmp + 9.0f * fxParam1) * .1f;
    	if (unlikely(fxParam1 > 1.0f)) {
    		fxParam1 = 1.0f;
    	}
    	if (unlikely(fxParam1 < 0.0f)) {
    		fxParam1 = 0.0f;
    	}

    	float pattern = (1 - fxParam2 * fxParam1);

    	float *sp = this->sampleBlock;
    	float localv0L = v0L;
    	float localv1L = v1L;
    	float localv0R = v0R;
    	float localv1R = v1R;

    	for (int k=0 ; k<BLOCK_SIZE  ; k++) {

    		// Left voice
    		localv0L =  pattern * localv0L  -  (fxParam1) * localv1L  + (fxParam1)* (*sp);
    		localv1L =  pattern * localv1L  +  (fxParam1) * localv0L;

    		*sp = localv1L * mixerGain;

    		if (unlikely(*sp > ratioTimbres)) {
    			*sp = ratioTimbres;
    		}
    		if (unlikely(*sp < -ratioTimbres)) {
    			*sp = -ratioTimbres;
    		}

    		sp++;

    		// Right voice
    		localv0R =  pattern * localv0R  -  (fxParam1)*localv1R  + (fxParam1)* (*sp);
    		localv1R =  pattern * localv1R  +  (fxParam1)*localv0R;

    		*sp = localv1R * mixerGain;

    		if (unlikely(*sp > ratioTimbres)) {
    			*sp = ratioTimbres;
    		}
    		if (unlikely(*sp < -ratioTimbres)) {
    			*sp = -ratioTimbres;
    		}

    		sp++;
    	}
    	v0L = localv0L;
    	v1L = localv1L;
    	v0R = localv0R;
    	v1R = localv1R;
    }
    break;
    case FILTER_HP:
    {
    	float fxParamTmp = params.effect.param1 + matrix.getDestination(FILTER_FREQUENCY);
    	fxParamTmp *= fxParamTmp;

    	// Low pass... on the Frequency
    	fxParam1 = (fxParamTmp + 9.0f * fxParam1) * .1f;
    	if (unlikely(fxParam1 > 1.0)) {
    		fxParam1 = 1.0;
    	}
    	float pattern = (1 - fxParam2 * fxParam1);

    	float *sp = this->sampleBlock;
    	float localv0L = v0L;
    	float localv1L = v1L;
    	float localv0R = v0R;
    	float localv1R = v1R;

    	for (int k=0 ; k<BLOCK_SIZE ; k++) {

    		// Left voice
    		localv0L =  pattern * localv0L  -  (fxParam1) * localv1L  + (fxParam1) * (*sp);
    		localv1L =  pattern * localv1L  +  (fxParam1) * localv0L;

    		*sp = (*sp - localv1L) * mixerGain;

    		if (unlikely(*sp > ratioTimbres)) {
    			*sp = ratioTimbres;
    		}
    		if (unlikely(*sp < -ratioTimbres)) {
    			*sp = -ratioTimbres;
    		}

    		sp++;

    		// Right voice
    		localv0R =  pattern * localv0R  -  (fxParam1) * localv1R  + (fxParam1) * (*sp);
    		localv1R =  pattern * localv1R  +  (fxParam1) * localv0R;

    		*sp = (*sp - localv1R) * mixerGain;

    		if (unlikely(*sp > ratioTimbres)) {
    			*sp = ratioTimbres;
    		}
    		if (unlikely(*sp < -ratioTimbres)) {
    			*sp = -ratioTimbres;
    		}

    		sp++;
    	}
    	v0L = localv0L;
    	v1L = localv1L;
    	v0R = localv0R;
    	v1R = localv1R;

    }
	break;
    case FILTER_BASS:
    {
    	// From musicdsp.com
    	//    	Bass Booster
    	//
    	//    	Type : LP and SUM
    	//    	References : Posted by Johny Dupej
    	//
    	//    	Notes :
    	//    	This function adds a low-passed signal to the original signal. The low-pass has a quite wide response.
    	//
    	//    	selectivity - frequency response of the LP (higher value gives a steeper one) [70.0 to 140.0 sounds good]
    	//    	ratio - how much of the filtered signal is mixed to the original
    	//    	gain2 - adjusts the final volume to handle cut-offs (might be good to set dynamically)

    	//static float selectivity, gain1, gain2, ratio, cap;
    	//gain1 = 1.0/(selectivity + 1.0);
    	//
    	//cap= (sample + cap*selectivity )*gain1;
    	//sample = saturate((sample + cap*ratio)*gain2);

    	float *sp = this->sampleBlock;
    	float localv0L = v0L;
    	float localv0R = v0R;

    	for (int k=0 ; k<BLOCK_SIZE ; k++) {

    		localv0L = ((*sp) + localv0L * fxParam1) * fxParam3;
    		(*sp) = ((*sp) + localv0L * fxParam2) * mixerGain;

    		if (unlikely(*sp > ratioTimbres)) {
    			*sp = ratioTimbres;
    		}
    		if (unlikely(*sp < -ratioTimbres)) {
    			*sp = -ratioTimbres;
    		}

    		sp++;

    		localv0R = ((*sp) + localv0R * fxParam1) * fxParam3;
    		(*sp) = ((*sp) + localv0R * fxParam2) * mixerGain;

    		if (unlikely(*sp > ratioTimbres)) {
    			*sp = ratioTimbres;
    		}
    		if (unlikely(*sp < -ratioTimbres)) {
    			*sp = -ratioTimbres;
    		}

    		sp++;
    	}
    	v0L = localv0L;
    	v0R = localv0R;

    }
    break;
    case FILTER_MIXER:
    {
    	float pan = params.effect.param1 * 2 - 1.0f ;
    	float *sp = this->sampleBlock;
    	float sampleR, sampleL;
    	if (pan <= 0) {
        	float onePlusPan = 1 + pan;
        	float minusPan = - pan;
        	for (int k=0 ; k<BLOCK_SIZE  ; k++) {
				sampleL = *(sp);
				sampleR = *(sp + 1);

				*sp = (sampleL + sampleR * minusPan) * mixerGain;
				sp++;
				*sp = sampleR * onePlusPan * mixerGain;
				sp++;
			}
    	} else if (pan > 0) {
        	float oneMinusPan = 1 - pan;
        	float adjustedmixerGain = (pan * .5) * mixerGain;
        	for (int k=0 ; k<BLOCK_SIZE ; k++) {
				sampleL = *(sp);
				sampleR = *(sp + 1);

				*sp = sampleL * oneMinusPan * mixerGain;
				sp++;
				*sp = (sampleR + sampleL * pan) * mixerGain;
				sp++;
			}
    	}
    }
    break;
    case FILTER_OFF:
    {
    	// Filter off has gain...
    	float *sp = this->sampleBlock;
    	for (int k=0 ; k<BLOCK_SIZE ; k++) {
			*sp++ = (*sp) * mixerGain;
			*sp++ = (*sp) * mixerGain;
		}
    }
    break;
    case FILTER_LP4:
    {
    	// musicdsp.org
    	//
		//    	Perfect LP4 filter
		//
		//    	Type : LP
		//    	References : Posted by azertopia at free dot fr
		//
		//    	Notes :
		//    	hacked from the exemple of user script in FL Edison

//    	float Frq = params.effect.param1 * PREENFM_FREQUENCY / 2;
//    	float res = params.effect.param2;
//    	float SR = PREENFM_FREQUENCY;
//    	float f = (Frq+Frq) / SR;
//    	float p = f * (1.8 - 0.8 * f);
//    	float k=p+p-1.0;
//    	float t=(1.0-p)*1.386249;
//    	float t2=12.0+t*t;
//    	float r = res*(t2+6.0*t)/(t2-6.0*t);
//
//    	float *sp = this->sampleBlock;


//    	while (sp < this->sbMax) {
//    		  f := (Frq+Frq) / SR;
//    		  p:=f*(1.8-0.8*f);
//    		  k:=p+p-1.0;
//    		  t:=(1.0-p)*1.386249;
//    		  t2:=12.0+t*t;
//    		  r := res*(t2+6.0*t)/(t2-6.0*t);

//    		  x := inp - r*y4;
//    		  y1:=x*p + oldx*p - k*y1;
//    		  y2:=y1*p+oldy1*p - k*y2;
//    		  y3:=y2*p+oldy2*p - k*y3;
//    		  y4:=y3*p+oldy3*p - k*y4;
//    		  y4 := y4 - ((y4*y4*y4)/6.0);
//    		  oldx := x;
//    		  oldy1 := y1+_kd;
//    		  oldy2 := y2+_kd;;
//    		  oldy3 := y3+_kd;;
//    		  outlp := y4;

//    		float x = (*sp) - r * v4L;
//    		v5L= x * p + v0L * p - k * v5L;
//    		v6L= v5L * p + v1L * p - k * v6L;
//    		v7L= v6L * p + v2L * p - k * v7L;
//    		v4L= v7L * p + v3L * p - k * v4L;
//    		v4L = v4L - ((v4L*v4L*v4L)/6.0);
//    		v0L = x;
//        	v1L = v5L;
//        	v2L = v6L;
//        	v3L = v7L;
//        	*sp = v4L;
//    		sp++;
//
//    		x = (*sp) - r * v4R;
//    		v5R= x * p + v0R * p - k * v5R;
//    		v6R= v5R * p + v1R * p - k * v6R;
//    		v7R= v6R * p + v2R * p - k * v7R;
//    		v4R= v7R * p + v3R * p - k * v4R;
//    		v4R = v4R - ((v4R*v4R*v4R)/6.0);
//    		v0R = x;
//        	v1R = v5R;
//        	v2R = v6R;
//        	v3R = v7R;
//        	*sp = v4R;
//    		sp++;


//    	}
    }
    break;

    default:
    	// NO EFFECT
   	break;
    }

}


void Timbre::afterNewParamsLoad() {
    this->matrix.resetSources();
    this->matrix.resetAllDestination();
	 for (int j=0; j<NUMBER_OF_ENCODERS * 2; j++) {
		this->env1.reloadADSR(j);
		this->env2.reloadADSR(j);
		this->env3.reloadADSR(j);
		this->env4.reloadADSR(j);
		this->env5.reloadADSR(j);
		this->env6.reloadADSR(j);
	}

	for (int j=0; j<NUMBER_OF_ENCODERS; j++) {
		this->lfoOsc[0].valueChanged(j);
		this->lfoOsc[1].valueChanged(j);
		this->lfoOsc[2].valueChanged(j);
		this->lfoEnv[0].valueChanged(j);
		this->lfoEnv2[0].valueChanged(j);
		this->lfoStepSeq[0].valueChanged(j);
		this->lfoStepSeq[1].valueChanged(j);
	}

    resetArpeggiator();
    v0L = v1L = 0.0f;
    v0R = v1R = 0.0f;
    for (int k=0; k<NUMBER_OF_ENCODERS; k++) {
    	setNewEffecParam(k);
    }
}


void Timbre::resetArpeggiator() {
	// Reset Arpeggiator
	FlushQueue();
	note_stack.Clear();
	setArpeggiatorClock(params.engineApr1.clock);
	setLatchMode(params.engineApr2.latche);
}


void Timbre::setNewValue(int index, struct ParameterDisplay* param, float newValue) {
    if (newValue > param->maxValue) {
        newValue= param->maxValue;
    } else if (newValue < param->minValue) {
        newValue= param->minValue;
    }
    ((float*)&params)[index] = newValue;
}

void Timbre::setNewEffecParam(int encoder) {
	if (encoder == 0) {
	    v0L = v1L = 0.0f;
	    v0R = v1R = 0.0f;
	}
	switch ((int)params.effect.type) {
	case FILTER_BASS:
		// Selectivity = fxParam1
		// ratio = fxParam2
		// gain1 = fxParam3
		fxParam1 = 50 + 200 * params.effect.param1;
		fxParam2 = params.effect.param2 * 4;
		fxParam3 = 1.0/(fxParam1 + 1.0);
		break;
	case FILTER_HP:
	case FILTER_LP:
		switch (encoder) {
		case ENCODER_EFFECT_TYPE:
			fxParam2 = 0.3f - params.effect.param2 * 0.3f;
			break;
		case ENCODER_EFFECT_PARAM1:
			// Done in every loop
			// fxParam1 = pow(0.5, (128- (params.effect.param1 * 128))   / 16.0);
			break;
		case ENCODER_EFFECT_PARAM2:
	    	// fxParam2 = pow(0.5, ((params.effect.param2 * 127)+24) / 16.0);
			// => value from 0.35 to 0.0
			fxParam2 = 0.27f - params.effect.param2 * 0.27f;
			break;
		}
    	break;
	}

}


// Code bellowed have been adapted by Xavier Hosxe for PreenFM2
// It come from Muteable Instrument midiPAL

/////////////////////////////////////////////////////////////////////////////////
// Copyright 2011 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// -----------------------------------------------------------------------------
//
// Arpeggiator app.



void Timbre::arpeggiatorNoteOn(char note, char velocity) {
	// CLOCK_MODE_INTERNAL
	if (params.engineApr1.clock == CLOCK_INTERNAL) {
		if (idle_ticks_ >= 96 || !running_) {
			Start();
		}
		idle_ticks_ = 0;
	}

	if (latch_ && !recording_) {
		note_stack.Clear();
		recording_ = 1;
	}
	note_stack.NoteOn(note, velocity);
}


void Timbre::arpeggiatorNoteOff(char note) {
	if (ignore_note_off_messages_) {
		return;
	}
	if (!latch_) {
		note_stack.NoteOff(note);
	} else {
		if (note == note_stack.most_recent_note().note) {
			recording_ = 0;
		}
	}
}


void Timbre::OnMidiContinue() {
	if (params.engineApr1.clock == CLOCK_EXTERNAL) {
		running_ = 1;
	}
}

void Timbre::OnMidiStart() {
	if (params.engineApr1.clock == CLOCK_EXTERNAL) {
		Start();
	}
}

void Timbre::OnMidiStop() {
	if (params.engineApr1.clock == CLOCK_EXTERNAL) {
		running_ = 0;
		FlushQueue();
	}
}


void Timbre::OnMidiClock() {
	if (params.engineApr1.clock == CLOCK_EXTERNAL && running_) {
		Tick();
	}
}




void Timbre::SendLater(uint8_t note, uint8_t velocity, uint8_t when, uint8_t tag) {
	event_scheduler.Schedule(note, velocity, when, tag);
}


void Timbre::SendScheduledNotes() {
  uint8_t current = event_scheduler.root();
  while (current) {
    const SchedulerEntry& entry = event_scheduler.entry(current);
    if (entry.when) {
      break;
    }
    if (entry.note != kZombieSlot) {
      if (entry.velocity == 0) {
    	  preenNoteOff(entry.note);
      } else {
    	  preenNoteOn(entry.note, entry.velocity);
      }
    }
    current = entry.next;
  }
  event_scheduler.Tick();
}


void Timbre::FlushQueue() {
  while (event_scheduler.size()) {
    SendScheduledNotes();
  }
}



void Timbre::Tick() {
	++tick_;

	if (note_stack.size()) {
		idle_ticks_ = 0;
	}
	++idle_ticks_;
	if (idle_ticks_ >= 96) {
		idle_ticks_ = 96;
	    if (params.engineApr1.clock == CLOCK_INTERNAL) {
	      running_ = 0;
	      FlushQueue();
	    }
	}

	SendScheduledNotes();

	if (tick_ >= midi_clock_tick_per_step[(int)params.engineApr2.division]) {
		tick_ = 0;
		uint16_t pattern = lut_res_arpeggiator_patterns[(int)params.engineApr2.pattern - 1];
		uint8_t has_arpeggiator_note = (bitmask_ & pattern) ? 255 : 0;
		if (note_stack.size() && has_arpeggiator_note) {
			StepArpeggio();
			const NoteEntry &noteEntry = ARPEGGIO_DIRECTION_PLAYED == params.engineApr1.direction
			  ? note_stack.played_note(current_step_)
			  : note_stack.sorted_note(current_step_);

			uint8_t note = noteEntry.note;
			uint8_t velocity = noteEntry.velocity;
			note += 12 * current_octave_;

	    	while (note > 127) {
				note -= 12;
			}
			// If there are some Note Off messages for the note about to be triggeered
			// remove them from the queue and process them now.
			if (event_scheduler.Remove(note, 0)) {
				preenNoteOff(note);
			}
			// Send a note on and schedule a note off later.
			preenNoteOn(note, velocity);
			event_scheduler.Schedule(note, 0, midi_clock_tick_per_step[(int)params.engineApr2.duration] - 1, 0);
		}
		bitmask_ <<= 1;
		if (!bitmask_) {
			bitmask_ = 1;
		}
	}
}



void Timbre::StepArpeggio() {

	if (current_octave_ == 127) {
		StartArpeggio();
		return;
	}

	uint8_t num_notes = note_stack.size();
	if (params.engineApr1.direction == ARPEGGIO_DIRECTION_RANDOM) {
		uint8_t random_byte = *(uint8_t*)noise;
		current_octave_ = random_byte & 0xf;
		current_step_ = (random_byte & 0xf0) >> 4;
		while (current_octave_ >= params.engineApr1.octave) {
			current_octave_ -= params.engineApr1.octave;
		}
		while (current_step_ >= num_notes) {
			current_step_ -= num_notes;
		}
	} else {
		current_step_ += current_direction_;
		uint8_t change_octave = 0;
		if (current_step_ >= num_notes) {
			current_step_ = 0;
			change_octave = 1;
		} else if (current_step_ < 0) {
			current_step_ = num_notes - 1;
			change_octave = 1;
		}
		if (change_octave) {
			current_octave_ += current_direction_;
			if (current_octave_ >= params.engineApr1.octave || current_octave_ < 0) {
				if (params.engineApr1.direction == ARPEGGIO_DIRECTION_UP_DOWN) {
					current_direction_ = -current_direction_;
					StartArpeggio();
					if (num_notes > 1 || params.engineApr1.octave > 1) {
						StepArpeggio();
					}
				} else {
					StartArpeggio();
				}
			}
		}
	}
}

void Timbre::StartArpeggio() {

  if (current_direction_ == 1) {
    current_octave_ = 0;
    current_step_ = 0;
  } else {
    current_step_ = note_stack.size() - 1;
    current_octave_ = params.engineApr1.octave - 1;
  }
}

void Timbre::Start() {
	bitmask_ = 1;
	recording_ = 0;
	running_ = 1;
	tick_ = midi_clock_tick_per_step[(int)params.engineApr2.division] - 1;
    current_octave_ = 127;
	current_direction_ = (params.engineApr1.direction == ARPEGGIO_DIRECTION_DOWN ? -1 : 1);
}


void Timbre::arpeggiatorSetHoldPedal(uint8_t value) {
  if (ignore_note_off_messages_ && !value) {
    // Pedal was released, kill all pending arpeggios.
    note_stack.Clear();
  }
  ignore_note_off_messages_ = value;
}


void Timbre::setLatchMode(uint8_t value) {
    // When disabling latch mode, clear the note stack.
	latch_ = value;
    if (value == 0) {
      note_stack.Clear();
      recording_ = 0;
    }
}

void Timbre::setDirection(uint8_t value) {
	// When changing the arpeggio direction, reset the pattern.
	current_direction_ = (value == ARPEGGIO_DIRECTION_DOWN ? -1 : 1);
	StartArpeggio();
}

void Timbre::lfoValueChange(int currentRow, int encoder, float newValue) {
	switch (currentRow) {
	case ROW_LFOOSC1:
	case ROW_LFOOSC2:
	case ROW_LFOOSC3:
		lfoOsc[currentRow - ROW_LFOOSC1].valueChanged(encoder);
		break;
	case ROW_LFOENV1:
		lfoEnv[0].valueChanged(encoder);
		break;
	case ROW_LFOENV2:
		lfoEnv2[0].valueChanged(encoder);
		break;
	case ROW_LFOSEQ1:
	case ROW_LFOSEQ2:
		lfoStepSeq[currentRow - ROW_LFOSEQ1].valueChanged(encoder);
		break;
	}
}


