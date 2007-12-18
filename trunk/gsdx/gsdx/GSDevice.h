/* 
 *	Copyright (C) 2007 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GSTexture.h"

template<class Texture> class GSDevice
{
	CAtlList<Texture> m_pool;

	Texture m_weavebob;
	Texture m_blend;

protected:
	bool Fetch(int type, Texture& t, int w, int h, int format)
	{
		Recycle(t);

		for(POSITION pos = m_pool.GetHeadPosition(); pos; m_pool.GetNext(pos))
		{
			const Texture& t2 = m_pool.GetAt(pos);

			if(t2.GetType() == type && t2.GetWidth() == w && t2.GetHeight() == h && t2.GetFormat() == format)
			{
				t = t2;

				m_pool.RemoveAt(pos);

				return true;
			}
		}

		return Create(type, t, w, h, format);
	}

	virtual bool Create(int type, Texture& t, int w, int h, int format) = 0;

	virtual void Deinterlace(Texture& st, Texture& dt, int shader, bool linear, float yoffset) = 0;

public:
	GSDevice() 
	{
	}

	virtual ~GSDevice() 
	{
	}

	virtual bool Create(HWND hWnd)
	{
		return true;
	}
	
	virtual bool Reset(int w, int h, bool fs)
	{
		m_pool.RemoveAll();
		m_weavebob = Texture();
		m_blend = Texture();

		return true;
	}

	virtual bool IsLost() = 0;

	virtual void Present(int arx, int ary) = 0;

	virtual void BeginScene() = 0;

	virtual void EndScene() = 0;

	virtual bool CreateRenderTarget(Texture& t, int w, int h, int format = 0)
	{
		return Fetch(GSTexture::RenderTarget, t, w, h, format);
	}

	virtual bool CreateDepthStencil(Texture& t, int w, int h, int format = 0)
	{
		return Fetch(GSTexture::DepthStencil, t, w, h, format);
	}

	virtual bool CreateTexture(Texture& t, int w, int h, int format = 0)
	{
		return Fetch(GSTexture::Texture, t, w, h, format);
	}

	virtual bool CreateOffscreen(Texture& t, int w, int h, int format = 0)
	{
		return Fetch(GSTexture::Offscreen, t, w, h, format);
	}

	void Recycle(Texture& t)
	{
		if(t)
		{
			m_pool.AddHead(t);

			while(m_pool.GetCount() > 200)
			{
				m_pool.RemoveTail();
			}

			t = Texture();
		}
	}

	bool Interlace(Texture& st, Texture& dt, CSize ds, int field, int mode, float yoffset)
	{
		dt = st;

		if(!m_weavebob || m_weavebob.GetWidth() != ds.cx || m_weavebob.GetHeight() != ds.cy)
		{
			CreateRenderTarget(m_weavebob, ds.cx, ds.cy);
		}

		if(mode == 0 || mode == 2) // weave or blend
		{
			// weave first

			Deinterlace(st, m_weavebob, field, false, 0);

			if(mode == 2)
			{
				// blend

				if(!m_blend || m_blend.GetWidth() != ds.cx || m_blend.GetHeight() != ds.cy)
				{
					CreateRenderTarget(m_blend, ds.cx, ds.cy);
				}

				if(field == 0) return false;

				Deinterlace(m_weavebob, m_blend, 2, false, 0);

				dt = m_blend;
			}
			else
			{
				dt = m_weavebob;
			}
		}
		else if(mode == 1) // bob
		{
			Deinterlace(st, m_weavebob, 3, true, yoffset * field);

			dt = m_weavebob;
		}

		return true;
	}
};
