#pragma once
#include "Types.h"
#include "Color.h"

class CPoint {
public:
	int x, y; // TODO: !!!
	CPoint()
	{
	}
	CPoint(int _x, int _y) : 
		x(_x), y(_y)
	{
	}
	CPoint operator +(const CPoint& cp)
	{
		return CPoint( x + cp.x, y + cp.y );
	}
    bool operator !=(const CPoint& cp)
    {
        return cp.x != x || cp.y != y;
    }
};

class CRect {
public:
	CRect()
	{
	}

	CRect( int _left, int _top, int _right, int _bottom ) :
	  left(_left), top(_top), right(_right), bottom(_bottom)
	{
	}
	int CenterX()
	{
		return (left+right)>>1;
	}
	CPoint Center()
	{
		return CPoint( (left+right) >> 1, (top+bottom) >> 1 );
	}
	void Deflate(int l, int t, int r, int b)
	{
		left += l;
		top += t;
		right -= r;
		bottom -= b;
	}

	void Inflate(int l, int t, int r, int b)
	{
		left -= l;
		top -= t;
		right += r;
		bottom += b;
	}
	int Width() const
	{
		return right - left;
	}
	int Height() const
	{
		return bottom - top;
	}
	const CPoint& TopLeft() const
	{
		return *((CPoint*)this);
	}
	void Offset(int x, int y)
	{
		left += x;
		right += x;
		top += y;
		bottom += y;
	}

	void Invalidate()
	{
		left = 0;
		top = 0;
		right = 0;
		bottom = 0;
	}

	bool IsValid() const
	{
		return right > left;
	}

	void operator |= (const CRect& rcUnion)
	{
		if ( !IsValid() )
		{
			*this = rcUnion;
			return;
		}
		left = min(left, rcUnion.left);
		top = min(top, rcUnion.top);
		right = max(right, rcUnion.right);
		bottom = max(bottom, rcUnion.bottom);
	}

	CRect operator +(const CPoint& cp)
	{
		CRect rcNew( *this );
		rcNew.left += cp.x;
		rcNew.right += cp.x;
		rcNew.top += cp.y;
		rcNew.bottom += cp.y;
		return rcNew;
	}

	bool IsInside( int x, int y )
	{
		if ( x < left || x >= right || y < top || y >= bottom )
			return false;
		return true;
	}

	int left, top, right, bottom;
};

template <class TYPE>
class CArray
{
	TYPE	*m_arrElements;
	ui16	m_nCount;
	ui16	m_nMaxCount;

	typedef int (*TCompareFunction)(TYPE&, TYPE&);

public:
	CArray()
	{
	}

	CArray( TYPE *pSource, int nLength )
	{
		m_arrElements = pSource;
		m_nCount = 0;
		m_nMaxCount = nLength;
	}
	
	void Init( TYPE *pSource, int nLength )
	{
		m_arrElements = pSource;
		m_nCount = 0;
		m_nMaxCount = nLength;
	}

	BOOL IsEmpty()
	{
		_ASSERT( m_arrElements );
		return m_nCount == 0;
	}

	void Add(TYPE t)
	{
		_ASSERT( m_nCount < m_nMaxCount );
		m_arrElements[m_nCount++] = t;
	}

	TYPE &GetLast()
	{
		_ASSERT( m_nCount > 0 );
		return m_arrElements[m_nCount-1];
	}
    
	// returns pointer to existing element
	TYPE& RemoveLast()
	{
		_ASSERT( m_nCount > 0 );
		return m_arrElements[--m_nCount];
	}

	void Resize( int nDif )
	{
		m_nCount += nDif;
		_ASSERT( m_nCount >= 0 && m_nCount <= m_nMaxCount );
	}

	int GetSize() const
	{
		return m_nCount;
	}

	int GetMaxSize()
	{
		return m_nMaxCount;
	}

	void SetSize( int nSize )
	{
		m_nCount = nSize;
	}

	TYPE& operator []( int i)
	{
		if ( i < 0 )
			i += m_nCount;
		_ASSERT( i >= 0 && i < GetSize() );
		return m_arrElements[i];
	}

	const TYPE& operator []( int i) const
	{
		if ( i < 0 )
			i += m_nCount;
		_ASSERT( i >= 0 && i < GetSize() );
		return m_arrElements[i];
	}

	void RemoveAt( int i )
	{
		_ASSERT( i < GetSize() );
		for ( ; i < GetSize()-1; i++ )
			m_arrElements[i] = m_arrElements[i+1];
		Resize(-1);
	}

	void InsertAt( int i, const TYPE& element )
	{
		int nSize = GetSize();
		_ASSERT( i <= nSize );
		Resize(+1);

		for ( int j = nSize-1; j >= i; j-- )
			m_arrElements[j+1] = m_arrElements[j];
		m_arrElements[i] = element;
	}

	void RemoveAll()
	{
		m_nCount = 0;
	}

	void Sort(TCompareFunction fCompare)
	{
		for ( int i=0; i<m_nCount; i++)
			for ( int j=i+1; j<m_nCount; j++)
				if ( fCompare( m_arrElements[i], m_arrElements[j] ) < 0 ) 
				{
					TYPE tTemp = m_arrElements[i];
					m_arrElements[i] = m_arrElements[j];
					m_arrElements[j] = tTemp;
				}
	}
    
    int Find(TYPE& value)
    {
        for (int i=0; i<m_nCount; i++)
            if (m_arrElements[i] == value)
                return i;
        return -1;
    }

	TYPE* GetData()
	{
		return m_arrElements;
	}

    void Copy(const CArray<TYPE>& source)
    {
        SetSize(source.GetSize());
        for (int i=0; i<source.GetSize(); i++)
            m_arrElements[i] = source.m_arrElements[i];
    }
};
