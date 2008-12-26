#ifndef __H_SYNTHESIZER_H__
#define __H_SYNTHESIZER_H__

#include <vector>
#include "Cluster.h"

using std::vector;
class Synthesizer
{
public:
	Synthesizer();
	virtual ~Synthesizer();

	IplImage* synthesize(int nNewWidth, int nNewHeight, int depth, int nChannels, vector<Cluster> &textonList);

private:
	bool insertTexton(int x, int y, const IplImage * textonImg, IplImage* synthesizedImage);

	void applyCoOccurence(int x, int y, vector<CoOccurences>* co, vector<Cluster> &clusterList, IplImage* synthesizedImage);

	void CopyImageWithoutBorder(IplImage * src,IplImage * dst, int nBorderSize);

private:

	CvScalar m_textonBgColor;
	int		m_nBorder;
};

class CoOccurenceQueueItem
{
public:
	CoOccurenceQueueItem(int x, int y, vector<CoOccurences>* co){
		m_x = x;
		m_y = y;
		m_co = co;
	}
	vector<CoOccurences>* m_co;
	int	m_x;
	int m_y;
};

#endif	//__H_SYNTHESIZER_H__