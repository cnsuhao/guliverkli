/* 
 *	Copyright (C) 2003-2006 Gabest
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

#include "stdafx.h"
#include "Subtitle.h"
#include "Split.h"
#include <math.h>

namespace ssf
{
	struct Subtitle::n2n_t Subtitle::m_n2n;

	Subtitle::Subtitle(File* pFile) 
		: m_pFile(pFile)
	{
		if(m_n2n.align[0].IsEmpty())
		{
			m_n2n.align[0][L"left"] = 0;
			m_n2n.align[0][L"center"] = 0.5;
			m_n2n.align[0][L"middle"] = 0.5;
			m_n2n.align[0][L"right"] = 1;
		}
		
		if(m_n2n.align[1].IsEmpty())
		{
			m_n2n.align[1][L"top"] = 0;
			m_n2n.align[1][L"middle"] = 0.5;
			m_n2n.align[1][L"center"] = 0.5;
			m_n2n.align[1][L"bottom"] = 1;
		}
		
		if(m_n2n.weight.IsEmpty())
		{
			m_n2n.weight[L"thin"] = FW_THIN;
			m_n2n.weight[L"normal"] = FW_NORMAL;
			m_n2n.weight[L"bold"] = FW_BOLD;
		}

		if(m_n2n.transition.IsEmpty())
		{
			m_n2n.transition[L"start"] = 0;
			m_n2n.transition[L"stop"] = 1e10;
			m_n2n.transition[L"linear"] = 1;
		}
	}

	Subtitle::~Subtitle() 
	{
	}

	bool Subtitle::Parse(Definition* pDef, double start, double stop, double at)
	{
		ASSERT(m_pFile && pDef);

		m_text.RemoveAll();

		m_time.start = start;
		m_time.stop = stop;

		at -= start;

		Fill::gen_id = 0;

		try
		{
			Definition& frame = (*pDef)[L"frame"];

			m_frame.reference = frame[L"reference"];
			m_frame.resolution.cx = frame[L"resolution"][L"cx"];
			m_frame.resolution.cy = frame[L"resolution"][L"cy"];

			Definition& direction = (*pDef)[L"direction"];

			m_direction.primary = direction[L"primary"];
			m_direction.secondary = direction[L"secondary"];

			m_wrap = (*pDef)[L"wrap"];

			m_layer = (*pDef)[L"layer"];

			m_pFile->Commit();

			Style style;
			GetStyle(&(*pDef)[L"style"], style);

			CAtlStringMapW<double> offset;
			Definition& block = (*pDef)[L"@"];
			Parse(WCharStream((LPCWSTR)block), style, at, offset, dynamic_cast<Reference*>(block.m_parent));

			m_pFile->Rollback();

			// TODO: trimming should be done by the renderer later, after breaking the words into lines

			while(!m_text.IsEmpty() && (m_text.GetHead().str == Text::SP || m_text.GetHead().str == Text::LSEP))
				m_text.RemoveHead();

			while(!m_text.IsEmpty() && (m_text.GetTail().str == Text::SP || m_text.GetTail().str == Text::LSEP))
				m_text.RemoveTail();

			for(POSITION pos = m_text.GetHeadPosition(); pos; m_text.GetNext(pos))
			{
				if(m_text.GetAt(pos).str == Text::LSEP)
				{
					POSITION prev = pos;
					m_text.GetPrev(prev);

					while(prev && m_text.GetAt(prev).str == Text::SP)
					{
						POSITION tmp = prev;
						m_text.GetPrev(prev);
						m_text.RemoveAt(tmp);
					}

					POSITION next = pos;
					m_text.GetNext(next);

					while(next && m_text.GetAt(next).str == Text::SP)
					{
						POSITION tmp = next;
						m_text.GetNext(next);
						m_text.RemoveAt(tmp);
					}
				}
			}
		}
		catch(Exception& e)
		{
			TRACE(_T("%s"), e.ToString());
			return false;
		}

		return true;
	}

	void Subtitle::GetStyle(Definition* pDef, Style& style)
	{
		style.placement.pos.x = 0;
		style.placement.pos.y = 0;
		style.placement.pos.auto_x = true;
		style.placement.pos.auto_y = true;

		Rect frame = {0, m_frame.resolution.cx, m_frame.resolution.cy, 0};

		style.placement.clip.t = -1;
		style.placement.clip.r = -1;
		style.placement.clip.b = -1;
		style.placement.clip.l = -1;

		//

		style.linebreak = (*pDef)[L"linebreak"];

		Definition& placement = (*pDef)[L"placement"];

		Definition& clip = placement[L"clip"];

		if(clip.IsValue(Definition::string))
		{
			CStringW str = clip;

			if(str == L"frame") style.placement.clip = frame;
			// else ?
		}
		else
		{
			if(clip[L"t"].IsValue()) style.placement.clip.t = clip[L"t"];
			if(clip[L"r"].IsValue()) style.placement.clip.r = clip[L"r"];
			if(clip[L"b"].IsValue()) style.placement.clip.b = clip[L"b"];
			if(clip[L"l"].IsValue()) style.placement.clip.l = clip[L"l"];
		}

		CAtlStringMapW<double> n2n_margin[2];

		n2n_margin[0][L"top"] = 0;
		n2n_margin[0][L"right"] = 0;
		n2n_margin[0][L"bottom"] = frame.b - frame.t;
		n2n_margin[0][L"left"] = frame.r - frame.l;
		n2n_margin[1][L"top"] = frame.b - frame.t;
		n2n_margin[1][L"right"] = frame.r - frame.l;
		n2n_margin[1][L"bottom"] = 0;
		n2n_margin[1][L"left"] = 0;

		placement[L"margin"][L"t"].GetAsNumber(style.placement.margin.t, &n2n_margin[0]);
		placement[L"margin"][L"r"].GetAsNumber(style.placement.margin.r, &n2n_margin[0]);
		placement[L"margin"][L"b"].GetAsNumber(style.placement.margin.b, &n2n_margin[1]);
		placement[L"margin"][L"l"].GetAsNumber(style.placement.margin.l, &n2n_margin[1]);

		placement[L"align"][L"h"].GetAsNumber(style.placement.align.h, &m_n2n.align[0]);
		placement[L"align"][L"v"].GetAsNumber(style.placement.align.v, &m_n2n.align[1]);

		if(placement[L"pos"][L"x"].IsValue()) {style.placement.pos.x = placement[L"pos"][L"x"]; style.placement.pos.auto_x = false;}
		if(placement[L"pos"][L"y"].IsValue()) {style.placement.pos.y = placement[L"pos"][L"y"]; style.placement.pos.auto_y = false;}

		placement[L"offset"][L"x"].GetAsNumber(style.placement.offset.x);
		placement[L"offset"][L"y"].GetAsNumber(style.placement.offset.y);

		style.placement.angle.x = placement[L"angle"][L"x"];
		style.placement.angle.y = placement[L"angle"][L"y"];
		style.placement.angle.z = placement[L"angle"][L"z"];

		Definition& font = (*pDef)[L"font"];

		style.font.face = font[L"face"];
		style.font.size = font[L"size"];
		font[L"weight"].GetAsNumber(style.font.weight, &m_n2n.weight);
		style.font.color.a = font[L"color"][L"a"];
		style.font.color.r = font[L"color"][L"r"];
		style.font.color.g = font[L"color"][L"g"];
		style.font.color.b = font[L"color"][L"b"];
		style.font.underline = font[L"underline"];
		style.font.strikethrough = font[L"strikethrough"];
		style.font.italic = font[L"italic"];
		style.font.spacing = font[L"spacing"];
		style.font.scale.cx = font[L"scale"][L"cx"];
		style.font.scale.cy = font[L"scale"][L"cy"];

		Definition& background = (*pDef)[L"background"];

		style.background.color.a = background[L"color"][L"a"];
		style.background.color.r = background[L"color"][L"r"];
		style.background.color.g = background[L"color"][L"g"];
		style.background.color.b = background[L"color"][L"b"];
		style.background.size = background[L"size"];
		style.background.type = background[L"type"];

		Definition& shadow = (*pDef)[L"shadow"];

		style.shadow.color.a = shadow[L"color"][L"a"];
		style.shadow.color.r = shadow[L"color"][L"r"];
		style.shadow.color.g = shadow[L"color"][L"g"];
		style.shadow.color.b = shadow[L"color"][L"b"];
		style.shadow.depth = shadow[L"depth"];
		style.shadow.angle = shadow[L"angle"];
		style.shadow.blur = shadow[L"blur"];

		Definition& fill = (*pDef)[L"fill"];

		style.fill.color.a = fill[L"color"][L"a"];
		style.fill.color.r = fill[L"color"][L"r"];
		style.fill.color.g = fill[L"color"][L"g"];
		style.fill.color.b = fill[L"color"][L"b"];
		style.fill.width = fill[L"width"];
	}

	double Subtitle::GetMixWeight(Definition* pDef, double at, CAtlStringMapW<double>& offset, int default_id)
	{
		double t = 1;

		try
		{
			CAtlStringMapW<double> n2n;

			n2n[L"start"] = 0;
			n2n[L"stop"] = m_time.stop - m_time.start;

			Definition::Time time;
			if(pDef->GetAsTime(time, offset, &n2n, default_id) && time.start.value < time.stop.value)
			{
				t = (at - time.start.value) / (time.stop.value - time.start.value);

				double u = t;

				if(t < 0) t = 0;
				else if(t >= 1) t = 0.999999; // doh

				if((*pDef)[L"loop"].IsValue()) t *= (double)(*pDef)[L"loop"];

				CStringW direction = (*pDef)[L"direction"].IsValue() ? (*pDef)[L"direction"] : L"fw";
				if(direction == L"fwbw" || direction == L"bwfw") t *= 2;

				double n;
				t = modf(t, &n);

				if(direction == L"bw" 
				|| direction == L"fwbw" && ((int)n & 1)
				|| direction == L"bwfw" && !((int)n & 1)) 
					t = 1 - t;

				double accel = 1;

				if((*pDef)[L"transition"].IsValue())
				{
					Definition::Number<double> n;
					(*pDef)[L"transition"].GetAsNumber(n, &m_n2n.transition);
					if(n.value >= 0) accel = n.value;
				}

				if(t == 0.999999) t = 1;

				if(u >= 0 && u < 1)
				{
					t = accel == 0 ? 1 : 
						accel == 1 ? t : 
						accel >= 1e10 ? 0 :
						pow(t, accel);
				}
			}
		}
		catch(Exception&)
		{
		}

		return t;
	}

	template<class T> 
	bool Subtitle::MixValue(Definition& def, T& value, double t)
	{
		CAtlStringMapW<T> n2n;
		return MixValue(def, value, t, &n2n);
	}

	template<> 
	bool Subtitle::MixValue(Definition& def, double& value, double t)
	{
		CAtlStringMapW<double> n2n;
		return MixValue(def, value, t, &n2n);
	}

	template<class T> 
	bool Subtitle::MixValue(Definition& def, T& value, double t, CAtlStringMapW<T>* n2n)
	{
		if(!def.IsValue()) return false;

		if(t >= 0.5)
		{
			if(n2n && def.IsValue(Definition::string))
			{
				if(CAtlStringMapW<T>::CPair* p = n2n->Lookup(def))
				{
					value = p->m_value;
					return true;
				}
			}

			value = (T)def;
		}

		return true;
	}

	template<> 
	bool Subtitle::MixValue(Definition& def, double& value, double t, CAtlStringMapW<double>* n2n)
	{
		if(!def.IsValue()) return false;

		if(t > 0)
		{
			if(n2n && def.IsValue(Definition::string))
			{
				if(CAtlStringMap<double, CStringW>::CPair* p = n2n->Lookup(def))
				{
					value += (p->m_value - value) * t;
					return true;
				}
			}

			value += ((double)def - value) * t;
		}

		return true;
	}

	void Subtitle::MixStyle(Definition* pDef, Style& dst, double t)
	{
		const Style src = dst;

		if(t <= 0) return;
		else if(t > 1) t = 1;

		MixValue((*pDef)[L"linebreak"], dst.linebreak, t);

		Definition& placement = (*pDef)[L"placement"];

		MixValue(placement[L"clip"][L"t"], dst.placement.clip.t, t);
		MixValue(placement[L"clip"][L"r"], dst.placement.clip.r, t);
		MixValue(placement[L"clip"][L"b"], dst.placement.clip.b, t);
		MixValue(placement[L"clip"][L"l"], dst.placement.clip.l, t);
		MixValue(placement[L"align"][L"h"], dst.placement.align.h, t, &m_n2n.align[0]);
		MixValue(placement[L"align"][L"v"], dst.placement.align.v, t, &m_n2n.align[1]);
		dst.placement.pos.auto_x = !MixValue(placement[L"pos"][L"x"], dst.placement.pos.x, dst.placement.pos.auto_x ? 1 : t);
		dst.placement.pos.auto_y = !MixValue(placement[L"pos"][L"y"], dst.placement.pos.y, dst.placement.pos.auto_y ? 1 : t);
		MixValue(placement[L"offset"][L"x"], dst.placement.offset.x, t);
		MixValue(placement[L"offset"][L"y"], dst.placement.offset.y, t);
		MixValue(placement[L"angle"][L"x"], dst.placement.angle.x, t);
		MixValue(placement[L"angle"][L"y"], dst.placement.angle.y, t);
		MixValue(placement[L"angle"][L"z"], dst.placement.angle.z, t);

		Definition& font = (*pDef)[L"font"];

		MixValue(font[L"face"], dst.font.face, t);
		MixValue(font[L"size"], dst.font.size, t);
		MixValue(font[L"weight"], dst.font.weight, t, &m_n2n.weight);
		MixValue(font[L"color"][L"a"], dst.font.color.a, t);
		MixValue(font[L"color"][L"r"], dst.font.color.r, t);
		MixValue(font[L"color"][L"g"], dst.font.color.g, t);
		MixValue(font[L"color"][L"b"], dst.font.color.b, t);
		MixValue(font[L"underline"], dst.font.underline, t);
		MixValue(font[L"strikethrough"], dst.font.strikethrough, t);
		MixValue(font[L"italic"], dst.font.italic, t);
		MixValue(font[L"spacing"], dst.font.spacing, t);
		MixValue(font[L"scale"][L"cx"], dst.font.scale.cx, t);
		MixValue(font[L"scale"][L"cy"], dst.font.scale.cy, t);

		Definition& background = (*pDef)[L"background"];

		MixValue(background[L"color"][L"a"], dst.background.color.a, t);
		MixValue(background[L"color"][L"r"], dst.background.color.r, t);
		MixValue(background[L"color"][L"g"], dst.background.color.g, t);
		MixValue(background[L"color"][L"b"], dst.background.color.b, t);
		MixValue(background[L"size"], dst.background.size, t);
		MixValue(background[L"type"], dst.background.type, t);

		Definition& shadow = (*pDef)[L"shadow"];

		MixValue(shadow[L"color"][L"a"], dst.shadow.color.a, t);
		MixValue(shadow[L"color"][L"r"], dst.shadow.color.r, t);
		MixValue(shadow[L"color"][L"g"], dst.shadow.color.g, t);
		MixValue(shadow[L"color"][L"b"], dst.shadow.color.b, t);
		MixValue(shadow[L"depth"], dst.shadow.depth, t);
		MixValue(shadow[L"angle"], dst.shadow.angle, t);
		MixValue(shadow[L"blur"], dst.shadow.blur, t);

		Definition& fill = (*pDef)[L"fill"];

		MixValue(fill[L"color"][L"a"], dst.fill.color.a, t);
		MixValue(fill[L"color"][L"r"], dst.fill.color.r, t);
		MixValue(fill[L"color"][L"g"], dst.fill.color.g, t);
		MixValue(fill[L"color"][L"b"], dst.fill.color.b, t);
		MixValue(fill[L"width"], dst.fill.width, t);

		if(fill.m_priority >= PNormal) // this assumes there is no way to set low priority inline overrides
		{
			if(dst.fill.id > 0) throw Exception(_T("cannot apply fill more than once on the same text"));
			dst.fill.id = ++Fill::gen_id;
		}
	}

	void Subtitle::Parse(Stream& s, Style style, double at, CAtlStringMapW<double>& offset, Reference* pParentRef)
	{
		Text text;
		text.style = style;

		for(int c = s.PeekChar(); c != Stream::EOS; c = s.PeekChar())
		{
			s.GetChar();

			if(c == '[')
			{
				AddText(text);

				style = text.style;

				CAtlStringMapW<double> inneroffset = offset;

				int default_id = 0;

				do
				{
					Definition* pDef = m_pFile->CreateDef(pParentRef);

					m_pFile->ParseRefs(s, pDef, L",;]");

					ASSERT(pDef->IsType(L"style") || pDef->IsTypeUnknown());

					double t = GetMixWeight(pDef, at, offset, ++default_id);
					MixStyle(pDef, style, t);

					if((*pDef)[L"@"].IsValue())
					{
						Parse(WCharStream((LPCWSTR)(*pDef)[L"@"]), style, at, offset, pParentRef);
					}

					s.SkipWhiteSpace();
					c = s.GetChar();
				}
				while(c == ',' || c == ';');

				if(c != ']') s.ThrowError(_T("unterminated override"));

				bool fWhiteSpaceNext = s.IsWhiteSpace(s.PeekChar());
				c = s.SkipWhiteSpace();

				if(c == '{') 
				{
					s.GetChar();
					Parse(s, style, at, inneroffset, pParentRef);
				}
				else 
				{
					if(fWhiteSpaceNext) text.str += (WCHAR)Text::SP;
					text.style = style;
				}
			}
			else if(c == ']')
			{
				s.ThrowError(_T("unexpected ] found"));
			}
			else if(c == '{')
			{
				CAtlStringMapW<double> inneroffset = offset;

				AddText(text);
				Parse(s, text.style, at, offset, pParentRef);
			}
			else if(c == '}')
			{
				break;
			}
			else
			{
				if(c == '\\')
				{
					c = s.GetChar();
					if(c == Stream::EOS) break;
					else if(c == 'n') {AddText(text); text.str = (WCHAR)Text::LSEP; AddText(text); continue;}
					else if(c == 'h') c = Text::NBSP;
				}

				AddChar(text, (WCHAR)c);
			}
		}

		AddText(text);
	}

	void Subtitle::AddChar(Text& t, WCHAR c)
	{
		bool f1 = !t.str.IsEmpty() && Stream::IsWhiteSpace(t.str[t.str.GetLength()-1]);
		bool f2 = Stream::IsWhiteSpace(c);
		if(f2) c = Text::SP;
		if(!f1 || !f2) t.str += (WCHAR)c;
	}

	void Subtitle::AddText(Text& t)
	{
		if(t.str.IsEmpty()) return;

		Split sa(' ', t.str, 0, Split::Max);

		for(size_t i = 0, n = sa; i < n; i++)
		{
			CStringW str = sa[i];

			if(!str.IsEmpty())
			{
				t.str = str;
				m_text.AddTail(t);
			}

			if(i < n-1 && (m_text.IsEmpty() || m_text.GetTail().str != Text::SP))
			{
				t.str = (WCHAR)Text::SP;
				m_text.AddTail(t);
			}
		}
		
		t.str.Empty();
	}

	//

	unsigned int Fill::gen_id = 0;
}