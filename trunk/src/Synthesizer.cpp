#include "Synthesizer.h"

Synthesizer::Synthesizer()
{}

Synthesizer::~Synthesizer()
{

}

void Synthesizer::synthesize(int nNewWidth, int nNewHeight, int depth, int nChannels, std::vector<Texton*> &textonList)
{
	IplImage * synthesizedImage = cvCreateImage(cvSize(nNewWidth,nNewHeight), depth, nChannels);
}