#pragma once

template <class T>
class NiRect
{
public:
	NiRect(T left = T(0), T right = T(0), T top = T(0), T bottom = T(0)) :
		m_left(left),
		m_right(right),
		m_top(top),
		m_bottom(bottom)
	{}

	T GetWidth() const { return m_right > m_left ? (m_right - m_left) : (m_left - m_right); }
	T GetHeight() const { return m_top > m_bottom ? (m_top - m_bottom) : (m_bottom - m_top); }

	T m_left;
	T m_right;
	T m_top;
	T m_bottom;
};
