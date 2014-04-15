#ifndef TIMBREFX_H_
#define TIMBREFX_H_

struct sample_buffer_fx {
  const float *src;		// source buffer
  float *dst;			// destination buffer
  unsigned count;		// total number of samples

  float gateCoef;		// starting gate coefficient value
  float gateStep;		// gate step
  float mixerGain;		// mixer gain
  float clip;			// positive clip value
  float pan;			// panning
};

struct filter_values {
  float pattern;
  float fxParam1, fxParam2;

  float v[4]; // v0l, v1l, v0r, v1r

};

void __fxProcessBuffer( const struct sample_buffer_fx *buffer, int _enablePan, int _enableClip );

void __fxProcessBufferLP( const struct sample_buffer_fx *buffer, int _enablePan, int _enableClip, struct filter_values *_filter );

#endif /* TIMBREFX_H_ */
