#ifndef __H_CLUSTER_H__
#define __H_CLUSTER_H__

#include <vector>
using std::vector;

#include "Texton.h"

class Cluster
{
public:
	Cluster():m_fBackground(false),m_fImageFilling(false) {}

	void setBackground()		{ m_fBackground = true; }
	void setImageFilling()		{ m_fImageFilling = true; }

	bool isBackground() const	{ return m_fBackground; }
	bool isImageFilling() const	{ return m_fImageFilling; }

public:
	
	vector<Texton*>	m_textonList;
	int			m_nClusterSize;
	bool		m_fBackground;
	bool		m_fImageFilling;
};

#endif	//__H_CLUSTER_H__