#ifndef _H_SEGMENTATOR_H_
#define _H_SEGMENTATOR_H_

#include <vector>

#include <cv.h>
#include <highgui.h>

#include "fe/FeatureExtraction.h"
#include "Cluster.h"

using std::vector;

#define FIRST_TEXTON_NUM	5
#define UNCLUSTERED_DATA	0
#define OUT_OF_SEGMENT_DATA	4
#define BORDER_DATA			1

#define EDGE_DATA			255

#define UNDEFINED			-1

#define MAX_DILATIONS		100
#define EXTRA_DILATIONS		20

class Occurence
{
public:
	Occurence(int nTexton, int nCluster,int nDistance = UNDEFINED)
		:m_nDistance(nDistance),m_nTexton(nTexton),m_nCluster(nCluster) {}

	bool operator==(const Occurence& o) {
		return (m_nCluster == o.m_nCluster && m_nTexton == o.m_nTexton);
	}

	int m_nDistance;
	int m_nTexton;
	int m_nCluster;
};

class Textonator
{
public:
	Textonator(IplImage * Img, int nClusters, int nMinTextonSize);
	virtual ~Textonator();

	void	textonize(vector<Cluster>& clusterList);

private:
	
	void	colorWindow(int x, int y, int sizeX, int sizeY);
	void	recolorPixel(uchar * pData, int y, int x, int step, CvScalar * pColor);

	void	segment();
	void	cluster(CFeatureExtraction *pFeatureExtractor);

    void	colorCluster(int nCluster);
	void	blurImage();
	void	cannyEdgeDetect();
	void	extractTextons(int nCluster, vector<Cluster>& clusterList, int * pTextonMap);

	int		scanForTextons(int nCluster, int * pTextonMap);
	void	colorTextonMap(uchar *pBorderData,int * pTextonMap,int nCluster);
	void	assignTextons(int x, int y, uchar * pData, int * pTextonMap, int nClust);
	void	retrieveTextons(int nTexton, int nCluster, int * pTextonMap, vector<Cluster>& clusterList);
	
	void	assignRemainingData(int * pTextonMap);
	void	assignStrayPixels(int * ppTextonMap, int nSize);

	void	extractTexton(int minX, int maxX, int minY, int maxY, uchar * pImageData, IplImage* pTexton);
	void	extractTexton(SBox& boundingBox, 
							uchar * pImageData, 
							IplImage* pTexton);
	
	
	void	computeCoOccurences(vector<int*> pTextonMapList, vector<Cluster>& clusterList);
	void	retrieveTextonCoOccurences(int nCluster, int nOffsetCurTexton, vector<Occurence>& Occurences, CvScalar& bg, uchar * pData,vector<int*> pTextonMapList);
	void	computeTextonCoOccurences(Texton * curTexton, vector<Occurence>& Occurences, vector<Cluster>& clusterList);

private:
	void getNeighbors(int *map, int i, int j, int width, int height, int arrNeighbors[]);

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
