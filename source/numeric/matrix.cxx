/****************************************************************************
 * Copyright (c) 2005-2009 Kohei Yoshida
 * 
 * This code is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3 only,
 * as published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License version 3 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with this work.  If not, see
 * <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "numeric/matrix.hxx"
#include "tool/global.hxx"
#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <exception>
#include <iterator>
#include <sstream>

#define USE_BOOST_1_30_2 1

#if USE_BOOST_1_30_2
#ifdef __GNUC__
#warning "noalias not present in boost 1.30.2"
#endif
#undef noalias
#define noalias(a) (a)
#endif

namespace bnu = ::boost::numeric::ublas;

using ::std::cout;
using ::std::endl;
using ::boost::numeric::ublas::prod;
using ::std::vector;

namespace scsolver { namespace numeric {

const char* BadIndex::what() const throw()
{
    return "Bad index";
}

const char* MatrixSizeMismatch::what() const throw()
{
    return "Matrix size mismatch";
}

const char* MatrixNotDiagonal::what() const throw()
{
    return "Matrix not diagonal";
}

const char* OperationOnEmptyMatrix::what() const throw()
{
    return "Operation on empty matrix";
}

const char* SingularMatrix::what() const throw()
{
    return "Singular matrix encountered";
}

const char* NonSquareMatrix::what() const throw()
{
    return "Matrix not square where a square matrix is required";
}

//---------------------------------------------------------------------------
// Local helper functions

namespace mxhelper {

#ifdef DEBUG
void print( const bnu::matrix<double>& mx )
{
    for ( size_t i = 0; i < mx.size1(); ++i )
    {
        printf( "|" );
        for ( size_t j = 0; j < mx.size2(); ++j )
            printf( " %f", mx( i, j ) );
        printf( " |\n" );
    }
}
#else
void print( const bnu::matrix<double>& )
{
}
#endif

typedef bnu::matrix_range< bnu::matrix<double> >  MxRange;
typedef bnu::matrix_row< bnu::matrix<double> >    MxRow;
typedef bnu::matrix_column< bnu::matrix<double> > MxColumn;

bool isRowEmpty( const bnu::matrix<double>& A, size_t nRowId )
{
    if ( nRowId >= A.size1() )
        throw MatrixSizeMismatch();

    size_t nColSize = A.size2();
    for ( size_t i = 0; i < nColSize; ++i )
        if ( A( nRowId, i ) )
            return false;

    return true;
}

bool isColumnEmpty( const bnu::matrix<double>& A, size_t nColId )
{
    if ( nColId >= A.size2() )
        throw MatrixSizeMismatch();

    size_t nRowSize = A.size1();
    for ( size_t i = 0; i < nRowSize; ++i )
        if ( A( i, nColId ) )
            return false;

    return true;
}

void swapRows( bnu::matrix<double>& A, size_t nRow1, size_t nRow2 )
{
    MxRow row1( A, nRow1 );
    MxRow row2( A, nRow2 );
    row1.swap( row2 );
}

void factorize( const bnu::matrix<double>& mxA, bnu::matrix<double>& mxL, 
        bnu::matrix<double>& mxU, bnu::matrix<double>& mxP )
{
    if ( mxA.size1() != mxA.size2() )
        throw NonSquareMatrix();

#if 0
    size_t n = mxA.size1();
    bnu::matrix<double> A( mxA );
    bnu::identity_matrix<double> I( A.size1(), A.size2() );
    bnu::matrix<double> P( I );

    for ( unsigned int k = 0; k < n; ++k )
    {
        // Partial pivoting ( get column k and scan row(s) k:n )
        if ( n - k > 1 )
        {
            MxColumn cl( A, k );
            for ( unsigned int i = k + 1; i < n; ++i )
            {
                size_t nSwapRowId = k;
                double fVal = cl( k );
                for ( size_t nRowId = k; nRowId < n; ++nRowId )
                {
                    double fTemp = cl( nRowId );
                    if ( fTemp > fVal )
                    {
                        nSwapRowId = nRowId;
                        fVal = fTemp;
                    }
                }

                if ( nSwapRowId != k )
                {
                    swapRows( A, k, nSwapRowId );
                    swapRows( P, k, nSwapRowId );
                }   
            }
        }

        double fPivot = A( k, k );
        if ( fPivot == 0.0 )
            throw SingularMatrix();

        MxRange mr1( A, bnu::range( k + 1, n ), bnu::range( k, k + 1 ) );
        mr1 /= fPivot;
        MxRange mr2( A, bnu::range( k + 1, n ), bnu::range( k, k + 1 ) );
        MxRange mr3( A, bnu::range( k, k + 1 ), bnu::range( k + 1, n ) );
        MxRange mr4( A, bnu::range( k + 1, n ), bnu::range( k + 1, n ) );
        mr4 -= prod( mr2, mr3 );
    }
    
    // Transfer elements from A into L and U
    bnu::matrix<double> L( I ), U( n, n );
    for ( unsigned int i = 0; i < n; ++i )
        for ( unsigned int j = 0; j < n; ++j )
        {
            if ( i > j )
                L( i, j ) = A( i, j );
            else
                U( i, j ) = A( i, j );
        }
#endif

    size_t n = mxA.size1();
    bnu::matrix<double> A( mxA );

    ::std::vector<size_t> cnP;
    cnP.reserve( n );
    for ( size_t i = 0; i < n; ++i )
        cnP.push_back( i );

    // for each column k...
    for ( size_t k = 0; k < n - 1; ++k )
    {
        double fMax = 0.0;
        size_t nSwapRowId = k;
        for ( size_t nRowId = k; nRowId < n; ++nRowId )
        {
            double fVal = fabs( A( nRowId, k ) );
            if ( fVal > fMax )
            {
                fMax = fVal;
                nSwapRowId = nRowId;
            }
        }

        if ( fMax == 0.0 )
            throw SingularMatrix();
            
        ::std::swap( cnP.at( k ), cnP.at( nSwapRowId ) );
        swapRows( A, k, nSwapRowId );
        
        for ( size_t i = k + 1; i < n; ++i )
        {
            A( i, k ) /= A( k, k );
            for ( size_t j = k + 1; j < n; ++j )
                A( i, j ) -= A( i, k )*A( k, j );
        }
    }

    // Transfer elements from A into L and U
    bnu::identity_matrix<double> I( n, n );
    bnu::zero_matrix<double> N( n, n );
    bnu::matrix<double> L( I ), U( N ), P( N );

    for ( unsigned int i = 0; i < n; ++i )
        for ( unsigned int j = 0; j < n; ++j )
        {
            if ( i > j )
                L( i, j ) = A( i, j );
            else
                U( i, j ) = A( i, j );
        }

#if REWRITE_FOR_SUN_STUDIO_COMPILER
    n = cnP.size();
    for (size_t i = 0; i < n; ++i)
        P(i, cnP[i]) = 1.0;
#else
    ::std::vector<size_t>::const_iterator it, itBeg = cnP.begin(), itEnd = cnP.end();
    for ( it = itBeg; it != itEnd; ++it )
        P( ::std::distance( itBeg, it ), *it ) = 1.0;
#endif

    noalias( mxU ) = U;
    noalias( mxL ) = L;
    noalias( mxP ) = P;
}

void inverseU( const bnu::matrix<double>& mxU, bnu::matrix<double>& mxUInv )
{
    size_t nSize = mxU.size1();
    if ( nSize != mxU.size2() )
        throw NonSquareMatrix();

    bnu::matrix<double> Combo( nSize, nSize*2 );
    MxRange U( Combo, bnu::range( 0, nSize ), bnu::range( 0, nSize ) );
    MxRange UInv( Combo, bnu::range( 0, nSize ), bnu::range( nSize, nSize*2 ) );
    bnu::identity_matrix<double> I( nSize );
    U = mxU;
    UInv = I;
    
    // back substitution
    size_t nRow = nSize;
    do
    {
        // normalize the current row first
        --nRow;
        double fPivot = U( nRow, nRow );
        MxRow mrCur( Combo, nRow );
        mrCur /= fPivot;
        U( nRow, nRow ) = 1.0; // maybe redundant because it should already be 1.0
        
        // substitute upward
        if ( nRow > 0 )
        {
            size_t nSubRow = nRow;
            do
            {
                MxRow mrSub( Combo, --nSubRow );
                mrSub -= mrCur*U( nSubRow, nRow );
            }
            while ( nSubRow != 0 );
        }   
    }
    while ( nRow != 0 );
    
    noalias( mxUInv ) = UInv;
}

void inverseL( const bnu::matrix<double>& mxL, bnu::matrix<double>& mxLInv )
{
    size_t nSize = mxL.size1();
    if ( nSize != mxL.size2() )
        throw NonSquareMatrix();

    bnu::matrix<double> Combo( nSize, nSize*2 );
    MxRange L( Combo, bnu::range( 0, nSize ), bnu::range( 0, nSize ) );
    MxRange LInv( Combo, bnu::range( 0, nSize ), bnu::range( nSize, nSize*2 ) );
    bnu::identity_matrix<double> I( nSize );
    L = mxL;
    LInv = I;

    // forward substitution
    size_t nRow = 0;
    do
    {
        MxRow mrCur( Combo, nRow );
        
        // substitute downward
        if ( nRow < nSize - 1 )
        {
            size_t nSubRow = nRow + 1;
            do
            {
                MxRow mrSub( Combo, nSubRow );
                mrSub -= mrCur*L( nSubRow, nRow );
            }
            while ( ++nSubRow < nSize );
        }
    }
    while ( ++nRow < nSize );
    
    noalias( mxLInv ) = LInv;
}

/** Computes an inverse matrix of an arbitrary matrix.

    @param mxA original matrix
    @param mxInv inverse matrix ( reference variable )
 */
void inverse( const bnu::matrix<double>& mxA, bnu::matrix<double>& mxInv )
{
    size_t nRow = mxA.size1(), nCol = mxA.size2();
    if ( nRow != nCol )
        throw NonSquareMatrix();
        
    size_t nSize = nRow;
        
    bnu::matrix<double> mxL( nSize, nSize );
    bnu::matrix<double> mxU( nSize, nSize );
    bnu::matrix<double> mxP( nSize, nSize );
    factorize( mxA, mxL, mxU, mxP );
    
    bnu::matrix<double> mxUInv( nSize, nSize );
    inverseU( mxU, mxUInv );
    
    bnu::matrix<double> mxLInv( nSize, nSize );
    inverseL( mxL, mxLInv );
    
    bnu::matrix<double> mxTemp = prod( mxUInv, mxLInv );
    mxTemp = prod( mxTemp, mxP );
    noalias( mxInv ) = mxTemp;
}

} // end of namespace mxhelper

//---------------------------------------------------------------------------

Matrix::Matrix() : 
    m_bResizable(true)
{
    bnu::matrix<double> m( 0, 0 );
    m_aArray = m;
}

Matrix::Matrix(size_t row, size_t col, bool identity_matrix) :
    m_bResizable(true)
{   
    bnu::matrix<double> m(row, col);
    for ( unsigned int i = 0; i < m.size1(); ++i )
    {
        for ( unsigned int j = 0; j < m.size2(); ++j )
        {
            if (identity_matrix && i == j)
                m( i, j ) = 1.0;
            else
                m( i, j ) = 0.0;
        }
    }
    m_aArray = m;
}

Matrix::Matrix( const Matrix& other ) :
    m_bResizable( other.m_bResizable ),
    m_aArray( other.m_aArray )
{
}

Matrix::Matrix( const Matrix* p ) :
    m_bResizable( p->m_bResizable ),
    m_aArray( p->m_aArray )
{
}

Matrix::Matrix( bnu::matrix<double> m ) : m_bResizable( true )
{
    m_aArray = m;
}

Matrix::~Matrix() throw()
{
}

void Matrix::setResizable(bool resizable)
{ 
    m_bResizable = resizable; 
}

void Matrix::swap( Matrix& other ) throw()
{
    m_aArray.swap( other.m_aArray );
    std::swap( m_bResizable, other.m_bResizable );
}

void Matrix::clear()
{
    m_aArray.resize( 0, 0 );
}

void Matrix::copy( const Matrix& other )
{
    Matrix tmp( other );
    swap( tmp );
}

Matrix Matrix::clone() const
{
    Matrix mxCloned( this );
    return mxCloned;
}

const double Matrix::getValue(size_t row, size_t col) const
{
    try
    {
        return m_aArray(row, col);
    }
    catch (const ::boost::numeric::ublas::bad_index&)
    {
        throw BadIndex();
    }
}

double& Matrix::getValue(size_t row, size_t col)
{
    try
    {
        return m_aArray(row, col);
    }
    catch (const ::boost::numeric::ublas::bad_index&)
    {
        throw BadIndex();
    }
}

void Matrix::setValue(size_t row, size_t col, double val)
{
    maybeExpand(row, col);
    m_aArray(row, col) = val;
}

Matrix Matrix::getColumn(size_t col)
{
    size_t nRows = rows();
    Matrix m( nRows, 1 );
    m.setResizable( false );
    for ( size_t i = 0; i < nRows; ++i )
        m( i, 0 ) = getValue( i, col );
    return m;
}

Matrix Matrix::getRow(size_t row)
{
    size_t nCols = cols();
    Matrix mx( 1, nCols );
    mx.setResizable( false );
    for ( size_t i = 0; i < nCols; ++i )
        mx( 0, i ) = getValue( row, i );
    return mx;
}

void Matrix::deleteColumn( size_t nColId )
{
    if ( nColId >= m_aArray.size2() )
    {
        Debug( "deleteColumn" );
        throw MatrixSizeMismatch();
    }

    bnu::matrix<double> m( m_aArray.size1(), m_aArray.size2()-1 );
    for ( size_t i = 0; i < m_aArray.size1(); ++i )
        for ( size_t j = 0; j < m_aArray.size2(); ++j )
        {
            if ( j == nColId )
                continue;
            size_t j2 = j > nColId ? j - 1 : j;
            m( i, j2 ) = m_aArray( i, j );
        }
    m_aArray = m;
}

void Matrix::deleteColumns( const std::vector<size_t>& cnColIds )
{
    // reverse sorted set
    typedef std::set<size_t,std::greater<size_t> > uIntSet;
    
    uIntSet aSortedIds;
    std::vector<size_t>::const_iterator pos;
    for ( pos = cnColIds.begin(); pos != cnColIds.end(); ++pos )
        aSortedIds.insert( *pos );

    uIntSet::iterator pos2;
    for ( pos2 = aSortedIds.begin(); pos2 != aSortedIds.end(); ++pos2 )
        deleteColumn( *pos2 );
}

void Matrix::deleteRow( size_t nRowId )
{
    if ( nRowId >= m_aArray.size1() )
    {
        Debug( "deleteRow" );
        throw MatrixSizeMismatch();
    }

    bnu::matrix<double> m( m_aArray.size1()-1, m_aArray.size2() );
    for ( size_t i = 0; i < m_aArray.size1(); ++i )
        if ( i != nRowId )
        {
            size_t i2 = i > nRowId ? i - 1 : i;
            for ( size_t j = 0; j < m_aArray.size2(); ++j )
                m( i2, j ) = m_aArray( i, j );
        }
    m_aArray = m;
}

void Matrix::deleteRows( const std::vector<size_t>& cnRowIds )
{
    std::vector<size_t> cnRowIds2( cnRowIds.begin(), cnRowIds.end() );
    std::sort( cnRowIds2.begin(), cnRowIds2.end() );

    std::vector<size_t>::reverse_iterator it,
        itBeg = cnRowIds2.rbegin(), itEnd = cnRowIds2.rend();
    for ( it = itBeg; it != itEnd; ++it )
        deleteRow( *it );
}

const Matrix Matrix::adj() const
{
    throwIfEmpty();

    Matrix mxadj( cols(), rows() ); // transposed

    for ( size_t i = 0; i < mxadj.rows(); ++i )
        for ( size_t j = 0; j < mxadj.cols(); ++j )
            mxadj.setValue( i, j, cofactor( j, i ) );

    return mxadj;
}

double Matrix::cofactor( size_t i, size_t j ) const
{
    throwIfEmpty();

    double fVal = minors( i, j );
    if ( (i+j)%2 )
        fVal *= -1;

    return fVal;
}

double Matrix::det() const
{
    throwIfEmpty();
    
    if ( !isSquare() )
    {
        Debug( "is not square" );
        throw NonSquareMatrix();
    }

    if ( cols() == 1 )
        return getValue( 0, 0 );
    else if ( cols() == 2 )
        return getValue( 0, 0 )*getValue( 1, 1 ) - getValue( 0, 1 )*getValue( 1, 0 );
    else if ( cols() == 3 )
        return getValue( 0, 0 )*getValue( 1, 1 )*getValue( 2, 2 ) - 
            getValue( 0, 0 )*getValue( 1, 2 )*getValue( 2, 1 ) - 
            getValue( 1, 0 )*getValue( 0, 1 )*getValue( 2, 2 ) +
            getValue( 1, 0 )*getValue( 0, 2 )*getValue( 2, 1 ) +
            getValue( 2, 0 )*getValue( 0, 1 )*getValue( 1, 2 ) -
            getValue( 2, 0 )*getValue( 0, 2 )*getValue( 1, 1 );

    double fSum = 0.0;
    
    for ( size_t j = 0; j < cols(); ++j )
    {
        double fHead = getValue( 0, j );
        if ( fHead )
        {
            if ( j%2 )
                fHead *= -1.0;
            fHead *= minors( 0, j );
            fSum += fHead;
        }
    }
    return fSum;
}

const Matrix Matrix::inverse() const
{
    throwIfEmpty();
    if ( !isSquare() )
    {
        cout << "matrix size = ( " << rows() << ", " << cols() << " )" << endl;
        Debug( "inversion requires a square matrix" );
        throw MatrixSizeMismatch();
    }

    bnu::matrix<double> mxAInv( m_aArray.size1(), m_aArray.size2() );
    mxhelper::inverse( m_aArray, mxAInv );
    Matrix mxInv( mxAInv );
    mxInv.setResizable( m_bResizable );

    return mxInv;
}

const Matrix Matrix::trans() const
{
    throwIfEmpty();
    Matrix m( ::boost::numeric::ublas::trans( m_aArray ) );
    m.m_bResizable = m_bResizable;
    return m;
}

double Matrix::minors( size_t iSkip, size_t jSkip ) const
{
    throwIfEmpty();
    Matrix m( rows() - 1, cols() - 1 );
    m.setResizable( false );
    for ( size_t i = 0; i < m.cols(); ++i )
    {
        size_t iOffset = i >= iSkip ? 1 : 0;
        for ( size_t j = 0; j < m.rows(); ++j )
        {
            size_t jOffset = j >= jSkip ? 1 : 0;
            double fVal = getValue( i + iOffset, j + jOffset );
            m.setValue( i, j, fVal );
        }
    }
    m.setResizable( true );
    return m.det();
}

void Matrix::resize(size_t row, size_t col)
{
#if USE_BOOST_1_30_2
#ifdef __GNUC__
#warning "using workaround for boost 1.30.2 which has a buggy resize"
#endif
    bnu::matrix<double, bnu::row_major, std::vector<double> > aArray(row, col);

    size_t nRowSize = m_aArray.size1() < row ? m_aArray.size1() : row;
    size_t nColSize = m_aArray.size2() < col ? m_aArray.size2() : col;
    for ( size_t nRowId = 0; nRowId < nRowSize; ++nRowId )
        for ( size_t nColId = 0; nColId < nColSize; ++nColId )
            aArray( nRowId, nColId ) = m_aArray( nRowId, nColId );
    
    m_aArray.swap( aArray );
#else
    // boost 1.30.2 has a bug in resize().  Works in 1.32.0.
    m_aArray.resize( row, col, true );
#endif
}

size_t Matrix::rows() const 
{ 
    return m_aArray.size1(); 
}

size_t Matrix::cols() const
{
    return m_aArray.size2();
}

bool Matrix::empty() const
{
    return ( rows() == 0 && cols() == 0 );
}

bool Matrix::isRowEmpty( size_t nRow ) const
{
    return mxhelper::isRowEmpty( m_aArray, nRow );
}

bool Matrix::isColumnEmpty( size_t nCol ) const
{
    return mxhelper::isColumnEmpty( m_aArray, nCol );
}

bool Matrix::isSameSize( const Matrix& r ) const
{
    return !( rows() != r.rows() || cols() != r.cols() );
}

bool Matrix::isSquare() const
{
    return rows() == cols();
}

//----------------------------------------------------------------------------
// Overloaded Operators

Matrix& Matrix::operator=( const Matrix& r )
{
    // check for assignment to self
    if ( this == &r )
        return *this;

    Matrix temp( r );
    swap( temp );
    return *this;
}

const Matrix Matrix::operator+( const Matrix& r ) const
{
    Matrix m( this );
    if ( !m.isSameSize( r ) )
    {
        Debug( "addition of mis-matched matrices" );
        throw MatrixSizeMismatch();
    }
    
    for ( size_t i = 0; i < m.rows(); ++i )
        for ( size_t j = 0; j < m.cols(); ++j )
            m.setValue( i, j, m.getValue( i, j ) + r.getValue( i, j ) );
    return m;
}

const Matrix Matrix::operator-( const Matrix& r ) const
{
    Matrix m( this );
    if ( !m.isSameSize( r ) )
    {
        Debug( "subtraction of mis-matched matrices" );
        throw MatrixSizeMismatch();
    }
    
    for ( size_t i = 0; i < m.rows(); ++i )
        for ( size_t j = 0; j < m.cols(); ++j )
            m.setValue( i, j, m.getValue( i, j ) - r.getValue( i, j ) );
    return m;
}

const Matrix Matrix::operator*( double fMul ) const
{
    Matrix m( this );
    for ( size_t i = 0; i < m.rows(); ++i )
        for ( size_t j = 0; j < m.cols(); ++j )
            m.setValue( i, j, m.getValue( i, j )*fMul );
    return m;
}

const Matrix Matrix::operator*( const Matrix& r ) const
{
    if ( cols() != r.rows() )
    {
        Debug( "illegal multiplication" );
        throw MatrixSizeMismatch();
    }
    
    Matrix m( prod( m_aArray, r.m_aArray ) );
    return m;
}

const Matrix Matrix::operator/( double fDiv ) const
{
    Matrix m( this );
    for ( size_t i = 0; i < m.rows(); ++i )
        for ( size_t j = 0; j < m.cols(); ++j )
            m.setValue( i, j, m.getValue( i, j )/fDiv );
    return m;
}

Matrix& Matrix::operator+=( const Matrix& r )
{
    *this = *this + r;
    return *this;
}

Matrix& Matrix::operator+=( double f )
{
    Matrix mx( this );
    for ( size_t i = 0; i < mx.rows(); ++i )
        for ( size_t j = 0; j < mx.cols(); ++j )
            mx.setValue( i, j, mx.getValue( i, j ) + f );
    swap( mx );
    return *this;
}

Matrix& Matrix::operator-=( const Matrix& r )
{
    *this = *this - r;
    return *this;
}

Matrix& Matrix::operator*=( double f )
{
    *this = *this * f;
    return *this;
}

Matrix& Matrix::operator/=( double f )
{
    *this = *this / f;
    return *this;
}

const double Matrix::operator()(size_t row, size_t col) const
{
    return getValue(row, col);
}

double& Matrix::operator()(size_t row, size_t col)
{
    maybeExpand(row, col);
    return getValue(row, col);
}

bool Matrix::operator==( const Matrix& other ) const
{
    return !operator!=( other );
}

bool Matrix::operator!=( const Matrix& other ) const
{
    if ( cols() != other.cols() || rows() != other.rows() )
        return true;

    for ( size_t i = 0; i < rows(); ++i )
        for ( size_t j = 0; j < cols(); ++j )
            if ( getValue( i, j ) != other( i, j ) )
                return true;
    return false;
}

void Matrix::maybeExpand(size_t row, size_t col)
{
    if ( m_bResizable )
    {
        size_t nRowSize = m_aArray.size1();
        size_t nColSize = m_aArray.size2();
        if ( row >= nRowSize || col >= nColSize )
        {
            size_t nNewRowSize = row + 1 > nRowSize ? row + 1 : nRowSize;
            size_t nNewColSize = col + 1 > nColSize ? col + 1 : nColSize;
            resize( nNewRowSize, nNewColSize );
        }
    }
}

void Matrix::throwIfEmpty() const
{
    if ( empty() )
        throw OperationOnEmptyMatrix();
}

Matrix::StringMatrixType Matrix::getDisplayElements( 
        int prec, size_t nColSpace, bool bFormula) const
{
    using std::string;
    using std::ostringstream;

    // Set all column widths to 0.
    std::vector<unsigned int> aColLen;
    for ( unsigned int j = 0; j < m_aArray.size2(); ++j )
        aColLen.push_back( 0 );
    
    // Get string matrix.
    StringMatrixType mxElements( m_aArray.size1(), m_aArray.size2() );
    for ( unsigned int i = 0; i < m_aArray.size1(); ++i )
        for ( unsigned int j = 0; j < m_aArray.size2(); ++j )
        {
            ::std::ostringstream osElem;
            double fVal = m_aArray( i, j );
            for ( unsigned int k = 0; k < nColSpace; ++k )
                osElem << " ";
            if ( prec > 0 )
                osElem << std::showpoint;
            osElem << std::fixed << std::setprecision( prec ) << fVal;
            mxElements( i, j ) = osElem.str();
            if ( aColLen.at( j ) < osElem.str().size() )
                aColLen[j] = osElem.str().size();
        }

    // Adjust column widths
    for ( unsigned int i = 0; i < mxElements.size1(); ++i )
        for ( unsigned int j = 0; j < mxElements.size2(); ++j )
        {
            unsigned int nLen = mxElements( i, j ).size();
            if ( aColLen.at( j ) > nLen )
            {
                std::string sSpace;
                for ( unsigned int k = 0; k < (aColLen.at( j ) - nLen); ++k )
                    sSpace += " ";
                mxElements( i, j ) = sSpace + mxElements( i, j );
            }
        }
    
    if ( !bFormula )
        return mxElements;
    
    // Process elements for constraint formula display.
    for ( unsigned int i = 0; i < mxElements.size1(); ++i )
    {
        for ( unsigned int j = 0; j < mxElements.size2(); ++j )
        {
            double fVal = m_aArray( i, j );
            string sElem = mxElements( i, j );
            if ( fVal == 1.0 )
            {
                string::size_type nPos = sElem.find_first_of( "1" );
                if ( nPos != string::npos )
                {
                    sElem[nPos] = ' ';
                    mxElements( i, j ) = sElem;
                }
            }
            else if ( fVal == -1.0 )
            {
                // find "-1"
                string::size_type nPos = sElem.find_first_of( "-" );
                string::size_type nPos2 = nPos + 1;
                if ( nPos != string::npos && nPos2 != string::npos && sElem[nPos2] == '1' )
                {
                    sElem[nPos]  = ' ';
                    sElem[nPos2] = '-';
                    mxElements( i, j ) = sElem;
                }
            }
                
            if ( j != 0 )
            {
                string sElem2 = mxElements( i, j );
                ostringstream os;
                os << repeatString( " ", nColSpace );
                if ( fVal >= 0 )
                    os << "+" << sElem2.substr(1);
                else
                {
                    string::size_type nPos = sElem2.find_first_of( "-", 0 );
                    if ( nPos != string::npos )
                        sElem2[nPos] = ' ';
                    os << "-" << sElem.substr(1);
                }
                mxElements( i, j ) = os.str();
            }
        }
    }
    return mxElements;
}

void Matrix::print(size_t prec, size_t colspace) const
{
    using std::string;
    using std::ostringstream;

    // Print to stdout
    ostringstream os;
    const bnu::matrix< string > mElements = getDisplayElements(prec, colspace, false);
    for ( unsigned int i = 0; i < mElements.size1(); ++i )
    {
        os << "|";
        for ( unsigned int j = 0; j < mElements.size2(); ++j )
        {
            std::string s = mElements( i, j );
            os << s;
        }
        os << repeatString( " ", colspace ) << "|" << endl;
    }
    cout << os.str();
}

// ----------------------------------------------------------------------------
// Non-member functions

const Matrix operator+(const Matrix& mx, double scalar)
{
    Matrix tmp(mx);
    tmp += scalar;
    return tmp;
}

const Matrix operator+(double scalar, const Matrix& mx)
{
    return mx + scalar;
}

const Matrix operator*(double scalar, const Matrix& mx)
{
    return mx * scalar;
}

}}

