#ifndef __H_SYNTHESIZER_H__
#define __H_SYNTHESIZER_H__

#include <vector>
#include <cv.h>
#include <time.h>
#include <list>
#include "Cluster.h"

using std::vector;
using std::list;

class Synthesizer
{
public:
	Synthesizer();
	virtual ~Synthesizer();

	IplImage* synthesize(int nNewWidth, int nNewHeight, int depth, 
						int nChannels, vector<Cluster> &textonList);

private:
	bool insertTexton(int x, int y, const IplImage * textonImg, IplImage* synthesizedImage);

	void applyCoOccurence(int x, int y, vector<CoOccurences>* co, 
							vector<Cluster> &clusterList, 
							IplImage* synthesizedImage);

	void copyImageWithoutBorder(IplImage * src,IplImage * dst, int nBorderSize);
	void copyImageWithoutBackground(IplImage * src,IplImage * dst);


	bool checkSurrounding(int x, int y, Texton* t, IplImage* synthesizedImage);

	IplImage * retrieveBackground(vector<Cluster> &clusterList, IplImage * img);

	void removeUnconformingTextons(vector<Cluster> &clusterList);
	void removeBorderTextons(vector<Cluster>& clusterList);

private:

	CvScalar m_textonBgColor;
	CvScalar m_resultBgColor;
	int		 m_nBorder;
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