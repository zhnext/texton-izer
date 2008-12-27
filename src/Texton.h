#ifndef __H_TEXTON_H__
#define __H_TEXTON_H__

#include <highgui.h>
#include <vector>
using std::vector;

struct SBox
{
	SBox(int minx, int miny, int maxx, int maxy)
	{
		minX = minx;
		maxX = maxx;
		minY = miny;
		maxY = maxy;
	}

	int minX;
	int minY;
	int maxX;
	int maxY;
};

class CoOccurences
{
public:
	CoOccurences(int distx, int disty, int ncluster)
	{
		distX = distx;
		distY = disty;
		nCluster = ncluster;
	}

	int distX;
	int distY;
	int nCluster;
};

class Texton
{
public:
	enum EPosition { NO_BORDER = 0x0, 
					TOP_BORDER = 0x1, 
					LEFT_BORDER = 0x2, 
					RIGHT_BORDER = 0x4, 
					BOTTOM_BORDER = 0x8 };

public:
	Texton(IplImage * textonImg, int nCluster, int positionMask, CvScalar& means, SBox& box);
	virtual ~Texton();

	const IplImage* getTextonImg() const		{ return m_textonImg; }
	int				getClusterNumber() const	{ return m_nCluster; }
	int				getPosition() const			{ return m_positionMask; }		
	CvScalar		getMeans() const			{ return m_means; }
	SBox			getBoundingBox() const		{ return m_box; }
	vector<CoOccurences>* getCoOccurences()		{ return &m_coOccurences; }
	int				getDilationArea() const		{ return m_nDilation; }

	void			setCoOccurences(vector<CoOccurences> coOccurences);
	void			setDilationArea(int nDilation);

private:
	IplImage*	m_textonImg;
	int			m_nCluster;
	int			m_positionMask;
	int			m_nDilation;
	CvScalar	m_means;
	SBox		m_box;
	vector<CoOccurences> m_coOccurences;
};


#endif	//__H_TEXTON_H__