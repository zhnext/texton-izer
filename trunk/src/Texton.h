#ifndef __H_TEXTON_H__
#define __H_TEXTON_H__

#include <highgui.h>

class Texton
{
public:
	enum EPosition { NO_BORDER = 0x0, 
					TOP_BORDER = 0x1, 
					LEFT_BORDER = 0x2, 
					RIGHT_BORDER = 0x4, 
					BOTTOM_BORDER = 0x8 };

public:
	Texton(IplImage * textonImg, int nCluster, int positionMask, CvScalar& means);
	virtual ~Texton();

	const IplImage* getTextonImg() const		{ return m_textonImg; }
	int				getClusterNumber() const	{ return m_nCluster; }
	int				getPosition() const			{ return m_positionMask; }		
	CvScalar		getMeans() const			{ return m_means; }
	bool			isBackground() const		{ return m_fBackground; }

	void			setBackground();

private:
	IplImage*	m_textonImg;
	int			m_nCluster;
	int			m_positionMask;
	CvScalar	m_means;
	bool		m_fBackground;
};

#endif	//__H_TEXTON_H__