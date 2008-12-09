#ifndef _H_SEGMENTATOR_H_
#define _H_SEGMENTATOR_H_

#include <cv.h>
#include <highgui.h>

#include "fe/FeatureExtraction.h"

#define FIRST_TEXTON_NUM	5
#define UNCLUSTERED_DATA	0
#define OUT_OF_SEGMENT_DATA	4
#define BORDER_DATA			1

#define EDGE_DATA			255

class Textonator
{
public:
	Textonator(IplImage * Img, int nClusters, int nMinTextonSize);
	virtual ~Textonator();

	void Textonize();

private:
	
	void ColorWindow(int x, int y, int sizeX, int sizeY);
	void SaveImage(char *filename,IplImage* img);
	void RecolorPixel(uchar * pData, int y, int x, int step, CvScalar * pColor);

	void Segment();
	void Cluster(CFeatureExtraction *pFeatureExtractor);

    void colorCluster(int nCluster);
	void blurImage();
	void cannyEdgeDetect();

	void extractTexton(int minX, int maxX, int minY, int maxY, uchar * pImageData, IplImage* pTexton);

	void extractTextons(int nCluster);
	int scanForTextons(int nCluster, int * pTextonMap);
	void colorTextons(int nTexton, int nCluster, int * pTextonMap);
	void colorTextonMap(uchar *pBorderData,int * pTextonMap,int nCluster);
	void assignTextons(int x, int y, uchar * pData, int * pTextonMap, int nClust);

private:

	IplImage*	m_pImg;
	IplImage*	m_pOutImg;
	IplImage*	m_pSegmentBoundaries;

	CvMat *		m_pClusters;

	int			m_nClusters;
	int			m_nMinTextonSize;
	int			m_nCurTextonSize;

	CvScalar	m_bgColor;
	
};

#endif	//_H_SEGMENTATOR_H_
