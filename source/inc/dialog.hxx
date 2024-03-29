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
 
#ifndef _DIALOG_HXX_
#define _DIALOG_HXX_

#include <type.hxx>
#include <basedlg.hxx>
#include <unoglobal.hxx>
#include <numeric/type.hxx>
#include <memory>
#include <com/sun/star/awt/PushButtonType.hpp>
#include <com/sun/star/awt/Rectangle.hpp>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using scsolver::numeric::EqualityType;
using scsolver::numeric::GoalType;

namespace com { namespace sun { namespace star { namespace awt {

    class XTextComponent;

}}}}

namespace scsolver {

class SolverImpl;

class Listener;
class RngBtnListener;
class BaseDialog;
class WidgetProperty;
class MessageDialog;
class OptionDialog;

class SolverDialog;
class SolveBtnListener;
class CloseBtnListener;
class SaveBtnListener;
class LoadBtnListener;
class ResetBtnListener;
class OptionBtnListener;
class RadioBtnListener;
class ConstEditBtnListener;
class WindowFocusListener;
class WindowMouseListener;
class ConstListBoxListener;
class OKCancelBtnListener;
class ConstDlgCloseAction;
class SolverDlgCloseAction;

/**
 * Constraint input dialog.
 */
class ConstEditDialog : public BaseDialog
{
public:
    ConstEditDialog( SolverImpl* );
    virtual ~ConstEditDialog() throw();
    virtual void setVisible( bool );
    virtual bool doneRangeSelection() const;
    virtual void close();

    void initialize();
    void reset();
    
    sal_uInt32 getConstraintId() const { return m_nConstraintId; }
    rtl::OUString getLeftCellReference();
    rtl::OUString getRightCellReference();
    EqualityType getEquality();

    void setChangeMode( sal_Bool b ) { m_bIsChangeMode = b; }
    void setConstraintId( sal_uInt32 );
    
    const rtl::OUString getLeftCellReference() const;
    const rtl::OUString getRightCellReference() const;
    void setLeftCellReference( const rtl::OUString& );
    void setRightCellReference( const rtl::OUString& );

    void setEquality( const EqualityType );
    
    bool isChangeMode() const { return m_bIsChangeMode; }
    bool isCellRangeGeometryEqual() const;
    
protected:
    virtual void registerListeners();
    virtual void unregisterListeners();

private:
    bool m_bIsChangeMode;
    sal_uInt32 m_nConstraintId;

    OKCancelBtnListener* m_pOKListener;
    OKCancelBtnListener* m_pCancelListener;
    RngBtnListener* m_pLeftRngListener;
    RngBtnListener* m_pRightRngListener;
    TopWindowListener* m_pTopWindowListener;
    ConstDlgCloseAction* m_pCloseAction;
};

/**
 * Main solver dialog.
 */
class SolverDialog : public BaseDialog
{

public:
    
    SolverDialog( SolverImpl* );
    virtual ~SolverDialog() throw();
    virtual void setVisible( bool );
    virtual bool doneRangeSelection() const { return true; }
    virtual void close();

    void initialize();

    ConstEditDialog* getConstEditDialog();
    OptionDialog*    getOptionDialog();

    sal_Int16 getSelectedConstraintPos();

    Reference< awt::XTextComponent > getXTextComponentFromWidget( const rtl::OUString& ) const;
    
    rtl::OUString getTargetCellAddress() const;
    void setTargetCellAddress( const rtl::OUString& );
    
    rtl::OUString getVarCellAddress();
    void setVarCellAddress( const rtl::OUString& );
    
    GoalType getGoal() const;
    void setGoal( GoalType );

    /** 
     * Get a value specified in the 'Value Of' input field.
     */
    double getSolveToValue() const;
    
    void output();
    
    void editConstraint( const sal_uInt32, const rtl::OUString&, const rtl::OUString&, 
            const EqualityType );
    void getConstraint( const sal_uInt32, rtl::OUString&, rtl::OUString&, EqualityType& );
    std::vector< ConstraintString > getAllConstraints() const;
    void removeConstraint( const sal_uInt32 );
    void clearConstraints();
    
    void setConstraint( const rtl::OUString&, const rtl::OUString&, const EqualityType );

    void showMessage( const rtl::OUString& );
    void showSolutionInfeasible();
    void showSolutionFound();
    
    void reset();
    void saveModelToDocument();
    void loadModelFromDocument();

    void updateWidgets();

    void enableAllWidgets(bool enable);

protected:
    
    virtual void registerListeners();
    virtual void unregisterListeners();
    
private:
    
    void removeConstraintsFromListBox( sal_Int16 = 0, sal_Int16 = 0 );
    void setConstraintImpl( const rtl::OUString&, const rtl::OUString&, const EqualityType,
            const sal_Bool, const sal_uInt32 = 0 );

    std::auto_ptr<ConstEditDialog> m_pConstEditDialog;
    std::auto_ptr<OptionDialog>  m_pOptionDialog;

    // Action Listeners
    TopWindowListener* m_pTopWindowListener;
    RngBtnListener* m_pTargetRngListener;
    RngBtnListener* m_pVarCellsRngListener;
    SolveBtnListener* m_pSolveListener;
    CloseBtnListener* m_pOKListener;
    SaveBtnListener* m_pSaveListener;
    LoadBtnListener* m_pLoadListener;
    ResetBtnListener* m_pResetListener;
    OptionBtnListener* m_pOptionListener;
    ItemListener*       m_pBtnMaxListener;
    ItemListener*       m_pBtnMinListener;
    ItemListener*       m_pBtnToValueListener;
    ConstEditBtnListener* m_pConstAddListener;
    ConstEditBtnListener* m_pConstChangeListener;
    ConstEditBtnListener* m_pConstDeleteListener;
    ItemListener* m_pConstListBoxListener;
    SolverDlgCloseAction* m_pCloseAction;

    // Constraint
    std::vector< ConstraintString > m_aConstraint;
};

//--------------------------------------------------------------------------


}


#endif //_DIALOG_HXX_
