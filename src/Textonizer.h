#ifndef _H_SEGMENTATOR_H_
#define _H_SEGMENTATOR_H_

#include <cv.h>
#include <highgui.h>

#include "fe/FeatureExtraction.h"

#define FIRST_TEXTON_NUM	5
#define UNCLUSTERED_DATA	0
#define OUT_OF_SEGMENT_DATA	4
#define BORDER_DATA			1

class Textonizer
{
public:
  Textonizer(IplImage * Img, int nClusters, int nMinTextonSize);
	virtual ~Textonizer();

public:

	void Textonize();

protected:
	
	
	void ColorWindow(int x, int y, int sizeX, int sizeY);
	void SaveImage(char *filename);
	void RecolorPixel(uchar * pData, int y, int x, CvScalar * pColor);

	void Segment();
	void Cluster(CFeatureExtraction *pFeatureExtractor);

    void colorCluster(int nCluster);
	void cannyEdgeDetect(int nCluster);
	void FindClusterBoundaries(int nCluster);

	void extractTextons(int nCluster);
	void assignTextons(int x, int y, uchar * pData, int * pTextonBool, int nClust);

	void repairData(uchar *pBorderData,int nCluster);
	int fixDataIterI(uchar *pBorderData, int nCluster);
	int fixDataIterII(uchar *pBorderData, int nCluster);
	int fixDataIterIII(uchar *pBorderData,int nCluster);

	void CalcHistogram();
	void CalcClusters(CvMat * pData);
	
	
	void colorTextonMap(uchar *pBorderData,int * pTextonBool,int nCluster);
	
	inline bool isInCluster(int i, int j, int nCluster) {
		return (m_pClusters->data.i[j*m_pImg->width+i] == nCluster);
	}

protected:

	IplImage *	m_pImg;
	IplImage *	m_pOutImg;
	IplImage*	m_pSegmentBoundaries;

	int m_nCurTextonSize;

	CvMat * m_pClusters;
	CvMat * m_pHist;
	CvMat * m_pTextons;

	int m_nClusters;
	int m_nMinTextonSize;
	
};

#endif	//_H_SEGMENTATOR_H_
