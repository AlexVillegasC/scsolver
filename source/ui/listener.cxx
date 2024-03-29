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

#include "listener.hxx"
#include "solver.hxx"
#include "dialog.hxx"
#include "unoglobal.hxx"
#include "tool/global.hxx"
#include "msgdlg.hxx"
#include "xcalc.hxx"
#include "optiondlg.hxx"

#include <memory>
#include <iostream>

#include <com/sun/star/awt/XTextComponent.hpp>

#include <com/sun/star/beans/XPropertySet.hpp>

#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/frame/XNotifyingDispatch.hpp>

#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/sheet/XRangeSelection.hpp>
#include <com/sun/star/sheet/XSpreadsheet.hpp>
#include <com/sun/star/table/XCell.hpp>
#include <com/sun/star/table/XCellRange.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <stdio.h>

namespace scsolver {

//---------------------------------------------------------------------------
// Class RngSelListener

RngSelListener::RngSelListener( BaseDialog* pDlg, RngBtnListener* pBtn, 
		const rtl::OUString& sEditName )
{
	m_pDlg = pDlg;
	m_pBtn = pBtn;
	m_sEditName = sEditName;
}

RngSelListener::~RngSelListener() throw()
{
}

void RngSelListener::disposing( const lang::EventObject& ) throw ( RuntimeException )
{
}
	
void RngSelListener::done( const sheet::RangeSelectionEvent& oEvt ) throw ( RuntimeException )
{
	// Since all instances of RngSelListener receive an event notification when
	// a cell selection is done, I need to make sure that this instance is the right 
	// instance for the event.
	if ( m_pBtn->isEventOwner() )
	{
		rtl::OUString sRange = oEvt.RangeDescriptor;	// Get cell range expression

		Reference< uno::XInterface > oRngEdit = m_pDlg->getWidgetByName( m_sEditName );
		Reference< awt::XTextComponent > xComp( oRngEdit, UNO_QUERY );
		rtl::OUString sRangeOld = xComp->getText();
		xComp->setText( sRange );

		if ( !m_pDlg->doneRangeSelection() )
			xComp->setText( sRangeOld );

		m_pDlg->setVisible( true );
		m_pBtn->setEventOwner( false );
	}
}

void RngSelListener::aborted( const sheet::RangeSelectionEvent& ) throw ( RuntimeException )
{
	Debug("RngSelListener::aborted");
	if (m_pBtn->isEventOwner())
	{
		m_pDlg->doneRangeSelection();
		m_pDlg->setVisible( true );
		m_pBtn->setEventOwner(false);
	}
}


//---------------------------------------------------------------------------
// Class RngBtnListener

RngBtnListener::RngBtnListener( BaseDialog* pDlg, 
		Reference< sheet::XRangeSelection > xRngSel, const rtl::OUString& sEditName )
		
		: ActionListener( pDlg ),
		
		m_xRngSel( xRngSel ),
		m_pRngSelListener( NULL ),
		m_sEditName( sEditName ),
		m_bEventOwner( false ),
		m_bSingleCell( false )
{
	registerRngSelListener();
}

RngBtnListener::~RngBtnListener() throw()
{
	if ( m_xRngSel.is() )
	{
		// It appears that by explicitly removing the selection listener
		// its instance is automatically delete'd, which means a manual delete 
		// of the listener after the line below will cause a crash.
		m_xRngSel->removeRangeSelectionListener( m_pRngSelListener );
	}
	else
		Debug( "m_xRngSel == NULL!" );
}

void RngBtnListener::disposing( const lang::EventObject& )
		throw ( RuntimeException )
{
}
	
void RngBtnListener::actionPerformed( const awt::ActionEvent& /*oActionEvt*/ ) 
	throw ( RuntimeException )
{
	if ( m_xRngSel != NULL )
	{
		uno::Sequence< beans::PropertyValue > aProp( 3 );
		uno::Any aValue;
		aValue <<= ascii_i18n( "Please select a range" );
		aProp[0].Name = ascii( "Title" );
		aProp[0].Value = aValue;
		aProp[1].Name = ascii( "CloseOnMouseRelease" );
		aValue <<= static_cast<sal_Bool>(false);
		aProp[1].Value = aValue;
		setEventOwner( true );

		m_xRngSel->startRangeSelection( aProp );
		
		// Do NOT set the dialog invisible before starting range selection!
		getDialog()->setVisible( false );

        if (dynamic_cast<ConstEditDialog*>(getDialog()))
		{
            // TODO: In the future, the dialogs should have the notion of 
            // parent-child relationship so that when a dialog starts a range
            // selection mode all of its parent dialogs are set invisible.

			SolverDialog* pMainDlg = getDialog()->getSolverImpl()->getMainDialog();
			pMainDlg->setVisible( false );
		}
	}
	else
		Debug( "Range selection interface NULL" );
}

void RngBtnListener::registerRngSelListener()
{
	if ( m_pRngSelListener == NULL && m_xRngSel != NULL )
	{
		m_pRngSelListener = new RngSelListener( getDialog(), this, m_sEditName );
		m_xRngSel->addRangeSelectionListener( m_pRngSelListener );
	}
}

//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Class SolveBtnListener

SolveBtnListener::SolveBtnListener( SolverDialog* pDlg ) : ActionListener( pDlg )
{
}

SolveBtnListener::~SolveBtnListener() throw()
{
}

void SolveBtnListener::disposing( const lang::EventObject& )
	throw ( RuntimeException )
{
}

void SolveBtnListener::actionPerformed( const awt::ActionEvent& )
	throw ( RuntimeException )
{
	getDialog()->getSolverImpl()->solveModel();
}


//---------------------------------------------------------------------------
// Class CloseBtnListener

CloseBtnListener::CloseBtnListener( BaseDialog* pDlg ) : ActionListener( pDlg )
{
}

CloseBtnListener::~CloseBtnListener() throw()
{
}

void CloseBtnListener::disposing( const lang::EventObject& )
	throw ( RuntimeException )
{
}
	
void CloseBtnListener::actionPerformed( const awt::ActionEvent& )
	throw ( RuntimeException )
{
	getDialog()->close();
}


//---------------------------------------------------------------------------
// Class SaveBtnListener

SaveBtnListener::SaveBtnListener( SolverDialog* pDlg ) : ActionListener( pDlg )
{
}

SaveBtnListener::~SaveBtnListener() throw()
{
}

void SaveBtnListener::disposing( const lang::EventObject& )
	throw ( RuntimeException )
{
}
	
void SaveBtnListener::actionPerformed( const awt::ActionEvent& )
	throw ( RuntimeException )
{
	getDialog()->getSolverImpl()->getMainDialog()->saveModelToDocument();
}


//---------------------------------------------------------------------------
// Class LoadBtnListener

LoadBtnListener::LoadBtnListener( SolverDialog* pDlg ) : ActionListener( pDlg )
{
}

LoadBtnListener::~LoadBtnListener() throw()
{
}

void LoadBtnListener::disposing( const lang::EventObject& )
	throw ( RuntimeException )
{
}
	
void LoadBtnListener::actionPerformed( const awt::ActionEvent& )
	throw ( RuntimeException )
{
	getDialog()->getSolverImpl()->getMainDialog()->loadModelFromDocument();
}


//---------------------------------------------------------------------------
// Class ResetBtnListener

ResetBtnListener::ResetBtnListener( SolverDialog* pDlg ) : ActionListener( pDlg ),
	m_pDlg( NULL )
{
}

ResetBtnListener::~ResetBtnListener() throw()
{
}

void ResetBtnListener::disposing( const lang::EventObject& )
	throw ( RuntimeException )
{
}
	
void ResetBtnListener::actionPerformed( const awt::ActionEvent& )
	throw ( RuntimeException )
{
	getDialog()->getSolverImpl()->getMainDialog()->reset();
}

//---------------------------------------------------------------------------

OptionBtnListener::OptionBtnListener( SolverDialog* pDlg ) : 
	ActionListener( pDlg )
{
}

OptionBtnListener::~OptionBtnListener() throw()
{
}

void OptionBtnListener::disposing( const lang::EventObject& )
	throw ( RuntimeException )
{
}
	
void OptionBtnListener::actionPerformed( const awt::ActionEvent& )
	throw ( RuntimeException )
{
	SolverImpl* p = getDialog()->getSolverImpl();
	OptionDialog* pDlg = p->getMainDialog()->getOptionDialog();
	pDlg->setModelType( p->getOptionData()->getModelType() );
	pDlg->setVisible(true);
}


//---------------------------------------------------------------------------
// Class ConstEditBtnListener

ConstEditBtnListener::ConstEditBtnListener( SolverDialog* pDlg, ConstButtonType eType ) :
		ActionListener( pDlg )
{
	m_eBtnType = eType;
}

ConstEditBtnListener::~ConstEditBtnListener() throw()
{
}

void ConstEditBtnListener::disposing( const lang::EventObject& )
	throw ( RuntimeException )
{
}
	
void ConstEditBtnListener::actionPerformed( const awt::ActionEvent& )
	throw ( RuntimeException )
{
	BaseDialog* pDlg = getDialog();
	
	ConstButtonType eType = getButtonType();
	SolverDialog* pMainDlg = pDlg->getSolverImpl()->getMainDialog();
	switch( eType )
	{
		case CONST_ADD:
		case CONST_CHANGE:
			{
				ConstEditDialog* pCE = pMainDlg->getConstEditDialog();
				if ( pCE != NULL )
				{
					// We need to show the dialog first then set the values, or
					// the equality combo box does not show proper item in first
					// invocation.
					pCE->setVisible( true );
					if ( eType == CONST_CHANGE )
					{
						// Set the selected constraint to the dialog.
						sal_Int16 nSel = pMainDlg->getSelectedConstraintPos();
						if (nSel < 0)
							// No item is selected.
							return;
						
						rtl::OUString sLeft, sRight;
						EqualityType eEq;
						pMainDlg->getConstraint( nSel, sLeft, sRight, eEq );
						pCE->setLeftCellReference( sLeft );
						pCE->setRightCellReference( sRight );
						pCE->setEquality( eEq );
						
						pCE->setChangeMode( true );
						pCE->setConstraintId( nSel );
					}
					else
					{
						OSL_ASSERT( eType == CONST_ADD );
						pCE->setChangeMode( false );
					}
				}
				else
					OSL_ASSERT( !"ConstEditDialog is NULL" );
			}
			break;
			
		case CONST_DELETE:
			{
				// Delete the selected constraint, and disable the "Change" and
				// "Delete" buttons.
			
				sal_Int16 nSel = pMainDlg->getSelectedConstraintPos();
				if (nSel < 0)
					// No item selected.
					return;
				pMainDlg->removeConstraint( nSel );
			}
			break;
	
		default:
			OSL_ASSERT( !"Wrong button type!" );
			break;
	}
	pMainDlg->updateWidgets();
}

#if 0
WindowFocusListener::WindowFocusListener( BaseDialog* pDlg ) : FocusListener( pDlg )
{
}

void WindowFocusListener::focusGained( const awt::FocusEvent& ) throw( RuntimeException )
{
	Debug( "focusGained" );
}

void WindowFocusListener::focusLost( const awt::FocusEvent& ) throw( RuntimeException )
{
	Debug( "focusLost" );
}

WindowMouseListener::WindowMouseListener( BaseDialog* pDlg ) : MouseListener( pDlg )
{
}

WindowMouseListener::~WindowMouseListener() throw()
{
}

void WindowMouseListener::mousePressed( const awt::MouseEvent& ) 
	throw( RuntimeException )
{
}
#endif

OKCancelBtnListener::OKCancelBtnListener( BaseDialog* pDlg, const rtl::OUString& sMode ) : 
		ActionListener( pDlg )
{
	m_sMode = sMode;
}

OKCancelBtnListener::~OKCancelBtnListener() throw()
{
}

void OKCancelBtnListener::disposing( const lang::EventObject& )
	throw ( RuntimeException )
{
}

void OKCancelBtnListener::actionPerformed( const awt::ActionEvent& )
	throw ( RuntimeException )
{	
	SolverDialog* pMainDlg = getDialog()->getSolverImpl()->getMainDialog();
	ConstEditDialog* pDlg = pMainDlg->getConstEditDialog();
	
	if ( m_sMode.equals( ascii ( "OK" ) ) )
	{
		rtl::OUString  sLeft = pDlg->getLeftCellReference();
		rtl::OUString sRight = pDlg->getRightCellReference();
		EqualityType         eEq = pDlg->getEquality();
		
		if ( pDlg->isChangeMode() )
			pMainDlg->editConstraint( pDlg->getConstraintId(), sLeft, sRight, eEq );
		else
			pMainDlg->setConstraint( sLeft, sRight, eEq );
	}
	
	pDlg->setVisible( false );
	pDlg->reset();
	pMainDlg->updateWidgets();
}


}
