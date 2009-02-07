#ifndef __H_TEXTON_H__
#define __H_TEXTON_H__

#include <highgui.h>
#include <vector>
using std::vector;

class SBox
{
public:
	SBox(int minx, int miny, int maxx, int maxy)
	{
		minX = minx;
		maxX = maxx;
		minY = miny;
		maxY = maxy;
	}

	inline int getWidth() {
		return ((maxX - minX == 0) ? 1 : (maxX - minX));
	}

	inline int getHeight() {
		return ((maxY - minY == 0) ? 1 : (maxY - minY));
	}

	int getPositionMask(IplImage *pImg);

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
	enum EPosition { NON_BORDER = 0x0, 
					TOP_BORDER = 0x1, 
					LEFT_BORDER = 0x2, 
					RIGHT_BORDER = 0x4, 
					BOTTOM_BORDER = 0x8 };

public:
	Texton(IplImage * textonImg, int nCluster, int positionMask, SBox& box);
	virtual ~Texton();

	 IplImage* getTextonImg() const		{ return m_textonImg; }
	int				getClusterNumber() const	{ return m_nCluster; }
	int				getPosition() const			{ return m_positionMask; }		
	SBox			getBoundingBox() const		{ return m_box; }
	vector<CoOccurences>* getCoOccurences()		{ return &m_coOccurences; }
	int				getDilationArea() const		{ return m_nDilation; }
	bool			isImageBackground() const		{ return m_fImageBackground; }

	void			setImageBackground();
	void			setCoOccurences(vector<CoOccurences> coOccurences);
	void			setDilationArea(int nDilation);

	void			addAppereance();
	int				getAppereances()			{ return m_nAppereances; }

	bool			operator<(const Texton& right) const;

private:
	bool		m_fImageBackground;
	IplImage*	m_textonImg;
	int			m_nCluster;
	int			m_positionMask;
	int			m_nDilation;
	int			m_nAppereances;
	SBox		m_box;
	vector<CoOccurences> m_coOccurences;

	
};


#endif	//__H_TEXTON_H__