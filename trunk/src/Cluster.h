#ifndef __H_CLUSTER_H__
#define __H_CLUSTER_H__

#include <vector>

#include "Texton.h"

class Cluster
{
public:
	Cluster():m_fBackground(false) {}

	void setBackground()		{ m_fBackground = true; }
	bool isBackground() const	{ return m_fBackground; }

public:
	
	Texton**	m_textonList;
	int			m_nClusterSize;
	bool		m_fBackground;
};

#endif	//__H_CLUSTER_H__