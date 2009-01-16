#ifndef __H_SYNTHESIZER_H__
#define __H_SYNTHESIZER_H__

#include <vector>
#include <cv.h>
#include <time.h>
#include <list>
#include "Cluster.h"

using std::vector;
using std::list;

#define AVG_DILATION_ERROR		2
#define MAXIMUM_TEXTON_OVERLAP	8

/**
 * A Synthesizer class that retrieves a list of textons partitioned by clusters and
 * outputs a new synthesized image.
 **/
class Synthesizer
{
public:
	Synthesizer();
	virtual ~Synthesizer();

	/**
	 * Synthesize a new image
	 * @param nNewWidth, nNewHeight, depth - the width, height, depth of the new image
	 * @param nChannels - number of channels in the new image
	 * @param clusterList - list of clusters to choose from
	 * @returns the synthesized IplImage image                                
	 **/
	IplImage* synthesize(int nNewWidth, int nNewHeight, int depth, 
						int nChannels, vector<Cluster> &clusterList);

private:
	/**
	 * Insert the texton into the synthesized image at a specific spot
	 * @param x the x value of the insertion destination
	 * @param y the y value of the insertion destination
	 * @param textonImg the texton we want to insert
	 * @param synthesizedImage the image in which we place the texton
	 * @return true iff the texton was inserted successully
	 **/
	bool insertTexton(int x, int y, const IplImage * textonImg, IplImage* synthesizedImage);

	/**
	 * Synthesize the image using the input cluster list by applying the precomputed co-occurrence
	 * relations between clusters.
	 * @param clusterList the cluster list
	 * @param synthesizedImage the output synthesized image
	 **/
	void Synthesizer::synthesizeImage(vector<Cluster> &clusterList, 
									IplImage * synthesizedImage);

	/**
	 * Choose the first texton to start the synthesized image from.
	 * @param clusterList the cluster list to choose the first texton from.
	 * @return the first texton to start image from
	 * @throws SynthesizerException if no adequete first texton is found
	 **/
	Texton* Synthesizer::chooseFirstTexton(vector<Cluster> &clusterList);

	/**
	 *  Copy an image from src into dst without the nBorderSize outer pixels of src
	 * if src has size [x+nBorderSize*2, y+nBorderSize*2] then dst must have size [x,y].
	 * @param src source image
	 * @param dst destination image
	 * @param nBorderSize the border size
	 **/
	void copyImageWithoutBorder(IplImage * src,IplImage * dst, int nBorderSize);

	/**
	 * Copy from src to dst pixels with colors different from m_resultBgColor.
	 * @param src source image
	 * @param dst destination image
	 **/
	void copyImageWithoutBackground(IplImage * src,IplImage * dst);

	/**
	 * Check for a given texton if the surrounding in the desired area is clear 
	 * (no other textons in the given vicinity)
	 * @param x,y the desired texton location
	 * @param t the texton we want to insert
	 * @param synthesizedImage the image on which we check the collisions
	 * @return true iff the texton's surrounding is clear
	 **/
	bool checkSurrounding(int x, int y, Texton* t, IplImage* synthesizedImage);

	/**
	 * Create a background image of size [img->width, img->height]
	 * By randomly selecting pixels from an image filling texton.     
	 * @param clusterList the cluster list to choose the background pixels from
	 * @param img the source image (used to acquire background image attributes)
	 * @return a new synthesized IplImage background image
	 **/
	IplImage * retrieveBackground(vector<Cluster> &clusterList, IplImage * img);

	/**
	 * Go through all the clusters and compute the dilation area average, if 
	 * the dilation area is mainly "big", then it means the textons should be far from
	 * each other, so eliminate all the textons that are "too close" (touching each
	 * other), as those are probably faulty textons.
	 * @param clusterList the list of clusters to remove nonconforming textons from
	 **/
	void removeNonconformingTextons(vector<Cluster> &clusterList);

	/**
	 * Remove from clusterList textons which are either border textons 
	 * or image filling textons.
     * @param clusterList the list of clusters to remove border textons from
	 **/
	void removeBorderTextons(vector<Cluster>& clusterList);

private:
	class SynthesizerException {};

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