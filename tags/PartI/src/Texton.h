#ifndef __H_TEXTON_H__
#define __H_TEXTON_H__

#include <highgui.h>

class Texton
{
public:
	Texton(IplImage * textonImg, int nCluster);
	virtual ~Texton();

	const IplImage* getTextonImg() const		{ return m_textonImg; }
	int				getClusterNumber() const	{ return m_nCluster; }

private:
	IplImage*	m_textonImg;
	int			m_nCluster;
};

#endif	//__H_TEXTON_H__