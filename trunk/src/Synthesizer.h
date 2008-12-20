#ifndef __H_SYNTHESIZER_H__
#define __H_SYNTHESIZER_H__

#include <vector>
#include "Texton.h"

using std::vector;
class Synthesizer
{
public:
	Synthesizer();
	virtual ~Synthesizer();

	void synthesize(int nNewWidth, int nNewHeight, int depth, int nChannels, std::vector<Texton*> &textonList);

};

#endif	//__H_SYNTHESIZER_H__