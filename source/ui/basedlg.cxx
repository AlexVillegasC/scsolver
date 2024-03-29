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

#include "basedlg.hxx"
#include "tool/global.hxx"
#include "unoglobal.hxx"
#include "type.hxx"
#include "solver.hxx"
#include "listener.hxx"
#include "xcalc.hxx"
#include "unohelper.hxx"

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XNameContainer.hpp>

#include <com/sun/star/awt/PosSize.hpp>
#include <com/sun/star/awt/Rectangle.hpp>
#include <com/sun/star/awt/XButton.hpp>
#include <com/sun/star/awt/XControl.hpp>
#include <com/sun/star/awt/XControlContainer.hpp>
#include <com/sun/star/awt/XControlModel.hpp>
#include <com/sun/star/awt/XDialog.hpp>
#include <com/sun/star/awt/XListBox.hpp>
#include <com/sun/star/awt/XRadioButton.hpp>
#include <com/sun/star/awt/XTextComponent.hpp>
#include <com/sun/star/awt/XTopWindow.hpp>
#include <com/sun/star/awt/XWindow.hpp>

#include <memory>
#include <vector>
#include <algorithm>

using namespace com::sun::star;
using ::com::sun::star::uno::UNO_QUERY;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::rtl::OUString;
using uno::Any;
using ::std::auto_ptr;
using ::std::for_each;
using ::std::vector;

namespace scsolver {

/**
 * Dump all service names supported by this widget and
 * terminate. This function is for debug purposes only, and
 * should never be reached under normal circumstances.
 */
#if SCSOLVER_DEBUG
void lcl_dumpServiceNames( const Reference< uno::XInterface >& oWgt )
{
	if (oWgt == NULL)
		return;
	Reference< lang::XServiceInfo > xSN( oWgt, UNO_QUERY );
	Sequence< rtl::OUString > sSN = xSN->getSupportedServiceNames();
	for ( int nIdx = 0; nIdx < sSN.getLength(); ++nIdx )
		printOUStr( sSN[nIdx] );

	OSL_ASSERT( !"No appropriate widget type got picked up!" );
}
#else
void lcl_dumpServiceNames( const Reference< uno::XInterface >& )
{
}
#endif

//--------------------------------------------------------------------------
// BaseDialogImpl

class BaseDialogImpl
{
public:
    struct WidgetState
    {
        OUString    name;
        bool        enabled;
    };

	BaseDialogImpl( SolverImpl* );
	~BaseDialogImpl();

    void initialize( sal_Int32 nW, sal_Int32 nH, const rtl::OUString& );
	void setVisibleDefault( bool );
	
	uno::Reference < uno::XInterface > getDialog() const { return m_oDlg; }
	uno::Reference < uno::XInterface > getDialogModel() const { return m_oDlgModel; }
	uno::Reference<awt::XControl> getWidgetByName( const rtl::OUString& );
	uno::Reference < uno::XInterface > getRangeEditWidget();
	SolverImpl* getSolverImpl() const { return m_pSolverImpl; }
	
	void enableWidget( const rtl::OUString&, sal_Bool );
	void toFront();
    void setFocus() const;
	void execute();
			
    void setRefBoundingBox(const awt::Rectangle* rect);
    const awt::Rectangle* getPosSize();

	apWidgetProp addButton( sal_Int32, sal_Int32, sal_Int32, sal_Int32,
		const rtl::OUString &, const rtl::OUString &, sal_Int16 = awt::PushButtonType_STANDARD );
		
	apWidgetProp addButtonImage( sal_Int32, sal_Int32, sal_Int32, sal_Int32,
		const rtl::OUString &, const rtl::OUString &, sal_Int16 = awt::PushButtonType_STANDARD );
		
	apWidgetProp addEdit( sal_Int32, sal_Int32, sal_Int32, sal_Int32, const rtl::OUString & );

	apWidgetProp addFixedLine( sal_Int32, sal_Int32, sal_Int32, sal_Int32, 
		const rtl::OUString &, const rtl::OUString &, sal_Int32 = 0 );
		
	apWidgetProp addFixedText( sal_Int32, sal_Int32, sal_Int32, sal_Int32, 
		const rtl::OUString &, const rtl::OUString & );
		
	apWidgetProp addGroupBox( sal_Int32, sal_Int32, sal_Int32, sal_Int32, const rtl::OUString & );
		
	apWidgetProp addListBox( sal_Int32, sal_Int32, sal_Int32, sal_Int32, const rtl::OUString & );
	
	apWidgetProp addRadioButton( sal_Int32, sal_Int32, sal_Int32, sal_Int32, 
		const rtl::OUString &, const rtl::OUString & );

	apWidgetProp addCheckBox( sal_Int32 x, sal_Int32 y, sal_Int32 w, sal_Int32 h,
		const rtl::OUString& name, const rtl::OUString& label )
	{
		apWidgetProp p = addWidget( 
			ascii("com.sun.star.awt.UnoControlCheckBoxModel"), x, y, w, h, name );
		p->setLabel(label);
		return p;
	}

	apWidgetProp addRangeEdit( sal_Int32, sal_Int32, sal_Int32, sal_Int32,
		const rtl::OUString &, const rtl::OUString & );
		
	apWidgetProp addWidget( const rtl::OUString&, 
		sal_Int32, sal_Int32, sal_Int32, sal_Int32, const rtl::OUString & );

	template<typename ListenerT>
	void registerListener( ListenerT* );

	template<typename ListenerT>
	void registerListener( const rtl::OUString &, ListenerT* );

	void registerListener( TopWindowListener* p )
	{
		Reference<awt::XTopWindow> xTW(getDialog(), UNO_QUERY);
		xTW->addTopWindowListener(p);
	}

	void registerListener( const Reference< uno::XInterface >&, ActionListener* );
	void registerListener( const Reference< uno::XInterface >&, ItemListener* );
	void registerListener( const Reference< uno::XInterface >&, FocusListener* );
	void registerListener( const Reference< uno::XInterface >&, MouseListener* );

	template<typename ListenerT>
	void unregisterListener( ListenerT* );

	template<typename ListenerT>
	void unregisterListener( const rtl::OUString &, ListenerT* );

	void unregisterListener( const Reference< uno::XInterface >&, ActionListener* );
	void unregisterListener( const Reference< uno::XInterface >&, ItemListener* );
	void unregisterListener( const Reference< uno::XInterface >&, FocusListener* );
	void unregisterListener( const Reference< uno::XInterface >&, MouseListener* );

	void unregisterListener( const Reference<uno::XInterface>& oWgt, TopWindowListener* p )
	{
		Reference<awt::XTopWindow> xWnd(oWgt, UNO_QUERY);
		if ( xWnd.is() )
		{
			xWnd->removeTopWindowListener( p );
			return;
		}
		lcl_dumpServiceNames( oWgt );
	}

    void pushWidgetState();
    void popWidgetState();

private:
    vector<WidgetState> m_widgetStates;

	SolverImpl* m_pSolverImpl;
	auto_ptr<awt::Rectangle> mpDlgPosSize;
    auto_ptr<awt::Rectangle> mpRefBoundingBox;
    bool mbIsVisible;
	
	uno::Reference< uno::XInterface > m_oDlg;
	uno::Reference< uno::XInterface > m_oDlgModel;
};



BaseDialogImpl::BaseDialogImpl( SolverImpl* p ) :
    m_pSolverImpl(p),
    mpDlgPosSize(NULL),
    mbIsVisible(false)
{
}

BaseDialogImpl::~BaseDialogImpl()
{
}

void BaseDialogImpl::initialize( sal_Int32 nW, sal_Int32 nH, const rtl::OUString& sTitle )
{
	CalcInterface* pCalc = getSolverImpl()->getCalcInterface();
	Reference< lang::XMultiComponentFactory > xSM = pCalc->getServiceManager();
	Reference< uno::XComponentContext > xCC = pCalc->getComponentContext();
	
	m_oDlg = xSM->createInstanceWithContext( 
			ascii( "com.sun.star.awt.UnoControlDialog" ), xCC );
	m_oDlgModel = xSM->createInstanceWithContext( 
			ascii( "com.sun.star.awt.UnoControlDialogModel" ), xCC );

    // Query necessary interfaces
    Reference< awt::XControl > xCtrl( m_oDlg, UNO_QUERY );
    Reference< awt::XControlModel > xCtrlModel( m_oDlgModel, UNO_QUERY );
    xCtrl->setModel( xCtrlModel );
	
	Reference< beans::XPropertySet > xPS( m_oDlgModel, UNO_QUERY );
	uno::Any aWidth, aHeight, aTitle, aCloseable, aHelpUrl;
	aWidth <<= nW;
	aHeight <<= nH;
	aTitle <<= sTitle;
	aCloseable <<= static_cast<sal_Bool>(true);
	aHelpUrl <<= ascii("org.go-oo.CalcSolver:Main");
	xPS->setPropertyValue( ascii( "Width" ), aWidth );
	xPS->setPropertyValue( ascii( "Height" ), aHeight );
	xPS->setPropertyValue( ascii( "Title" ), aTitle );
	xPS->setPropertyValue( ascii("Closeable"), aCloseable );
	xPS->setPropertyValue(ascii("HelpURL"), aHelpUrl);
}

void BaseDialogImpl::setVisibleDefault( bool bVisible )
{
    if (bVisible == mbIsVisible)
        return;

    Reference< awt::XWindow > xWnd(m_oDlg, UNO_QUERY);
    if (!xWnd.is())
    {
        Debug("xWnd is NULL!");
        return;
    }

    if (bVisible)
    {
        // show dialog.
        if (mpDlgPosSize.get())
        {
            // previous dialog position is stored.  use it.
            xWnd->setPosSize(mpDlgPosSize->X, mpDlgPosSize->Y, mpDlgPosSize->Width, 
                             mpDlgPosSize->Height, awt::PosSize::POS);
        }
        else if (mpRefBoundingBox.get())
        {
            // Reference bounding box (e.g. dialog size & position of the 
            // parent dialog) exists.  Compute desired dialog position from 
            // it.
            const awt::Rectangle* pRect = getPosSize();
            if (pRect)
            {
                sal_Int32 xoffset = (mpRefBoundingBox->Width - pRect->Width)/2;
                sal_Int32 yoffset = (mpRefBoundingBox->Height - pRect->Height)/2;
                xWnd->setPosSize(mpRefBoundingBox->X + xoffset, 
                                 mpRefBoundingBox->Y + yoffset,
                                 0, 0, awt::PosSize::POS);
            }
        }
    }
    else
    {
        // hide dialog.
        if (!mpDlgPosSize.get())
            mpDlgPosSize.reset(new awt::Rectangle);
        *mpDlgPosSize = xWnd->getPosSize();
    }

    xWnd->setVisible(bVisible);
    mbIsVisible = bVisible;
}

apWidgetProp BaseDialogImpl::addButton( 
	sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH, 
	const rtl::OUString& sName, const rtl::OUString& sLabel, sal_Int16 nPushBtnType )
{
	apWidgetProp p = addWidget( ascii( "com.sun.star.awt.UnoControlButtonModel" ),
			nX, nY, nW, nH, sName );	
	
	p->setLabel( sLabel );
	p->setPropertyValue( "PushButtonType", nPushBtnType );

	return p;
}

apWidgetProp BaseDialogImpl::addButtonImage( 
	sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH, 
	const rtl::OUString& sName, const rtl::OUString& sUrl, sal_Int16 nPushBtnType )
{
	apWidgetProp p = addWidget( ascii( "com.sun.star.awt.UnoControlButtonModel" ),
			nX, nY, nW, nH, sName );
	uno::Any aType, aUrl;
	aType <<= nPushBtnType;
	aUrl <<= sUrl;
	
	p->setPropertyValue( "ImageURL", aUrl );
	p->setPropertyValue( "PushButtonType", aType );
	
	return p;
}

apWidgetProp BaseDialogImpl::addEdit( sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH,
	const rtl::OUString& sName )
{
	apWidgetProp p = addWidget( ascii( "com.sun.star.awt.UnoControlEditModel" ),
			nX, nY, nW, nH, sName );

	return p;
}

apWidgetProp BaseDialogImpl::addFixedLine( sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH,
	const rtl::OUString& sName, const rtl::OUString& sLabel, sal_Int32 nOrientation )
{
	apWidgetProp p = addWidget( ascii( "com.sun.star.awt.UnoControlFixedLineModel" ),
			nX, nY, nW, nH, sName );
	
	if ( sLabel.getLength() )
		p->setLabel( sLabel );

	uno::Any aOrientation;
	aOrientation <<= nOrientation;
	p->setPropertyValue( "Orientation", aOrientation );

	return p;
}

apWidgetProp BaseDialogImpl::addFixedText( sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH,
	const rtl::OUString& sName, const rtl::OUString& sLabel )
{
	apWidgetProp p = addWidget( 
		ascii( "com.sun.star.awt.UnoControlFixedTextModel" ), nX, nY, nW, nH, sName );
	p->setLabel( sLabel );
	
	return p;
}

apWidgetProp BaseDialogImpl::addGroupBox( sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH,
	const rtl::OUString& sName )
{
	apWidgetProp p = addWidget( ascii( "com.sun.star.awt.UnoControlGroupBoxModel" ),
			nX, nY, nW, nH, sName );
	p->setLabel( ascii( "Group Box Label" ) ); // dummy text

	return p;
}

apWidgetProp BaseDialogImpl::addListBox( sal_Int32 nX, sal_Int32 nY, 
		sal_Int32 nW, sal_Int32 nH, const rtl::OUString& sName )
{
	apWidgetProp p = addWidget( 
		ascii( "com.sun.star.awt.UnoControlListBoxModel" ), nX, nY, nW, nH, sName );
	
	return p;
}

apWidgetProp BaseDialogImpl::addRadioButton( sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH,
	const rtl::OUString& sName, const rtl::OUString& sLabel )
{
	apWidgetProp p = addWidget( 
		ascii( "com.sun.star.awt.UnoControlRadioButtonModel" ), nX, nY, nW, nH, sName );
	p->setLabel( sLabel );
	return p;
}

apWidgetProp BaseDialogImpl::addRangeEdit( sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH,
    const rtl::OUString& sEditName, const rtl::OUString& sBtnName )
{
    // the spacings between the text box and the range selection button must
    // match those of built-in dialogs.
    const sal_Int32 nGap = 2, nBtnW = 13, nBtnH = 15;
    
    apWidgetProp p = addWidget(ascii("com.sun.star.awt.UnoControlEditModel"),
        nX, nY, nW - nGap - nBtnW, nH, sEditName);

    // add a range selection icon
    addButtonImage(nX + nW - nBtnW, nY-1, nBtnW, nBtnH, sBtnName, 
                   ascii("private:resource/sc/image/25041"));
    return p;
}

apWidgetProp BaseDialogImpl::addWidget( const rtl::OUString& sWidgetName, 
	sal_Int32 nX, sal_Int32 nY, sal_Int32 nWidth, sal_Int32 nHeight, const rtl::OUString& sName )
{
	Reference< lang::XMultiServiceFactory > xMSF( m_oDlgModel, UNO_QUERY );
	Reference< container::XNameContainer > xNC( m_oDlgModel, UNO_QUERY );
	
	Reference< uno::XInterface > oModel = xMSF->createInstance( sWidgetName );
	Any aWidget;
	aWidget <<= oModel;
	xNC->insertByName( sName, aWidget );

	apWidgetProp pProp( new WidgetProperty( oModel ) );
	pProp->setPositionX( nX );
	pProp->setPositionY( nY );
	pProp->setWidth( nWidth );
	pProp->setHeight( nHeight );
	pProp->setName( sName );
	
	return pProp;
}


Reference< uno::XInterface > BaseDialogImpl::getRangeEditWidget()
{
	return getWidgetByName( ascii( "editVarCells" ) );
}


Reference< awt::XControl > BaseDialogImpl::getWidgetByName( const rtl::OUString& sName )
{
	Reference< awt::XControlContainer > xCtrlContainer( m_oDlg, UNO_QUERY );
	Reference< awt::XControl > xWidget = xCtrlContainer->getControl( sName );
	return xWidget;
}

template<typename ListenerT>
void BaseDialogImpl::registerListener( ListenerT* p )
{
	registerListener( getDialog(), p );
}

template<typename ListenerT>
void BaseDialogImpl::registerListener( const rtl::OUString& sName, ListenerT* p )
{
	registerListener( getWidgetByName( sName ), p );
}


void BaseDialogImpl::registerListener( const Reference< uno::XInterface >& oWgt, ActionListener* p )
{
	OSL_ASSERT( oWgt != NULL );

	Reference< awt::XButton > xWidget( oWgt, UNO_QUERY );
	if ( xWidget.is() )
	{
		xWidget->addActionListener( p );
		return;
	}
	
	lcl_dumpServiceNames( oWgt );
}

void BaseDialogImpl::registerListener( const Reference< uno::XInterface >& oWgt, ItemListener* p )
{
	Reference< awt::XRadioButton > xRB( oWgt, UNO_QUERY );
	if ( xRB.is() )
	{
		xRB->addItemListener( p );
		return;
	}
	
	Reference< awt::XListBox > xLB( oWgt, UNO_QUERY );
	if ( xLB.is() )
	{
		xLB->addItemListener( p );
		return;
	}

	lcl_dumpServiceNames( oWgt );
}

void BaseDialogImpl::registerListener( const Reference< uno::XInterface >& oWgt, FocusListener* p )
{
	Reference< awt::XWindow > xWnd( oWgt, UNO_QUERY );
	if ( xWnd.is() )
	{
		xWnd->addFocusListener( p );
		return;
	}

	lcl_dumpServiceNames( oWgt );
}

void BaseDialogImpl::registerListener( const Reference< uno::XInterface >& oWgt, MouseListener* p )
{
	Reference< awt::XWindow > xWnd( oWgt, UNO_QUERY );
	if ( xWnd.is() )
	{
		xWnd->addMouseListener( p );
		return;
	}

	lcl_dumpServiceNames( oWgt );
}

template<typename ListenerT>
void BaseDialogImpl::unregisterListener( ListenerT* p )
{
	unregisterListener( getDialog(), p );
}

template<typename ListenerT>
void BaseDialogImpl::unregisterListener( const rtl::OUString& sName, ListenerT* p )
{
	Reference<uno::XInterface> oWgt = getWidgetByName(sName);
	if (oWgt == NULL)
	{
		printOUStr(ascii("warning: widget named ") + sName + ascii(" does not exist, thus cannot be unregistered."));
		return;
	}
	unregisterListener(oWgt, p);
}

void BaseDialogImpl::unregisterListener( const Reference< uno::XInterface >& oWgt, ActionListener* p )
{
	OSL_ASSERT( oWgt != NULL );
	Reference< awt::XButton > xWidget( oWgt, UNO_QUERY );
	if ( xWidget.is() )
	{
		xWidget->removeActionListener( p );
		return;
	}
	
	lcl_dumpServiceNames( oWgt );
}

void BaseDialogImpl::unregisterListener( const Reference< uno::XInterface >& oWgt, ItemListener* p )
{
	Reference< awt::XRadioButton > xRB( oWgt, UNO_QUERY );
	if ( xRB.is() )
	{
		xRB->removeItemListener( p );
		return;
	}

	Reference< awt::XListBox > xLB( oWgt, UNO_QUERY );
	if ( xLB.is() )
	{
		xLB->removeItemListener( p );
		return;
	}
	
	lcl_dumpServiceNames( oWgt );
}

void BaseDialogImpl::unregisterListener( const Reference< uno::XInterface >& oWgt, FocusListener* p )
{
	Reference< awt::XWindow > xWnd( oWgt, UNO_QUERY );
	if ( xWnd.is() )
	{
		xWnd->removeFocusListener( p );
		return;
	}
	
	lcl_dumpServiceNames( oWgt );
}

void BaseDialogImpl::unregisterListener( const Reference< uno::XInterface >& oWgt, MouseListener* p )
{
	Reference< awt::XWindow > xWnd( oWgt, UNO_QUERY );
	if ( xWnd.is() )
	{
		xWnd->removeMouseListener( static_cast<MouseListener*>( p ) );
		return;
	}

	lcl_dumpServiceNames( oWgt );
}

void BaseDialogImpl::enableWidget( const rtl::OUString& sName, sal_Bool bEnable )
{
	Reference< uno::XInterface > oWgt = getWidgetByName( sName );
	Reference< awt::XWindow > xWnd( oWgt, UNO_QUERY );
    if (!xWnd.is())
        return;
	xWnd->setEnable( bEnable );
}

void BaseDialogImpl::toFront()
{
	Reference< awt::XTopWindow > xWnd( m_oDlg, UNO_QUERY );
    if (!xWnd.is())
        return;
	xWnd->toFront();
}

void BaseDialogImpl::setFocus() const
{
    Reference<awt::XWindow> xWnd(m_oDlg, UNO_QUERY);
    if (!xWnd.is())
        return;

    xWnd->setFocus();
}

void BaseDialogImpl::execute()
{
	setVisibleDefault( true );
	toFront();
	Reference< awt::XDialog > xDlg( m_oDlg, UNO_QUERY );
	xDlg->execute();
}

void BaseDialogImpl::setRefBoundingBox(const awt::Rectangle* rect)
{
    if (!rect)
        return;

    if (!mpRefBoundingBox.get())
        mpRefBoundingBox.reset(new awt::Rectangle);
    *mpRefBoundingBox = *rect;
}

const awt::Rectangle* BaseDialogImpl::getPosSize()
{
    Reference< awt::XWindow > xWnd(m_oDlg, UNO_QUERY);
    if (xWnd.is())
        mpDlgPosSize.reset(new awt::Rectangle(xWnd->getPosSize()));
    return mpDlgPosSize.get();
}

void BaseDialogImpl::pushWidgetState()
{
    Reference<container::XNameContainer> xNC(m_oDlgModel, UNO_QUERY);
    if (!xNC.is())
        return;

    Sequence<OUString> names = xNC->getElementNames();
    sal_Int32 n = names.getLength();
    m_widgetStates.clear();
    m_widgetStates.reserve(n);
    for (sal_Int32 i = 0; i < n; ++i)
    {
        const OUString& name = names[i].intern();
        Reference<awt::XControl> widget = getWidgetByName(name);
        Reference<awt::XControlModel> model = widget->getModel();
        sal_Bool enabled = true;
        unohelper::getPropertyValue(model, ascii("Enabled"), enabled);

        WidgetState state;
        state.name = name;
        state.enabled = enabled;
        m_widgetStates.push_back(state);
    }
}

namespace {

class ApplyWidgetState
{
public:
    explicit ApplyWidgetState(BaseDialogImpl& rDlgImpl) :
        mrDlgImpl(rDlgImpl)
    {
    }

    void operator() (const BaseDialogImpl::WidgetState& widgetState) const
    {
        Reference<awt::XControl> widget = mrDlgImpl.getWidgetByName(widgetState.name);
        if (!widget.is())
            return;

        Reference<awt::XWindow> xWin(widget, UNO_QUERY);
        if (!xWin.is())
            return;

        xWin->setEnable(widgetState.enabled);
    }

private:
    BaseDialogImpl& mrDlgImpl;
};

}

void BaseDialogImpl::popWidgetState()
{
    for_each(m_widgetStates.begin(), m_widgetStates.end(), ApplyWidgetState(*this));
}

//--------------------------------------------------------------------------
// BaseDialog

BaseDialog::BaseDialog( SolverImpl* p ) : m_pImpl( new BaseDialogImpl( p ) )
{
}

BaseDialog::~BaseDialog() throw()
{
}

void BaseDialog::setRefBoundingBox(const awt::Rectangle* rect)
{
    m_pImpl->setRefBoundingBox(rect);
}

const awt::Rectangle* BaseDialog::getPosSize() const
{
    return m_pImpl->getPosSize();
}

void BaseDialog::initializeDefault( sal_Int16 nW, sal_Int16 nH, const rtl::OUString& sTitle ) const
{
	m_pImpl->initialize( nW, nH, sTitle );
}

void BaseDialog::setVisibleDefault( bool b ) const
{
	m_pImpl->setVisibleDefault( b );
}

void BaseDialog::pushWidgetState()
{
    m_pImpl->pushWidgetState();
}

void BaseDialog::popWidgetState()
{
    m_pImpl->popWidgetState();
}

Reference< uno::XInterface > BaseDialog::getDialogModel() const
{
    return m_pImpl->getDialogModel();
}

rtl::OUString BaseDialog::getResStr( int resid ) const
{
	return getSolverImpl()->getResStr( resid );
}

apWidgetProp BaseDialog::addButton( 
	sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH, 
	const rtl::OUString& sName, const rtl::OUString& sLabel, sal_Int16 nPushBtnType )
{
	return m_pImpl->addButton( nX, nY, nW, nH, sName, sLabel, nPushBtnType );
}


apWidgetProp BaseDialog::addButtonImage( 
	sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH, 
	const rtl::OUString& sName, const rtl::OUString& sUrl, sal_Int16 nPushBtnType )
{
	return m_pImpl->addButtonImage( nX, nY, nW, nH, sName, sUrl, nPushBtnType );
}


apWidgetProp BaseDialog::addEdit( sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH,
	const rtl::OUString& sName )
{
	return m_pImpl->addEdit( nX, nY, nW, nH, sName );
}


apWidgetProp BaseDialog::addFixedLine( sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH,
	const rtl::OUString& sName, const rtl::OUString& sLabel, sal_Int32 nOrientation )
{
	return m_pImpl->addFixedLine( nX, nY, nW, nH, sName, sLabel, nOrientation );
}


apWidgetProp BaseDialog::addFixedText( sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH,
	const rtl::OUString& sName, const rtl::OUString& sLabel )
{
	return m_pImpl->addFixedText( nX, nY, nW, nH, sName, sLabel );
}


apWidgetProp BaseDialog::addGroupBox( sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH,
	const rtl::OUString& sName )
{
	return m_pImpl->addGroupBox( nX, nY, nW, nH, sName );
}


apWidgetProp BaseDialog::addListBox( sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH, 
		const rtl::OUString& sName )
{
	return m_pImpl->addListBox( nX, nY, nW, nH, sName );
}


apWidgetProp BaseDialog::addRadioButton( sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH,
	const rtl::OUString& sName, const rtl::OUString& sLabel )
{
	return m_pImpl->addRadioButton( nX, nY, nW, nH, sName, sLabel );
}

apWidgetProp BaseDialog::addCheckBox( sal_Int32 x, sal_Int32 y, sal_Int32 w, sal_Int32 h,
	const rtl::OUString& name, const rtl::OUString& label )
{
	return m_pImpl->addCheckBox(x, y, w, h, name, label);
}

apWidgetProp BaseDialog::addRangeEdit( sal_Int32 nX, sal_Int32 nY, sal_Int32 nW, sal_Int32 nH,
	const rtl::OUString& sEditName, const rtl::OUString& sBtnName )
{
	return m_pImpl->addRangeEdit( nX, nY, nW, nH, sEditName, sBtnName );
}

void BaseDialog::registerListener( TopWindowListener* p ) const
{
	m_pImpl->registerListener(p);
}

void BaseDialog::registerListener( FocusListener* p ) const
{
	m_pImpl->registerListener( p );
}

void BaseDialog::registerListener( MouseListener* p ) const
{
	m_pImpl->registerListener( p );
}

void BaseDialog::registerListener( const rtl::OUString& sName, ActionListener* p ) const
{
	m_pImpl->registerListener( sName, p );
}

void BaseDialog::registerListener( const rtl::OUString& sName, ItemListener* p ) const
{
	m_pImpl->registerListener( sName, p );
}

void BaseDialog::unregisterListener( TopWindowListener* p ) const
{
	m_pImpl->unregisterListener(p);
}

void BaseDialog::unregisterListener( FocusListener* p ) const
{
	m_pImpl->unregisterListener( p );
}

void BaseDialog::unregisterListener( MouseListener* p ) const
{
	m_pImpl->unregisterListener( p );
}

void BaseDialog::unregisterListener( const rtl::OUString& sName, ActionListener* p ) const
{
	m_pImpl->unregisterListener( sName, p );
}

void BaseDialog::unregisterListener( const rtl::OUString& sName, ItemListener* p ) const
{
	m_pImpl->unregisterListener( sName, p );
}

SolverImpl* BaseDialog::getSolverImpl() const
{
	return m_pImpl->getSolverImpl();
}

const Reference< awt::XControl > BaseDialog::getWidgetByName( const rtl::OUString& sName ) const
{
	return m_pImpl->getWidgetByName( sName );
}

const Reference< awt::XControlModel > BaseDialog::getWidgetModelByName( 
		const rtl::OUString& sName ) const
{
	Reference< awt::XControl > obj = m_pImpl->getWidgetByName( sName );
	return obj->getModel();
}

void BaseDialog::enableWidget( const rtl::OUString& sName, sal_Bool bEnable ) const
{
	return m_pImpl->enableWidget( sName, bEnable );
}

void BaseDialog::toFront() const
{
    m_pImpl->toFront();
}

void BaseDialog::setFocus() const
{
    m_pImpl->setFocus();
}

void BaseDialog::execute() const
{
	m_pImpl->execute();
}

//--------------------------------------------------------------------------
// ObjPropertyImpl

class ObjPropertyImpl
{
public:
	ObjPropertyImpl( const Reference< XInterface >& );
	~ObjPropertyImpl() throw();
	void setPropertyValue( const char*, const Any& );
	
private:
	const Reference< beans::XPropertySet >& getPropertySet() const;

	Reference< XInterface > m_oModel;
	mutable Reference< beans::XPropertySet > m_xPS;
};

ObjPropertyImpl::ObjPropertyImpl( const Reference< XInterface >& aRef ) :
	m_oModel( aRef ), m_xPS( NULL )
{
	m_oModel = aRef;
}

ObjPropertyImpl::~ObjPropertyImpl() throw()
{
}

void ObjPropertyImpl::setPropertyValue( const char* name, const Any& aVal )
{
	getPropertySet()->setPropertyValue( ascii( name ), aVal );
}

const Reference< beans::XPropertySet >& ObjPropertyImpl::getPropertySet() const
{
	if ( m_xPS == NULL )
	{
		Reference< beans::XPropertySet > x( m_oModel, UNO_QUERY );
		m_xPS = x;
	}
	
	return m_xPS;
}


//--------------------------------------------------------------------------
// ObjProperty

ObjProperty::ObjProperty( const Reference< XInterface >& aRef ) : 
		m_pImpl( new ObjPropertyImpl( aRef ) )
{
}

ObjProperty::~ObjProperty() throw()
{
}

void ObjProperty::setPropertyValueAny( const char* name, const uno::Any& aVal )
{
	m_pImpl->setPropertyValue( name, aVal );
}

template<typename AnyValue>
void ObjProperty::setPropertyValue( const char* name, const AnyValue& t )
{
	Any aVal;
	aVal <<= t;
	m_pImpl->setPropertyValue( name, aVal );
}

//--------------------------------------------------------------------------
// WidgetPropertyImpl

class WidgetPropertyImpl
{
public:
	WidgetPropertyImpl( WidgetProperty* );
	~WidgetPropertyImpl() throw();

	void setPositionX( sal_Int32 ) const;
	void setPositionY( sal_Int32 ) const;
	void setWidth( sal_Int32 ) const;
	void setHeight( sal_Int32 ) const;
	void setName( const rtl::OUString& ) const;
	void setLabel( const rtl::OUString& ) const;

private:
	WidgetProperty* m_pSelf;
};

WidgetPropertyImpl::WidgetPropertyImpl( WidgetProperty* p ) :
		m_pSelf( p )
{
}

WidgetPropertyImpl::~WidgetPropertyImpl() throw()
{
}

void WidgetPropertyImpl::setPositionX( sal_Int32 n ) const
{
	m_pSelf->setPropertyValue( "PositionX", n );
}

void WidgetPropertyImpl::setPositionY( sal_Int32 n ) const
{
	m_pSelf->setPropertyValue( "PositionY", n );
}

void WidgetPropertyImpl::setWidth( sal_Int32 n ) const
{
	m_pSelf->setPropertyValue( "Width", n );
}

void WidgetPropertyImpl::setHeight( sal_Int32 n ) const
{
	m_pSelf->setPropertyValue( "Height", n );
}

void WidgetPropertyImpl::setName( const rtl::OUString& s ) const
{
	m_pSelf->setPropertyValue( "Name", s );
}

void WidgetPropertyImpl::setLabel( const rtl::OUString& s ) const
{
	m_pSelf->setPropertyValue( "Label", s );
}

//--------------------------------------------------------------------------
// WidgetProperty

WidgetProperty::WidgetProperty( const Reference< XInterface >& aRef ) :
		ObjProperty( aRef ), m_pImpl( new WidgetPropertyImpl( this ) )
{
}

WidgetProperty::~WidgetProperty() throw()
{
}

void WidgetProperty::setPositionX( sal_Int32 n ) const
{
	m_pImpl->setPositionX( n );
}

void WidgetProperty::setPositionY( sal_Int32 n ) const
{
	m_pImpl->setPositionY( n );
}

void WidgetProperty::setWidth( sal_Int32 n ) const
{
	m_pImpl->setWidth( n );
}

void WidgetProperty::setHeight( sal_Int32 n ) const
{
	m_pImpl->setHeight( n );
}

void WidgetProperty::setName( const rtl::OUString& s ) const
{
	m_pImpl->setName( s );
}

void WidgetProperty::setLabel( const rtl::OUString& s ) const
{
	m_pImpl->setLabel( s );
}

void WidgetProperty::setEnabled(bool b)
{
    setPropertyValue("Enabled", static_cast<sal_Bool>(b));
}

// Helper function

const rtl::OUString getTextByWidget( BaseDialog* pDlg, const rtl::OUString& sName )
{
	Reference< uno::XInterface > o = pDlg->getWidgetByName( sName );
	Reference< awt::XTextComponent > xComp( o, UNO_QUERY );
	return xComp->getText();
}

void setTextByWidget( BaseDialog* pDlg, const rtl::OUString& sName, const rtl::OUString& sVal )
{
	Reference< uno::XInterface > o = pDlg->getWidgetByName( sName );
	Reference< awt::XTextComponent > xComp( o, UNO_QUERY );
	xComp->setText( sVal );
}

}

















