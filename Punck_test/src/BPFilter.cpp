#include <BPFilter.h>

void BPFilter::SetCutoff(const float cutoff, const float Q)
{
	if (cutoff != currentCutoff || Q != currentQ) {
		currentCutoff = cutoff;
		currentQ = Q;
/*
		// Convert cutoff frequency to omega
		const float w0 = (2.0f * pi * cutoff) / systemSampleRate;
		const float alpha = sin(w0) / (2 * Q);
		iirCoeff.b0 = alpha;
		iirCoeff.b1 = 0.0f;
		iirCoeff.b2 = -iirCoeff.b0;
		iirCoeff.a0 = 1 + alpha;
		iirCoeff.a1 = -2 * cos(w0);
		iirCoeff.a2 = 1 - alpha;
	*/


		// Normalised to avoid division by a0 in filter calculation
		const float w0 = (2.0f * pi * cutoff) / systemSampleRate;
		const float n = (2.0f * Q) / (2.0f * Q + sin(w0));
		iirCoeff.b0 = 1.0f - n;
		iirCoeff.b1 = 0.0f;
		iirCoeff.b2 = n - 1.0f;;
		iirCoeff.a0 = 1.0f;
		iirCoeff.a1 = -2.0f * n * cos(w0);
		iirCoeff.a2 = 2.0f * n - 1.0f;;


	}
}


float BPFilter::CalcFilter(const float xn0)
{
	float yn0 = ((iirCoeff.b0 * xn0) + (iirCoeff.b1 * xn1) + (iirCoeff.b2 * xn2) - (iirCoeff.a1 * yn1) - (iirCoeff.a2 * yn2));// / iirCoeff.a0;
	xn2 = xn1;
	xn1 = xn0;
	yn2 = yn1;
	yn1 = yn0;

	return yn0;
}


void BPFilter::Init()
{
	xn2 = 0.0f;
	xn1 = 0.0f;
	yn2 = 0.0f;
	yn1 = 0.0f;
}
