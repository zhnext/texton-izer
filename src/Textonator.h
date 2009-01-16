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

	/**
	 * Color all pixels which are not in the cluster nCluster
	 * @param the cluster which should not be colored
	 **/
    void	colorCluster(int nCluster);

	void	blurImage();

	/**
	 * Use Canny edge detector to create a edge image in m_pSegmentBoundariess
	 **/
	void	cannyEdgeDetect();

	void	extractTextons(int nCluster, vector<Cluster>& clusterList, int * pTextonMap);

	/**
	 * Create a texton map of the current cluster
	 * @param nCluster the current cluster
	 * @param pTextonMap the result texton map
	 * @return the number of textons we colored
	 **/
	int		scanForTextons(int nCluster, int * pTextonMap);

	/**
	 * Color a texton "map" based on the clustering:
     * For pixels inside nCluster:
	 *	 BORDER_DATA for edges which we found in pBorderData, 
	 *	 UNCLUSTERED_DATA for things which are not edges
	 * For pixels outside nCluster
	 *	 OUT_OF_SEGMENT_DATA
	 * @param pBorderData the edge image input
	 * @param pTextonMap the texton map to color
	 * @param nCluster the current cluster number
	 **/
	void	colorTextonMap(uchar *pBorderData,int * pTextonMap,int nCluster);

	/**
	 * "Flood fill" pTextonMap with the value nTexton from the current coordinate
	 * Stop on borders and when we are out of our cluster
	 * @param x the x value to check
	 * @param y the y value to check
	 * @param pData the image data
	 * @param pTextonMap the textons location map
	 * @param nTexton the current texton
	 **/
	void	assignTextons(int x, int y, uchar * pData, int * pTextonMap, int nTexton);
	void	retrieveTextons(int nTexton, int nCluster, int * pTextonMap, vector<Cluster>& clusterList);
	
	void	assignRemainingData(int * pTextonMap);

	/**
	 * Assign any lonely pixels, that appear inside a texton, 
	 * and belong to another cluster, to that texton
	 * @param ppTextonMap a pointer to the current texton map
	 * @param nSize the texton map size
	 **/
	void	assignStrayPixels(int * ppTextonMap, int nSize);

	/**
	 * Extract bounding box minX,minY,maxX,maxY from pImageData to pTexton
	 **/
	void	extractTexton(int minX, int maxX, int minY, int maxY, uchar * pImageData, IplImage* pTexton);
	
	/**
	 * Extract boundingBox from pImageData to pTexton
	 * @param boundingBox the bounding box to extract
	 * @param pImageData the image to extract the data from
	 * @param pTexton the texton to extract the data to
	 **/
	void	extractTexton(SBox& boundingBox, 
							uchar * pImageData, 
							IplImage* pTexton);
	
	
	void	computeCoOccurences(vector<int*> pTextonMapList, vector<Cluster>& clusterList);
	void	retrieveTextonCoOccurences(int nCluster, int nOffsetCurTexton, vector<Occurence>& Occurences, CvScalar& bg, uchar * pData,vector<int*> pTextonMapList);
	void	computeTextonCoOccurences(Texton * curTexton, vector<Occurence>& Occurences, vector<Cluster>& clusterList);

	/**
	 * Get 8 neighbors of the current pixel
	 * @param map - the map the extract neighbor values from
	 * @param i, j - the coordinates whose neighbors we want
	 * @param width, height - width and height of map
	 * @param[out] arrNeighbors - the output array of neighbor values
	 **/
	void getNeighbors(int *map, int i, int j, int width, int height, int arrNeighbors[]);

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
