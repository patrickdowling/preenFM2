#include <math.h>
#include "utils/Macros.h"
#include "TimbreFX.h"
#include "Voice.h"

/**
 * Assuming -1.f < pan < 1.f, calculate coefficients to calculate
 * l' = pan_ll * l + pan_lr * r
 * r' = pan_rl * l + pan_rr * r
 */
static inline void __calcPanCoefficients( float pan, float &pan_ll, float &pan_lr, float &pan_rl, float &pan_rr )
{
  if ( pan < 0.0f ) {
    pan_ll = 1.f;
    pan_lr = -pan;
    pan_rl = 0.f;
    pan_rr = (1.f + pan);
  } else {
    pan_ll = (1.f - pan);
    pan_lr = 0.f;
    pan_rl = pan;
    pan_rr = 1.f;
  }
}

#define ASMCLIP(reg)					\
  "vcmp.f32 "STRINGIFY(reg)", %[clip]" "\n\t"		\
    "vmrs APSR_nzcv, fpscr" "\n\t"			\
    "it gt" "\n\t"					\
    "vmovgt.f32 "STRINGIFY(reg)", %[clip]" "\n\t"	\
    "vcmp.f32 "STRINGIFY(reg)", %[clipn]" "\n\t"	\
    "vmrs APSR_nzcv, fpscr" "\n\t"			\
    "it lt" "\n\t"					\
    "vmovlt.f32 "STRINGIFY(reg)", %[clipn]" "\n\t"	\
    ""

/**
 * Sample buffer processing
 */
//template <bool _enablePan, bool _enableClip>
void __fxProcessBuffer( const struct sample_buffer_fx *buffer, int _enablePan, int _enableClip )
{
  const float *src  = buffer->src;
  float *dst = buffer->dst;
  unsigned count = buffer->count;

  float gateCoef = buffer->gateCoef;
  float gateStep = buffer->gateStep;
  float gain = buffer->mixerGain;
  float clip = buffer->clip;
  float clipn = -buffer->clip;
  //int doclip = _enableClip;
  //int dopan = _enablePan;
  float pan_ll, pan_lr, pan_rl, pan_rr;
  if ( _enablePan )
    __calcPanCoefficients( buffer->pan, pan_ll, pan_lr, pan_rl, pan_rr );

  asm volatile ( "\n\t"
		 "0:" "\n\t"
		 "vldmia %[src]!, {s8-s11}" "\n\t"
		 
		 "vmul.f32 s8, s8, %[gate]" "\n\t" // sample = src * gate
		 "vmul.f32 s9, s9, %[gate]" "\n\t"
		 "vmul.f32 s10, s10, %[gate]" "\n\t"
		 "vmul.f32 s11, s11, %[gate]" "\n\t"

		 "cbz %[dopan], 1f" "\n\t"
		 // NOTE TO SELF: This looks like a case for vmla, but that turns out slower...
		 "vmul.f32 s20, s8, %[pan_ll]" "\n\t" // l * pan_ll
		 "vmul.f32 s21, s9, %[pan_lr]" "\n\t" // r * pan_lr
		 "vmul.f32 s22, s8, %[pan_rl]" "\n\t" // l * pan_rl
		 "vmul.f32 s23, s9, %[pan_rr]" "\n\t" // r * pan_rr

		 "vmul.f32 s20, s10, %[pan_ll]" "\n\t" // l * pan_ll
		 "vmul.f32 s21, s11, %[pan_lr]" "\n\t" // r * pan_lr
		 "vmul.f32 s22, s10, %[pan_rl]" "\n\t" // l * pan_rl
		 "vmul.f32 s23, s11, %[pan_rr]" "\n\t" // r * pan_rr

		 "vadd.f32 s8, s20, s21" "\n\t" // l = l * pan_ll + r * pan_lr
		 "vadd.f32 s9, s22, s23" "\n\t" // r = l * pan_rl + r * pan_rr
		 "vadd.f32 s10, s20, s21" "\n\t" // l = l * pan_ll + r * pan_lr
		 "vadd.f32 s11, s22, s23" "\n\t" // r = l * pan_rl + r * pan_rr

		 "1:" "\n\t"

		 "vldmia %[dst], {s12-s15}" "\n\t" // load destination values

		 "vmul.f32 s8, s8, %[gain]" "\n\t" // sample = sample * gain
		 "vmul.f32 s9, s9, %[gain]" "\n\t"
		 "vmul.f32 s10, s10, %[gain]" "\n\t"
		 "vmul.f32 s11, s11, %[gain]" "\n\t"

		 "cbz %[doclip], 2f" "\n\t"
		 ASMCLIP(s8)
		 ASMCLIP(s9)
		 ASMCLIP(s10)
		 ASMCLIP(s11)
		 "2:" "\n\t"

		 "vadd.f32 s12, s12, s8" "\n\t"
		 "vadd.f32 s13, s13, s9" "\n\t"
		 "vadd.f32 s14, s14, s10" "\n\t"
		 "vadd.f32 s15, s15, s11" "\n\t"

		 "vstmia %[dst]!, {s12-s15}" "\n\t"

		 "vadd.f32 %[gate], %[gate], %[step]" "\n\t"
		 "subs %[count], #4" "\n\t"
		 "bhi 0b" "\n\t"

		 : [src] "+r" (src), [dst] "+r" (dst), [gate] "+w" (gateCoef), [count] "+r" (count)
		 : [step] "w" (gateStep),
		   [gain] "w" (gain),
		   [clip] "w" (clip), [clipn] "w" (clipn), [doclip] "r" (_enableClip),
		   [pan_ll] "w" (pan_ll), [pan_lr] "w" (pan_lr), [pan_rl] "w" (pan_rl), [pan_rr] "w" (pan_rr), [dopan] "r" (_enablePan)
		 : "s8", "s9", "s10", "s11", // source data
		   "s12", "s13", "s14", "s15", // destination data
		   "s20", "s21", "s22", "s23" // pan calculation results
	       );
}

void __fxProcessBufferLP( const struct sample_buffer_fx *buffer, int _enablePan, int _enableClip, struct filter_values *_filter )
{
  const float *src  = buffer->src;
  float *dst = buffer->dst;
  unsigned count = buffer->count;

  float gateCoef = buffer->gateCoef;
  float gateStep = buffer->gateStep;
  float gain = buffer->mixerGain;
  float clip = buffer->clip;
  float clipn = -buffer->clip;
  //int doclip = _enableClip;
  //int dopan = _enablePan;
  float pan_ll, pan_lr, pan_rl, pan_rr;
  if ( _enablePan )
    __calcPanCoefficients( buffer->pan, pan_ll, pan_lr, pan_rl, pan_rr );

  float fxParam1 = _filter->fxParam1;
  float pattern = _filter->pattern;
  float *values = _filter->v;

  asm volatile ( "\n\t"
		 "0:" "\n\t"
		 "vldmia %[src]!, {s8-s11}" "\n\t"
		 
		 "vmul.f32 s8, s8, %[gate]" "\n\t" // sample = src * gate
		 "vmul.f32 s9, s9, %[gate]" "\n\t"
		 "vmul.f32 s10, s10, %[gate]" "\n\t"
		 "vmul.f32 s11, s11, %[gate]" "\n\t"

		 "vldmia %[values], {s20-s23}" "\n\t" // get localvxx values v0l, v1l, v0r, v1r

		 "vmul.f32 s20, %[pattern], s20" "\n\t" // L1: localv0x = pattern * localv0x
		 "vmul.f32 s22, %[pattern], s22" "\n\t" // R1: localv0x = pattern * localv0x;
		 "vmls.f32 s20, %[fxParam1], s21" "\n\t" // L1: localv0x -= fxParam1 * localv1x
		 "vmls.f32 s22, %[fxParam1], s23" "\n\t" // R1: localv0x -= fxParam1 * localv1x
		 "vmla.f32 s20, %[fxParam1], s8" "\n\t"  // L1: localv0x += fxParam1 * sample
		 "vmla.f32 s22, %[fxParam1], s9" "\n\t"  // R1: localv0x += fxParam1 * sample
		 "vmul.f32 s21, %[pattern], s21" "\n\t" // L1: localv1x = pattern * localv1x
		 "vmul.f32 s23, %[pattern], s23" "\n\t" // R1: localv1x = pattern * localv1x
		 "vmla.f32 s21, %[fxParam1], s20" "\n\t" // L1: localv1x += fxParam1 * localv0x
		 "vmla.f32 s23, %[fxParam1], s22" "\n\t" // R1: localv1x += fxParam1 * localv0x
		 "vmov s8, s21" "\n\t" // L1: sample = localv1x
		 "vmov s9, s23" "\n\t" // R1: sample = localv1x

		 "vmul.f32 s20, %[pattern], s20" "\n\t" // L2: localv0x = pattern * localv0x;
		 "vmul.f32 s22, %[pattern], s22" "\n\t" // R2: localv0x = pattern * localv0x;
		 "vmls.f32 s20, %[fxParam1], s21" "\n\t" // L2: localv0x -= fxParam1 * localv1x
		 "vmls.f32 s22, %[fxParam1], s23" "\n\t" // R2: localv0x -= fxParam1 * localv1x
		 "vmla.f32 s20, %[fxParam1], s10" "\n\t"  // L2: localv0x += fxParam1 * sample
		 "vmla.f32 s22, %[fxParam1], s11" "\n\t"  // R2: localv0x += fxParam1 * sample
		 "vmul.f32 s21, %[pattern], s21" "\n\t" // L2: localv1x = pattern * localv1x
		 "vmul.f32 s23, %[pattern], s23" "\n\t" // R2: localv1x = pattern * localv1x
		 "vmla.f32 s21, %[fxParam1], s20" "\n\t" // L2: localv1x += fxParam1 * localv0x
		 "vmla.f32 s23, %[fxParam1], s22" "\n\t" // R2: localv1x += fxParam1 * localv0x
		 "vmov s10, s21" "\n\t" // L2: sample = localv1x
		 "vmov s11, s23" "\n\t" // R2: sample = localv1x

		 "vstmia %[values], {s20-s23}" "\n\t"

		 "cbz %[dopan], 1f" "\n\t"
		 // NOTE TO SELF: This looks like a case for vmla, but that turns out slower...
		 "vmul.f32 s20, s8, %[pan_ll]" "\n\t" // l * pan_ll
		 "vmul.f32 s21, s9, %[pan_lr]" "\n\t" // r * pan_lr
		 "vmul.f32 s22, s8, %[pan_rl]" "\n\t" // l * pan_rl
		 "vmul.f32 s23, s9, %[pan_rr]" "\n\t" // r * pan_rr

		 "vmul.f32 s20, s10, %[pan_ll]" "\n\t" // l * pan_ll
		 "vmul.f32 s21, s11, %[pan_lr]" "\n\t" // r * pan_lr
		 "vmul.f32 s22, s10, %[pan_rl]" "\n\t" // l * pan_rl
		 "vmul.f32 s23, s11, %[pan_rr]" "\n\t" // r * pan_rr

		 "vadd.f32 s8, s20, s21" "\n\t" // l = l * pan_ll + r * pan_lr
		 "vadd.f32 s9, s22, s23" "\n\t" // r = l * pan_rl + r * pan_rr
		 "vadd.f32 s10, s20, s21" "\n\t" // l = l * pan_ll + r * pan_lr
		 "vadd.f32 s11, s22, s23" "\n\t" // r = l * pan_rl + r * pan_rr

		 "1:" "\n\t"

		 "vldmia %[dst], {s12-s15}" "\n\t" // load destination values

		 "vmul.f32 s8, s8, %[gain]" "\n\t" // sample = sample * gain
		 "vmul.f32 s9, s9, %[gain]" "\n\t"
		 "vmul.f32 s10, s10, %[gain]" "\n\t"
		 "vmul.f32 s11, s11, %[gain]" "\n\t"

		 "cbz %[doclip], 2f" "\n\t"
		 ASMCLIP(s8)
		 ASMCLIP(s9)
		 ASMCLIP(s10)
		 ASMCLIP(s11)
		 "2:" "\n\t"

		 "vadd.f32 s12, s12, s8" "\n\t"
		 "vadd.f32 s13, s13, s9" "\n\t"
		 "vadd.f32 s14, s14, s10" "\n\t"
		 "vadd.f32 s15, s15, s11" "\n\t"

		 "vstmia %[dst]!, {s12-s15}" "\n\t"

		 "vadd.f32 %[gate], %[gate], %[step]" "\n\t"
		 "subs %[count], #4" "\n\t"
		 "bhi 0b" "\n\t"

		 : [src] "+r" (src), [dst] "+r" (dst), [gate] "+w" (gateCoef), [count] "+r" (count), [values] "+r" (values)
		 : [step] "w" (gateStep),
		   [gain] "w" (gain),
		   [clip] "w" (clip), [clipn] "w" (clipn), [doclip] "r" (_enableClip),
		   [pan_ll] "w" (pan_ll), [pan_lr] "w" (pan_lr), [pan_rl] "w" (pan_rl), [pan_rr] "w" (pan_rr), [dopan] "r" (_enablePan),
		   [pattern] "w" (pattern), [fxParam1] "w" (fxParam1)
		 : "s8", "s9", "s10", "s11", // source data
		   "s12", "s13", "s14", "s15", // destination data
		   "s20", "s21", "s22", "s23" // (tmp) effect calculation results
	       );
}
