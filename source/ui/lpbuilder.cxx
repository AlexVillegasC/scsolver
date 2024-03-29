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

#include <algorithm>
#include <exception>

#include "lpbuilder.hxx"
#include "tool/global.hxx"
#include "unoglobal.hxx"
#include "type.hxx"
#include "numeric/type.hxx"
#include "numeric/lpmodel.hxx"
#include "numeric/matrix.hxx"

using namespace ::com::sun::star;
using com::sun::star::table::CellAddress;
using scsolver::numeric::Matrix;
using ::std::vector;
using ::std::cout;
using ::std::endl;
using ::std::distance;

namespace scsolver {

class NoMatchingElementsFound : public ::std::exception {};
class LogicError : public ::std::exception
{
public:
    virtual const char* what() const throw()
    {
        return "Logic Error";
    }
};

//---------------------------------------------------------------------------
// Local entities

bool operator==( const CellAddress& lhs, const CellAddress& rhs )
{
    if ( lhs.Sheet != rhs.Sheet || lhs.Column != rhs.Column || lhs.Row != rhs.Row )
        return false;
    return true;
}

bool operator!=( const CellAddress& lhs, const CellAddress& rhs )
{
    return !( lhs == rhs );
}

struct DecisionVar
{
    CellAddress Address;
    double Cost;
};

/** Used to hold a set of temporary cell attributes during constraint 
    parsing. */
struct CellAttr
{
    CellAddress Address;
    rtl::OUString Formula;
};


//---------------------------------------------------------------------------
// ConstraintAddress

ConstraintAddress::ConstraintAddress() :
    m_bIsLHSNumber(false),
    m_bIsRHSNumber(false),
    m_fLHSValue(0.0),
    m_fRHSValue(0.0)
{
}

ConstraintAddress::ConstraintAddress( const ConstraintAddress& aOther ) :
    Left( aOther.Left ), Right( aOther.Right ), Equal( aOther.Equal ),
    m_bIsLHSNumber( aOther.m_bIsLHSNumber ), m_bIsRHSNumber( aOther.m_bIsRHSNumber ),
    m_fLHSValue( aOther.m_fLHSValue ), m_fRHSValue( aOther.m_fRHSValue )
{
}

ConstraintAddress::~ConstraintAddress() throw()
{
}

bool ConstraintAddress::equals( const ConstraintAddress& aOther ) const
{
    if (m_bIsLHSNumber)
    {
        if (!aOther.m_bIsLHSNumber || aOther.m_fLHSValue != m_fLHSValue)
            return false;
    }
    else
    {
        // LHS is not a number.  Compare the cell addresses.
        if (aOther.m_bIsLHSNumber || aOther.Left.Sheet != Left.Sheet ||
            aOther.Left.Column != Left.Column || aOther.Left.Row != Left.Row)
            return false;
    }

    if (m_bIsRHSNumber)
    {
        if (!aOther.m_bIsRHSNumber || aOther.m_fRHSValue != m_fRHSValue)
            return false;
    }
    else
    {
        // RHS is not a number.  Compare the cell addresses.
        if (aOther.m_bIsRHSNumber || aOther.Right.Sheet != Right.Sheet ||
            aOther.Right.Column != Right.Column || aOther.Right.Row != Right.Row)
            return false;
    }
    return true;
}

bool ConstraintAddress::operator==( const ConstraintAddress& aOther ) const
{
    return equals( aOther );
}

table::CellAddress ConstraintAddress::getLeftCellAddr() const
{
    return Left;
}

void ConstraintAddress::setLeftCellAddr( const table::CellAddress& addr )
{
    Left = addr;
    m_bIsLHSNumber = false;
}

table::CellAddress ConstraintAddress::getRightCellAddr() const
{
    return Right;
}

void ConstraintAddress::setRightCellAddr( const table::CellAddress& addr )
{
    Right = addr;
    m_bIsRHSNumber = false;
}

double ConstraintAddress::getLeftCellValue() const
{
    return m_fLHSValue;
}

double ConstraintAddress::getRightCellValue() const
{
    return m_fRHSValue;
}

void ConstraintAddress::setLeftCellValue( double value )
{
    m_fLHSValue = value;
    m_bIsLHSNumber = true;
}

void ConstraintAddress::setRightCellValue( double value )
{
    m_fRHSValue = value;
    m_bIsRHSNumber = true;
}

bool ConstraintAddress::isLeftCellNumeric() const
{
    return m_bIsLHSNumber;
}

bool ConstraintAddress::isRightCellNumeric() const
{
    return m_bIsRHSNumber;
}

numeric::EqualityType ConstraintAddress::getEquality() const
{
    return Equal;
}

void ConstraintAddress::setEquality( numeric::EqualityType eq )
{
    Equal = eq;
}


//---------------------------------------------------------------------------
// LpModelBuilderImpl

/** The parent of this class (class LpModelBuilder) is to be instantiated by 
    an object of class SolveModel.  It builds up necessary model parameters
    via its member functions which are expected to be called from SolveModel.
    It then instantiates an object of class numeric::lp::Model
    based on those parameters when getModel() is called.
    
    In other words, this class acts as a liason between the model in a 
    spreadsheet form and the model in a form of class lp::Model.
 */
class LpModelBuilderImpl
{
public:
    LpModelBuilderImpl();
    ~LpModelBuilderImpl() throw();

    numeric::lp::Model getModel();

    numeric::GoalType getGoal() const;
    void setGoal( numeric::GoalType );

    double getSolveToValue() const;
    void setSolveToValue(double val);

    // Objective Formula
    CellAddress getObjectiveFormulaAddress() const;
    void setObjectiveFormulaAddress( const table::CellAddress& );

    // Constraints  
    sal_uInt32 getConstraintId( const ConstraintAddress& );
    void setConstraintAddress( const ConstraintAddress& );
    std::vector< ConstraintAddress > getAllConstraintAddresses() const { return m_cnConstraintAddress; }
    void setConstraintMatrixSize( size_t, size_t );
    void setConstraintCoefficient( const CellAddress&, const ConstraintAddress&, double, double );
    void clearConstraintAddresses() { m_cnConstraintAddress.clear(); }
    numeric::EqualityType getConstraintEquality( sal_uInt32 ) const;

    sal_uInt32 getDecisionVarId( const CellAddress& );
    void setDecisionVarAddress( const CellAddress& );
    vector< CellAddress > getAllDecisionVarAddresses() const;
    void clearDecisionVarAddresses() { m_cnDecisionVars.clear(); }

    double getCostVector( const CellAddress& );
    void setCostVector( const CellAddress&, double );
    
    const rtl::OUString getTempCellFormula( const CellAddress& ) const;
    void setTempCellFormula( const table::CellAddress&, const rtl::OUString& );

    void stripConstConstraint();
    void stripZeroCostDecisionVar();

private:

    numeric::GoalType m_eGoal;
    double m_solveToValue;

    // Objective Formula & Decision Variables
    table::CellAddress m_aObjFormulaAddr;
    std::vector< DecisionVar > m_cnDecisionVars;
    std::vector< ConstraintAddress > m_cnConstraintAddress;

    // Constraint Matrix
    Matrix m_mxConstraint;
    Matrix m_mxRHS;

    // Temporary Cell Formula Container used for recovery of cell values 
    // after modifying the cell.
    std::vector< CellAttr > m_cnCellAttrs;
};

LpModelBuilderImpl::LpModelBuilderImpl() :
    m_eGoal( numeric::GOAL_UNKNOWN ), 
    m_solveToValue(0.0),
    m_mxConstraint(0, 0), 
    m_mxRHS(0, 0)
{
}

LpModelBuilderImpl::~LpModelBuilderImpl() throw()
{
}

/** Construct an object of class numeric::lp::Model based on the 
    related parameters derived prior to calling this method, and 
    return it to the caller when it's successfully created. */
numeric::lp::Model LpModelBuilderImpl::getModel()
{
    //stripZeroCostDecisionVar();
    //stripConstConstraint();

    numeric::lp::Model aModel;
    
    vector< DecisionVar >::const_iterator it,
            itBeg = m_cnDecisionVars.begin(), itEnd = m_cnDecisionVars.end();
    for ( it = itBeg; it != itEnd; ++it )
        aModel.setCostVectorElement( distance( itBeg, it ), (*it).Cost );
    
    // Constraint matrix, equality, and RHS
    for ( sal_uInt32 i = 0; i < m_mxConstraint.rows(); ++i )
    {
        vector<double> aConst;
        for ( sal_uInt32 j = 0; j < m_mxConstraint.cols(); ++j )
            aConst.push_back( m_mxConstraint( i, j ) );

        numeric::EqualityType e = getConstraintEquality( i );
        aModel.addConstraint( aConst, e, m_mxRHS( i, 0 ) );
    }

    aModel.setGoal( getGoal() );
    aModel.setSolveToValue(getSolveToValue());
    return aModel;
}

numeric::GoalType LpModelBuilderImpl::getGoal() const
{
    return m_eGoal;
}

void LpModelBuilderImpl::setGoal( numeric::GoalType e )
{
    m_eGoal = e;
}

double LpModelBuilderImpl::getSolveToValue() const
{
    return m_solveToValue;
}

void LpModelBuilderImpl::setSolveToValue(double val)
{
    m_solveToValue = val;
}

CellAddress LpModelBuilderImpl::getObjectiveFormulaAddress() const
{ 
    return m_aObjFormulaAddr;
}

void LpModelBuilderImpl::setObjectiveFormulaAddress( const table::CellAddress& aAddr )
{
    m_aObjFormulaAddr = aAddr; 
}

sal_uInt32 LpModelBuilderImpl::getConstraintId( const ConstraintAddress& aConstAddr )
{
    vector< ConstraintAddress >::iterator pos;
    for ( pos = m_cnConstraintAddress.begin(); pos != m_cnConstraintAddress.end(); ++pos )
        if ( pos->equals( aConstAddr ) )
            return distance( m_cnConstraintAddress.begin(), pos );
    
    throw NoMatchingElementsFound();
}

void LpModelBuilderImpl::setConstraintAddress( const ConstraintAddress& aItem )
{
    m_cnConstraintAddress.push_back( aItem );
}

void LpModelBuilderImpl::setConstraintMatrixSize( size_t nRow, size_t nCol )
{
    m_mxConstraint.resize( nRow, nCol );
}

void LpModelBuilderImpl::setConstraintCoefficient( 
    const table::CellAddress& aCellAddr, const ConstraintAddress& aConstAddr, 
    double fCoef, double fRHS )
{
    // First, get the column ID of this coefficient from the address of a 
    // decision variable (aCellAddr).
    sal_uInt32 nColId = getDecisionVarId( aCellAddr );
    
    // Next, get this coefficient's row ID from ConstraintAddress.
    sal_uInt32 nRowId = getConstraintId( aConstAddr );
    
//  cout << "(" << nRowId << ", " << nColId << ") = " << fCoef << "  RHS = " << fRHS << endl;
    try
    {
        m_mxConstraint( nRowId, nColId ) = fCoef;
        m_mxRHS( nRowId, 0 ) = fRHS;
    }
    catch (const numeric::BadIndex& )
    {
        throw RuntimeError(ascii("Error assigning value to a matrix"));
    }
}

/** Returns a value of Equality enum by constraint ID.  A constraint ID 
    corresponds to an appropriate row ID of the constraint matrix.
 */
numeric::EqualityType LpModelBuilderImpl::getConstraintEquality( sal_uInt32 i ) const
{
    if ( m_cnConstraintAddress.size() > i )
        return m_cnConstraintAddress.at(i).getEquality();
    return numeric::EQUAL;
}

sal_uInt32 LpModelBuilderImpl::getDecisionVarId( const table::CellAddress& aAddr )
{
    vector< DecisionVar >::const_iterator it,
            itBeg = m_cnDecisionVars.begin(), itEnd = m_cnDecisionVars.end();
    for ( it = itBeg; it != itEnd; ++it )
    {
        CellAddress aAddrTmp = it->Address;
        if ( aAddrTmp == aAddr )
            return distance( itBeg, it );
    }

    throw NoMatchingElementsFound();
}

/** Append the address of a cell whose value represents the value of a decision
    variable.  Member variable m_cnDecisionVars contains their addresses in 
    sequential order (x_1, x_2, x_3, ... where each x is a decision variable). */
void LpModelBuilderImpl::setDecisionVarAddress( const table::CellAddress& aAddr )
{
    DecisionVar aVar;
    aVar.Address = aAddr;
    aVar.Cost = 0.0;
    m_cnDecisionVars.push_back( aVar );
}

vector< CellAddress > LpModelBuilderImpl::getAllDecisionVarAddresses() const
{
    vector< CellAddress > cnAddrs;
    vector< DecisionVar >::const_iterator it,
            itBeg = m_cnDecisionVars.begin(), itEnd = m_cnDecisionVars.end();
    for ( it = itBeg; it != itEnd; ++it )
        cnAddrs.push_back( it->Address );

    return cnAddrs;
}

/** Get a cost vector value associated with a given decision variable, whose
    value is in the cell passed on as the argument. */
double LpModelBuilderImpl::getCostVector( const table::CellAddress& aAddr )
{   
    vector< DecisionVar >::const_iterator it,
            itBeg = m_cnDecisionVars.begin(), itEnd = m_cnDecisionVars.end();
    for ( it = itBeg; it != itEnd; ++it )
    {
        DecisionVar aVar = *it;
        if ( aVar.Address == aAddr )
            return aVar.Cost;
    }
    
    // This should NOT be reached!
    throw NoMatchingElementsFound();
}

void LpModelBuilderImpl::setCostVector( const table::CellAddress& aAddr, double fCost )
{
    vector< DecisionVar >::iterator it,
            itBeg = m_cnDecisionVars.begin(), itEnd = m_cnDecisionVars.end();
    for ( it = itBeg; it != itEnd; ++it )
    {
        if ( it->Address == aAddr )
        {
            it->Cost = fCost;
            return;
        }
    }
    OSL_ASSERT( !"LogicError: no matching address found" );
}

const rtl::OUString LpModelBuilderImpl::getTempCellFormula( const table::CellAddress& aAddr ) const
{
    vector< CellAttr >::const_iterator it,
            itBeg = m_cnCellAttrs.begin(), itEnd = m_cnCellAttrs.end();

    for ( it = itBeg; it != itEnd; ++it )
        if( it->Address == aAddr )
            return it->Formula;

    rtl::OUString sEmpty;
    return sEmpty;
}

void LpModelBuilderImpl::setTempCellFormula( const table::CellAddress& aAddr, const rtl::OUString& sStr )
{
    CellAttr aCellAttr;
    aCellAttr.Address = aAddr;
    aCellAttr.Formula = sStr;
    m_cnCellAttrs.push_back( aCellAttr );
}

/** Remove constraint and right-hand-side row(s) if all elements of the 
    constraint row is zero and the constraint is already satisfied. */
void LpModelBuilderImpl::stripConstConstraint()
{
    using namespace numeric;

    Debug( "stripConstConstraint" );

    Matrix mxConstraint( m_mxConstraint ), mxRHS( m_mxRHS );
    OSL_ASSERT( mxConstraint.rows() == mxRHS.rows() );
    size_t nRowSize = mxConstraint.rows();

    vector<size_t> cnRowsToRemove;

    // Scan the constraint matrix to find empty rows.
    for ( size_t i = 0; i < nRowSize; ++i )
        if ( mxConstraint.isRowEmpty( i ) )
        {
            double fRHS = mxRHS( i, 0 );
            EqualityType eEq = getConstraintEquality( i );
            if ( ( fRHS <= 0 && eEq == GREATER_EQUAL ) ||
                 ( fRHS >= 0 && eEq == LESS_EQUAL ) ||
                 ( fRHS == 0.0 && eEq == EQUAL ) )
                cnRowsToRemove.push_back( i );
        }

    cout << "rows to remove: ";
    printElements( cnRowsToRemove );

    mxConstraint.deleteRows( cnRowsToRemove );
    mxRHS.deleteRows( cnRowsToRemove );

    m_mxConstraint.swap( mxConstraint );
    m_mxRHS.swap( mxRHS );
}

/** Remove decision variables and their corresponding constraint columns if 
    and only if they all have a zero cost coefficient and all elements in 
    their constraint column. */
void LpModelBuilderImpl::stripZeroCostDecisionVar()
{
    Debug( "stripZeroCostDecisionVar" );

    vector< DecisionVar > cnNewVars;
    Matrix mxConstraint( m_mxConstraint );
    cnNewVars.reserve( m_cnDecisionVars.size() );
    vector< size_t > cnColsToRemove;

    vector< DecisionVar >::iterator it,
            itBeg = m_cnDecisionVars.begin(), itEnd = m_cnDecisionVars.end();
    cout << m_cnDecisionVars.size() << endl;
    size_t nLastRow = mxConstraint.rows();
    for ( it = itBeg; it != itEnd; ++it )
    {
        if ( it->Cost )
            cnNewVars.push_back( *it );
        else
        {
            size_t nCol = distance( itBeg, it );
            if ( mxConstraint.isColumnEmpty( nCol ) )
                cnColsToRemove.push_back( nCol );
            else
                cnNewVars.push_back( *it );
        }
    }

    printElements( cnColsToRemove );
    cout << "mxConstraint:" << endl;
    mxConstraint.print( 0 );
    
    cout << "(" << nLastRow << ", " << mxConstraint.cols() << ")" << endl;
    
    mxConstraint.deleteColumns( cnColsToRemove );
    mxConstraint.print( 0 );
    swap( cnNewVars, m_cnDecisionVars );
    m_mxConstraint.swap( mxConstraint );
}

//---------------------------------------------------------------------------
// LpModelBuilder

LpModelBuilder::LpModelBuilder() : m_pImpl( new LpModelBuilderImpl() )
{
}

LpModelBuilder::~LpModelBuilder()
{
}

numeric::lp::Model LpModelBuilder::getModel()
{
    return m_pImpl->getModel();
}

numeric::GoalType LpModelBuilder::getGoal() const
{
    return m_pImpl->getGoal();
}

void LpModelBuilder::setGoal( numeric::GoalType e )
{
    m_pImpl->setGoal( e );
}

double LpModelBuilder::getSolveToValue() const
{
    return m_pImpl->getSolveToValue();
}

void LpModelBuilder::setSolveToValue(double val)
{
    m_pImpl->setSolveToValue(val);
}

const CellAddress LpModelBuilder::getObjectiveFormulaAddress() const
{
    return m_pImpl->getObjectiveFormulaAddress();
}

void LpModelBuilder::setObjectiveFormulaAddress( const table::CellAddress& aAddr )
{
    m_pImpl->setObjectiveFormulaAddress( aAddr );
}
    
double LpModelBuilder::getCostVector( const table::CellAddress& aAddr )
{
    return m_pImpl->getCostVector( aAddr );
}

void LpModelBuilder::setCostVector( const table::CellAddress& aAddr, double fCost )
{
    m_pImpl->setCostVector( aAddr, fCost );
}

void LpModelBuilder::clearConstraintAddresses()
{
    m_pImpl->clearConstraintAddresses();
}

void LpModelBuilder::setConstraintAddress( const ConstraintAddress& aItem )
{
    m_pImpl->setConstraintAddress( aItem );
}

vector< ConstraintAddress > LpModelBuilder::getAllConstraintAddresses() const
{
    return m_pImpl->getAllConstraintAddresses();
}

void LpModelBuilder::setConstraintMatrixSize( size_t nRow, size_t nCol )
{
    m_pImpl->setConstraintMatrixSize( nRow, nCol );
}

void LpModelBuilder::setConstraintCoefficient( 
    const CellAddress& aCellAddr, const ConstraintAddress& aConstAddr, 
    double fCoef, double fRHS )
{
    m_pImpl->setConstraintCoefficient( aCellAddr, aConstAddr, fCoef, fRHS );
}

void LpModelBuilder::setDecisionVarAddress( const table::CellAddress& aAddr )
{
    m_pImpl->setDecisionVarAddress( aAddr );
}

vector< CellAddress > LpModelBuilder::getAllDecisionVarAddresses() const
{
    return m_pImpl->getAllDecisionVarAddresses();
}

void LpModelBuilder::clearDecisionVarAddresses()
{
    return m_pImpl->clearDecisionVarAddresses();
}

const rtl::OUString LpModelBuilder::getTempCellFormula( const table::CellAddress& aAddr ) const
{
    return m_pImpl->getTempCellFormula( aAddr );
}

void LpModelBuilder::setTempCellFormula( const table::CellAddress& aAddr, const rtl::OUString& sStr )
{
    m_pImpl->setTempCellFormula( aAddr, sStr );
}


}

