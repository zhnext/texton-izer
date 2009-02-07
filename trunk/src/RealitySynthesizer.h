#ifndef __H_REALITY_SYNTHESIZER_H__
#define __H_REALITY_SYNTHESIZER_H__

#include "defs.h"

#include "Cluster.h"
#include "Synthesizer.h"

#include <vector>

using std::vector;

class RealitySynthesizer : public Synthesizer
{
public:
	RealitySynthesizer(int nWindow, int MaxDiff);
	virtual ~RealitySynthesizer();

	IplImage* synthesize(int nNewWidth, int nNewHeight, int depth, 
		int nChannels, vector<Cluster> &clusterList,int * pTextonMap, int nMapWidth, int nMapHeight);

private:

	bool checkMapSpace(int x, int y, int nCluster, int *scaledTextonMap, IplImage* img);

	int * scaleTextonMap(int * pTextonMap, int nWidth, int nHeight, int nScaledWidth, int nScaledHeight);

	void removeFromMap(int x, int y, Texton *t, int nWidth, int nHeight, int*scaledTextonMap);


	void printTextonMap(int nMapWidth, int nMapHeight, int *scaledTextonMap);

private:
	int m_nWindow;
	int m_nMaxDiff;

};



#endif	//__H_REALITY_SYNTHESIZER_H__
