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

#include <stdlib.h>
#include "Synth.h"
#include "Menu.h"
#include "stm32f4xx_rng.h"


#include "hardware/dwt.h"

#ifdef DEBUG
CYCCNT_buffer cycles_rng, cycles_voices1, cycles_voices2, cycles_fx, cycles_timbres;
#endif

extern float noise[32];
float ratiosTimbre[]= { 131072.0f * 1.0f, 131072.0f * 1.0f, 131072.0f *  0.5f, 131072.0f * 0.333f, 131072.0f * 0.25f };

Synth::Synth(void) {
}

Synth::~Synth(void) {
}

void Synth::init() {
    int numberOfVoices[]= { 6, 0, 0, 0};
    for (int t=0; t<NUMBER_OF_TIMBRES; t++) {
        for (int k=0; k<sizeof(struct OneSynthParams)/sizeof(float); k++) {
            ((float*)&timbres[t].params)[k] = ((float*)&preenMainPreset)[k];
        }
        timbres[t].params.engine1.numberOfVoice = numberOfVoices[t];
        timbres[t].init(t);
        for (int v=0; v<MAX_NUMBER_OF_VOICES; v++) {
            timbres[t].initVoicePointer(v, &voices[v]);
        }
    }

    newTimbre(0);
    this->writeCursor = 0;
    this->readCursor = 0;
    for (int k = 0; k < MAX_NUMBER_OF_VOICES; k++) {
        voices[k].init(&timbres[0], &timbres[1], &timbres[2], &timbres[3]);
    }
    rebuidVoiceTimbre();
    refreshNumberOfOsc();
    for (int t=0; t<NUMBER_OF_TIMBRES; t++) {
        timbres[t].numberOfVoicesChanged();
    }
    updateNumberOfActiveTimbres();
}

void Synth::noteOn(int timbre, char note, char velocity) {
    timbres[timbre].noteOn(note, velocity);
}

void Synth::noteOff(int timbre, char note) {
    timbres[timbre].noteOff(note);
}

void Synth::setHoldPedal(int timbre, int value) {
    timbres[timbre].setHoldPedal(value);
}

void Synth::allNoteOff(int timbre) {
    int numberOfVoices = timbres[timbre].params.engine1.numberOfVoice;
    for (int k = 0; k < numberOfVoices; k++) {
        // voice number k of timbre
        int n = timbres[timbre].voiceNumber[k];
        if (voices[n].isPlaying() && !voices[n].isReleased()) {
            voices[n].noteOff();
        }
    }
}

void Synth::allSoundOff(int timbre) {
    int numberOfVoices = timbres[timbre].params.engine1.numberOfVoice;
    for (int k = 0; k < numberOfVoices; k++) {
        // voice number k of timbre
        int n = timbres[timbre].voiceNumber[k];
        voices[n].killNow();
    }
}

void Synth::allSoundOff() {
    for (int k = 0; k < MAX_NUMBER_OF_VOICES; k++) {
        voices[k].killNow();
    }
}

bool Synth::isPlaying() {
    for (int k = 0; k < MAX_NUMBER_OF_VOICES; k++) {
        if (voices[k].isPlaying()) {
            return true;
        }
    }
    return false;
}


void Synth::buildNewSampleBlock() {
    CYCLE_MEASURE_START(cycles_rng);

    // We consider the random number is always ready here...
    uint32_t random32bit = RNG_GetRandomNumber();
    noise[0] = (random32bit & 0xffff) * .000030518f - 1.0f; // value between -1 and 1.
    noise[1] = (random32bit >> 16) * .000030518f - 1.0f; // value between -1 and 1.
    for (int noiseIndex = 2; noiseIndex<32; ) {
        random32bit = 214013 * random32bit + 2531011;
        noise[noiseIndex++] =  (random32bit & 0xffff) * .000030518f - 1.0f; // value between -1 and 1.
        noise[noiseIndex++] = (random32bit >> 16) * .000030518f - 1.0f; // value between -1 and 1.
    }
    CYCLE_MEASURE_END();


    CYCLE_MEASURE_START(cycles_voices1);
    for (int t=0; t<NUMBER_OF_TIMBRES; t++) {
        timbres[t].cleanNextBlock();
        if (likely(timbres[t].params.engine1.numberOfVoice > 0)) {
            timbres[t].prepareForNextBlock();
            // eventually glide
            if (timbres[t].voiceNumber[0] != -1 && this->voices[timbres[t].voiceNumber[0]].isGliding()) {
                this->voices[timbres[t].voiceNumber[0]].glide();
            }
        }
    }
    CYCLE_MEASURE_END();


    CYCLE_MEASURE_START(cycles_voices2);
    // render all voices in their timbre sample block...
    // 16 voices
    if (this->voices[0].isPlaying()) {
        this->voices[0].nextBlock();
    }
    if (this->voices[1].isPlaying()) {
        this->voices[1].nextBlock();
    }
    if (this->voices[2].isPlaying()) {
        this->voices[2].nextBlock();
    }
    if (this->voices[3].isPlaying()) {
        this->voices[3].nextBlock();
    }
    if (this->voices[4].isPlaying()) {
        this->voices[4].nextBlock();
    }
    if (this->voices[5].isPlaying()) {
        this->voices[5].nextBlock();
    }
    if (this->voices[6].isPlaying()) {
        this->voices[6].nextBlock();
    }
    if (this->voices[7].isPlaying()) {
        this->voices[7].nextBlock();
    }
    if (this->voices[8].isPlaying()) {
        this->voices[8].nextBlock();
    }
    if (this->voices[9].isPlaying()) {
        this->voices[9].nextBlock();
    }
    if (this->voices[10].isPlaying()) {
        this->voices[10].nextBlock();
    }
    if (this->voices[11].isPlaying()) {
        this->voices[11].nextBlock();
    }
    if (this->voices[12].isPlaying()) {
        this->voices[12].nextBlock();
    }
    if (this->voices[13].isPlaying()) {
        this->voices[13].nextBlock();
    }
    if (this->voices[14].isPlaying()) {
        this->voices[14].nextBlock();
    }
    if (this->voices[15].isPlaying()) {
        this->voices[15].nextBlock();
    }
    CYCLE_MEASURE_END();


    CYCLE_MEASURE_START(cycles_fx);

    float *mix = mixBuffer;
#if 0
    // PREP: 111 | 240
    asm volatile ( "mov r1, %[dst]" "\n\t"
		   "mov r2, %[val]" "\n\t"
		   "mov r3, %[val]" "\n\t"
		   "mov r4, %[val]" "\n\t"
		   "mov r5, %[val]" "\n\t"
		   "mov r6, %[val]" "\n\t"
		   "mov r7, %[val]" "\n\t"
		   "mov r8, %[val]" "\n\t"
		   "mov r9, %[val]" "\n\t"

		   "stmia r1!, {r2-r9}" "\n\t"
		   "stmia r1!, {r2-r9}" "\n\t"
		   "stmia r1!, {r2-r9}" "\n\t"
		   "stmia r1!, {r2-r9}" "\n\t"
		   "stmia r1!, {r2-r9}" "\n\t"
		   "stmia r1!, {r2-r9}" "\n\t"
		   "stmia r1!, {r2-r9}" "\n\t"
		   "stmia r1!, {r2-r9}" "\n\t"
		   : [dst] "+r" (mix)
		   : [val] "r" (131071.f)
		   : "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9"
		 );
#else
    // PREP: 105 | 230
    for ( unsigned i = 0; i < BLOCK_SIZE/2; ++i ) {
      *mix++ = 131071.f; *mix++ = 131071.f;
      *mix++ = 131071.f; *mix++ = 131071.f;
    }
#endif

    // Add timbre per timbre because gate and eventual other effect are per timbre
    if (likely(timbres[0].params.engine1.numberOfVoice > 0)) {
      timbres[0].fxAfterBlock(ratioTimbre, mixBuffer);
    }
    if (likely(timbres[1].params.engine1.numberOfVoice > 0)) {
      timbres[1].fxAfterBlock(ratioTimbre, mixBuffer);
    }
    if (likely(timbres[2].params.engine1.numberOfVoice > 0)) {
      timbres[2].fxAfterBlock(ratioTimbre, mixBuffer);
    }
    if (likely(timbres[3].params.engine1.numberOfVoice > 0)) {
      timbres[3].fxAfterBlock(ratioTimbre, mixBuffer);
    }
    CYCLE_MEASURE_END();

    CYCLE_MEASURE_START(cycles_timbres);

#define ASM_FINAL_MIX
#ifdef ASM_FINAL_MIX

    // TIM: 292 | 380
    float *src = mixBuffer;
    int *dst = &samples[writeCursor];
    unsigned count = BLOCK_SIZE * 2; // stereo samples
    asm volatile( "\n\t" 
		  "mov r1, %[src]" "\n\t"
		  "mov r2, %[dst]" "\n\t"
		  "mov r3, %[count]" "\n\t"
		  "1:" "\n\t"
		  "vldmia.32 r1!, {s1-s8}" "\n\t"
		  "vcvt.s32.f32 s9, s1" "\n\t"
		  "vcvt.s32.f32 s10, s2" "\n\t"
		  "vcvt.s32.f32 s11, s3" "\n\t"
		  "vcvt.s32.f32 s12, s4" "\n\t"
		  "vcvt.s32.f32 s13, s5" "\n\t"
		  "vcvt.s32.f32 s14, s6" "\n\t"
		  "vcvt.s32.f32 s15, s7" "\n\t"
		  "vcvt.s32.f32 s16, s8" "\n\t"
		  "vstmia.32 r2!, {s9-s16}" "\n\t"
		  "subs r3, #8" "\n\t"
		  "bhi 1b" "\n\t"
		  : [src] "+r" (src), [dst] "+r" (dst)
		  : [count] "r" (count)
		  : "r1", "r2", "r3", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15", "s16"
		  );

    /*
    // TIM: 328 | 420
    asm volatile( "\n\t" 
		  "mov r1, %[src]" "\n\t"
		  "mov r2, %[dst]" "\n\t"
		  "mov r3, %[count]" "\n\t"
		  "1:" "\n\t"
		  "vldr s1, [r1, #0]" "\n\t"
		  "vldr s2, [r1, #4]" "\n\t"
		  "vldr s3, [r1, #8]" "\n\t"
		  "vldr s4, [r1, #12]" "\n\t"

		  "vcvt.s32.f32 s9, s1" "\n\t"
		  "vcvt.s32.f32 s10, s2" "\n\t"
		  "vcvt.s32.f32 s11, s3" "\n\t"
		  "vcvt.s32.f32 s12, s4" "\n\t"

		  "vstr s9, [r2, #0]" "\n\t"
		  "vstr s10, [r2, #4]" "\n\t"
		  "vstr s11, [r2, #8]" "\n\t"
		  "vstr s12, [r2, #12]" "\n\t"

		  "vldr s5, [r1, #16]" "\n\t"
		  "vldr s6, [r1, #20]" "\n\t"
		  "vldr s7, [r1, #24]" "\n\t"
		  "vldr s8, [r1, #28]" "\n\t"

		  "vcvt.s32.f32 s13, s5" "\n\t"
		  "vcvt.s32.f32 s14, s6" "\n\t"
		  "vcvt.s32.f32 s15, s7" "\n\t"
		  "vcvt.s32.f32 s16, s8" "\n\t"

		  "vstr s13, [r2, #16]" "\n\t"
		  "vstr s14, [r2, #20]" "\n\t"
		  "vstr s15, [r2, #24]" "\n\t"
		  "vstr s16, [r2, #28]" "\n\t"

		  "adds r1, #32" "\n\t"
		  "adds r2, #32" "\n\t"
		  "subs r3, #8" "\n\t"
		  "bhi 1b" "\n\t"
		  : [src] "+r" (src), [dst] "+r" (dst)
		  : [count] "r" (count)
		  : "r1", "r2", "r3", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15", "s16"
		  );
    */

#else
    // TIM: 580 | 600

#define COPY_FOUR_SAMPLES()			\
    *cb++ = (int)( *src++ );			\
    *cb++ = (int)( *src++ );			\
    *cb++ = (int)( *src++ );			\
    *cb++ = (int)( *src++ )

    int *cb = &samples[writeCursor];
    const float *src = mixBuffer;
    for ( unsigned i = 0; i < BLOCK_SIZE/16; i++ ) {
      COPY_FOUR_SAMPLES();      COPY_FOUR_SAMPLES();
      COPY_FOUR_SAMPLES();      COPY_FOUR_SAMPLES();
      COPY_FOUR_SAMPLES();      COPY_FOUR_SAMPLES();
      COPY_FOUR_SAMPLES();      COPY_FOUR_SAMPLES();
    }
#endif
    CYCLE_MEASURE_END();

    writeCursor = (writeCursor + 64) & 255;
}

void Synth::beforeNewParamsLoad(int timbre) {
    for (int t=0; t<NUMBER_OF_TIMBRES; t++) {
        timbres[t].resetArpeggiator();
        for (int v=0; v<MAX_NUMBER_OF_VOICES; v++) {
            timbres[t].setVoiceNumber(v, -1);
        }
    }
    // Stop all voices
    allSoundOff();
};

void Synth::afterNewParamsLoad(int timbre) {
    timbres[timbre].afterNewParamsLoad();
    // Reset to 0 the number of voice then try to set the right value
    int numberOfVoice = timbres[timbre].params.engine1.numberOfVoice;
    timbres[timbre].params.engine1.numberOfVoice = 0;


    // refresh a first time with numberofvoice = 0
    refreshNumberOfOsc();

    int freeOsc = MAX_NUMBER_OF_OPERATORS - this->numberOfOsc;
    float voicesMax = (float)freeOsc / (float)algoInformation[(int)timbres[timbre].params.engine1.algo].osc ;

    if (numberOfVoice > voicesMax) {
        timbres[timbre].params.engine1.numberOfVoice = (int)voicesMax;
    } else {
        timbres[timbre].params.engine1.numberOfVoice = numberOfVoice;
    }
    // Refresh again so that the value is up to date
    refreshNumberOfOsc();
    rebuidVoiceTimbre();
    updateNumberOfActiveTimbres();
    timbres[timbre].numberOfVoicesChanged();
}

void Synth::afterNewComboLoad() {
    for (int t=0; t<NUMBER_OF_TIMBRES ; t++) {
        timbres[t].afterNewParamsLoad();
    }
    rebuidVoiceTimbre();
    refreshNumberOfOsc();
    for (int t=0; t<NUMBER_OF_TIMBRES ; t++) {
        timbres[t].numberOfVoicesChanged();
    }
    updateNumberOfActiveTimbres();
}

void Synth::updateNumberOfActiveTimbres() {
    int activeTimbres = 0;
    if (timbres[0].params.engine1.numberOfVoice > 0) {
        activeTimbres ++;
    }
    if (timbres[1].params.engine1.numberOfVoice > 0) {
        activeTimbres ++;
    }
    if (timbres[2].params.engine1.numberOfVoice > 0) {
        activeTimbres ++;
    }
    if (timbres[3].params.engine1.numberOfVoice > 0) {
        activeTimbres ++;
    }
    ratioTimbre = ratiosTimbre[activeTimbres];
}

int Synth::getFreeVoice() {
    // Loop on all voices
    for (int voice=0; voice< MAX_NUMBER_OF_VOICES; voice++) {
        bool used = false;

        for (int t=0; t< NUMBER_OF_TIMBRES && !used; t++) {
            // Must be different from 0 and -1
            int interVoice = -10;
            for (int v=0;  v < MAX_NUMBER_OF_VOICES  && !used; v++) {
                interVoice = timbres[t].voiceNumber[v];
                if (interVoice == -1) {
                    break;
                }
                if (interVoice == voice) {
                    used = true;
                }
            }
        }

        if (!used) {
            return voice;
        }
    }
    return -1;
}

// can prevent some value change...
void Synth::checkNewParamValue(int timbre, int currentRow, int encoder, float oldValue, float *newValue) {
    if (unlikely(currentRow == ROW_ENGINE)) {

        int freeOsc = MAX_NUMBER_OF_OPERATORS - numberOfOsc;
        if (unlikely(encoder == ENCODER_ENGINE_ALGO)) {
            // If one voice exactly and not enough free osc to change algo
            // If more than 1 voice, it's OK, the number of voice will be reduced later.
            if (timbres[timbre].params.engine1.numberOfVoice < 1.5f && timbres[timbre].params.engine1.numberOfVoice > 0.5f
                    &&  freeOsc < (algoInformation[(int)(*newValue)].osc - algoInformation[(int)oldValue].osc)) {
                // not enough free osc
                *newValue = oldValue;
            }
        }
        if (unlikely(encoder == ENCODER_ENGINE_VOICE)) {

            // Increase number of voice ?
            if ((*newValue)> oldValue) {
                if (freeOsc < ((*newValue) - oldValue) * algoInformation[(int)timbres[timbre].params.engine1.algo].osc) {
                    *newValue = oldValue;
                }
            }
        }
    }
}

void Synth::newParamValue(int timbre, int currentRow, int encoder, ParameterDisplay* param, float oldValue, float newValue) {
    switch (currentRow) {
    case ROW_ENGINE:
        switch (encoder) {
        case ENCODER_ENGINE_ALGO:
            refreshNumberOfOsc();
            fixMaxNumberOfVoices(timbre);
            break;
        case ENCODER_ENGINE_VOICE:
            if (newValue > oldValue) {
                for (int v=(int)oldValue; v < (int)newValue; v++) {
                    timbres[timbre].setVoiceNumber(v, getFreeVoice());
                }
                refreshNumberOfOsc();
            } else {
                for (int v=(int)newValue; v < (int)oldValue; v++) {
                    voices[timbres[timbre].voiceNumber[v]].killNow();
                    timbres[timbre].setVoiceNumber(v, -1);
                }
                refreshNumberOfOsc();
            }
            timbres[timbre].numberOfVoicesChanged();
            if (newValue == 0.0f || oldValue == 0.0f) {
                updateNumberOfActiveTimbres();
            }
            break;
        }
        break;
        case ROW_ARPEGGIATOR1:
            switch (encoder) {
            case ENCODER_ARPEGGIATOR_CLOCK:
                allNoteOff(timbre);
                timbres[timbre].setArpeggiatorClock((uint8_t) newValue);
                break;
            case ENCODER_ARPEGGIATOR_BPM:
                timbres[timbre].setNewBPMValue((uint8_t) newValue);
                break;
            case ENCODER_ARPEGGIATOR_DIRECTION:
                timbres[timbre].setDirection((uint8_t) newValue);
                break;
            }
            break;
            case ROW_ARPEGGIATOR2:
                if (unlikely(encoder == ENCODER_ARPEGGIATOR_LATCH)) {
                    timbres[timbre].setLatchMode((uint8_t) newValue);
                }
                break;
            case ROW_ARPEGGIATOR3:
                break;
            case ROW_EFFECT:
                timbres[timbre].setNewEffecParam(encoder);
                break;
            case ROW_ENV1a:
                timbres[timbre].env1.reloadADSR(encoder);
                break;
            case ROW_ENV1b:
                timbres[timbre].env1.reloadADSR(encoder + 4);
                break;
            case ROW_ENV2a:
                timbres[timbre].env2.reloadADSR(encoder);
                break;
            case ROW_ENV2b:
                timbres[timbre].env2.reloadADSR(encoder + 4);
                break;
            case ROW_ENV3a:
                timbres[timbre].env3.reloadADSR(encoder);
                break;
            case ROW_ENV3b:
                timbres[timbre].env3.reloadADSR(encoder + 4);
                break;
            case ROW_ENV4a:
                timbres[timbre].env4.reloadADSR(encoder);
                break;
            case ROW_ENV4b:
                timbres[timbre].env4.reloadADSR(encoder + 4);
                break;
            case ROW_ENV5a:
                timbres[timbre].env5.reloadADSR(encoder);
                break;
            case ROW_ENV5b:
                timbres[timbre].env5.reloadADSR(encoder + 4);
                break;
            case ROW_ENV6a:
                timbres[timbre].env6.reloadADSR(encoder);
                break;
            case ROW_ENV6b:
                timbres[timbre].env6.reloadADSR(encoder + 4);
                break;
            case ROW_MATRIX_FIRST ... ROW_MATRIX_LAST:
            if (encoder == ENCODER_MATRIX_DEST) {
                // Reset old destination
                timbres[timbre].matrix.resetDestination(oldValue);
            }
            break;
            case ROW_LFO_FIRST ... ROW_LFO_LAST:
            // timbres[timbre].lfo[currentRow - ROW_LFOOSC1]->valueChanged(encoder);
            timbres[timbre].lfoValueChange(currentRow, encoder, newValue);
            break;
            case ROW_PERFORMANCE1:
                timbres[timbre].matrix.setSource((enum SourceEnum)(MATRIX_SOURCE_CC1 + encoder), newValue);
                break;
    }
}


// synth is the only one who knows timbres
void Synth::newTimbre(int timbre)  {
    this->synthState->setParamsAndTimbre(&timbres[timbre].params, timbre, timbres[timbre].getPerformanceValuesAddress());
}


/*
 * Return false if had to change number of voice
 *
 */
bool Synth::fixMaxNumberOfVoices(int timbre) {
    int freeOsc = MAX_NUMBER_OF_OPERATORS - this->numberOfOsc;

    int voicesMax = (float)timbres[timbre].params.engine1.numberOfVoice + .001f + (float)freeOsc / (float)algoInformation[(int)timbres[timbre].params.engine1.algo].osc ;

    if (this->timbres[timbre].params.engine1.numberOfVoice > voicesMax) {
        int oldValue = this->timbres[timbre].params.engine1.numberOfVoice;
        this->timbres[timbre].params.engine1.numberOfVoice = voicesMax;
        ParameterDisplay *params = &allParameterRows.row[ROW_ENGINE]->params[ENCODER_ENGINE_VOICE];
        synthState->propagateNewParamValue(timbre, ROW_ENGINE, ENCODER_ENGINE_VOICE, params, oldValue, voicesMax );
        return false;
    }

    return true;
}



void Synth::rebuidVoiceTimbre() {
    int voices = 0;
    int activeTimbre = 0;

    for (int t=0; t<NUMBER_OF_TIMBRES; t++) {
        int nv = timbres[t].params.engine1.numberOfVoice;

        for (int v=0; v < nv; v++) {
            timbres[t].setVoiceNumber(v, voices++);
        }
        for (int v=nv; v < MAX_NUMBER_OF_VOICES;  v++) {
            timbres[t].setVoiceNumber(v, -1);
        }
    }
}

void Synth::refreshNumberOfOsc() {
    this->numberOfOsc = 0;

    for (int t=0; t<NUMBER_OF_TIMBRES; t++) {
        int nv = timbres[t].params.engine1.numberOfVoice + .0001f;
        this->numberOfOsc += algoInformation[(int)timbres[t].params.engine1.algo].osc * nv;
    }
}

void Synth::loadPreenFMPatchFromMidi(int timbre, int bank, int bankLSB, int patchNumber) {
    this->synthState->loadPreenFMPatchFromMidi(timbre, bank, bankLSB, patchNumber, &timbres[timbre].params);
}


void Synth::setNewValueFromMidi(int timbre, int row, int encoder, float newValue) {
    struct ParameterDisplay* param = &(allParameterRows.row[row]->params[encoder]);
    int index = row * NUMBER_OF_ENCODERS + encoder;
    float oldValue = ((float*)this->timbres[timbre].getParamRaw())[index];
    this->timbres[timbre].setNewValue(index, param, newValue);
    this->synthState->propagateNewParamValueFromExternal(timbre, row, encoder, param, oldValue, newValue);
}


#ifdef DEBUG

// ========================== DEBUG ========================
void Synth::debugVoice() {

    lcd.setRealTimeAction(true);
    lcd.clearActions();
    lcd.clear();
    int numberOfVoices = timbres[0].params.engine1.numberOfVoice;
    // HARDFAULT !!! :-)
    //    for (int k = 0; k <10000; k++) {
    //    	numberOfVoices += timbres[k].params.engine1.numberOfVoice;
    //    	timbres[k].params.engine1.numberOfVoice = 100;
    //    }

    for (int k = 0; k < numberOfVoices && k < 4; k++) {

        // voice number k of timbre
        int n = timbres[0].voiceNumber[k];

        lcd.setCursor(0, k);
        lcd.print((int)voices[n].getNote());

        lcd.setCursor(4, k);
        lcd.print((int)voices[n].getNextPendingNote());

        lcd.setCursor(18, k);
        lcd.print((int)voices[n].getIndex());

        lcd.setCursor(18, k);
        lcd.print((int) voices[n].isReleased());
        lcd.print((int) voices[n].isPlaying());
    }
    lcd.setRealTimeAction(false);
}

void Synth::showCycles() {
    lcd.setRealTimeAction(true);
    lcd.clearActions();
    lcd.clear();

    lcd.setCursor( 0, 0 );
    lcd.print( "RNG: " );
    lcd.print( cycles_rng.remove() );

    lcd.setCursor( 0, 1 );
    lcd.print( "VOI: " );  lcd.print( cycles_voices1.remove() );
    lcd.print( " " ); lcd.print( cycles_voices2.remove() );

    lcd.setCursor( 0, 2 );
    lcd.print( "FX : " );
    lcd.print( cycles_fx.remove() );

    lcd.setCursor( 0, 3 );
    lcd.print( "TIM: " );
    lcd.print( cycles_timbres.remove() );

    lcd.setRealTimeAction(false);
}

#endif
