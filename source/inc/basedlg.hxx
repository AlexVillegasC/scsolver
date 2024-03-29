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
 
#ifndef _BASEDLG_HXX_
#define _BASEDLG_HXX_

#include "tool/global.hxx"
#include <unoglobal.hxx>
#include <memory>
#include <com/sun/star/awt/PushButtonType.hpp>

#ifdef SCSOLVER_UNO_COMPONENT
class ResMgr;
#else
#include <tools/resmgr.hxx>
#endif

namespace com { namespace sun { namespace star {

	namespace awt
	{
        struct Rectangle;
		class XControlModel;
        class XControl;
	}

}}}

namespace scsolver {

class SolverImpl;
class Listener;
class FocusListener;
class MouseListener;
class ActionListener;
class ItemListener;
class TopWindowListener;

class BaseDialogImpl;
class ObjProperty;
class ObjPropertyImpl;
class WidgetProperty;
class WidgetPropertyImpl;

typedef std::auto_ptr<ObjProperty> apObjProp;
typedef std::auto_ptr<WidgetProperty> apWidgetProp;


/** Base class for all dialog classes used in scsolver. */
class BaseDialog
{
public:
	BaseDialog( SolverImpl* );
	virtual ~BaseDialog() throw() = 0;
	
	virtual void setVisible( bool ) = 0;

	/**
     * This method is called when a range selection is finished.  If
     * the dialog does not contain a range selection widget pair,
     * just leave this method empty.
     * 
     * @return false for signaling to the calling function that
     *  the selection is invalid and should be discarded, or true if
     *  the selection is satisfactory.
     */
	virtual bool doneRangeSelection() const = 0;

	virtual void close() = 0;

    void setRefBoundingBox(const ::com::sun::star::awt::Rectangle* rect);
    const ::com::sun::star::awt::Rectangle* getPosSize() const;

	// Widget creation methods
	
	apWidgetProp addButton( sal_Int32, sal_Int32, sal_Int32, sal_Int32,
			const rtl::OUString&, const rtl::OUString&, sal_Int16 = ::com::sun::star::awt::PushButtonType_STANDARD );
		
	apWidgetProp addButtonImage( sal_Int32, sal_Int32, sal_Int32, sal_Int32,
			const rtl::OUString &, const rtl::OUString &, sal_Int16 = ::com::sun::star::awt::PushButtonType_STANDARD );
		
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
			const rtl::OUString& name, const rtl::OUString& label );
		
	apWidgetProp addRangeEdit( sal_Int32, sal_Int32, sal_Int32, sal_Int32,
			const rtl::OUString &, const rtl::OUString & );
			
	// Widget registration methods

	void registerListener( TopWindowListener* p ) const;
	void registerListener( FocusListener* ) const;
	void registerListener( MouseListener* ) const;
	void registerListener( const rtl::OUString&, ActionListener* ) const;
	void registerListener( const rtl::OUString&, ItemListener* ) const;

	void unregisterListener( TopWindowListener* p ) const;
	void unregisterListener( FocusListener* ) const;
	void unregisterListener( MouseListener* ) const;
	void unregisterListener( const rtl::OUString&, ActionListener* ) const;
	void unregisterListener( const rtl::OUString&, ItemListener* ) const;

	// Other methods

	SolverImpl* getSolverImpl() const;
	const ::com::sun::star::uno::Reference < ::com::sun::star::awt::XControl > 
        getWidgetByName( const rtl::OUString& ) const;
	const ::com::sun::star::uno::Reference < ::com::sun::star::awt::XControlModel > 
        getWidgetModelByName( const rtl::OUString& ) const;
	void enableWidget( const rtl::OUString&, sal_Bool = true ) const;
	void toFront() const;
    void setFocus() const;
	void execute() const;

	rtl::OUString getResStr( int resid ) const;

protected:

	void initializeDefault( sal_Int16, sal_Int16, const rtl::OUString& ) const;
	void setVisibleDefault( bool ) const;

    void pushWidgetState();
    void popWidgetState();
    ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >
        getDialogModel() const;

private:
	std::auto_ptr<BaseDialogImpl> m_pImpl;
};

//--------------------------------------------------------------------------
// ObjProperty

class ObjProperty
{
public:
	ObjProperty( const ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >& );
	virtual ~ObjProperty() throw() = 0;
	
	void setPropertyValueAny( const char*, const ::com::sun::star::uno::Any& );

	template<typename AnyValue>
	void setPropertyValue( const char*, const AnyValue& );
	
private:
	std::auto_ptr<ObjPropertyImpl> m_pImpl;
};

//--------------------------------------------------------------------------
// WidgetProperty

class WidgetProperty : public ObjProperty
{
public:
	WidgetProperty( const ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >& );
	virtual ~WidgetProperty() throw();
	
	void setPositionX( sal_Int32 ) const;
	void setPositionY( sal_Int32 ) const;
	void setWidth( sal_Int32 ) const;
	void setHeight( sal_Int32 ) const;
	void setName( const rtl::OUString& ) const;
	void setLabel( const rtl::OUString& ) const;
    void setEnabled(bool b);
	
private:
	std::auto_ptr<WidgetPropertyImpl> m_pImpl;
};


// Helper function

const rtl::OUString getTextByWidget( BaseDialog*, const rtl::OUString& );
void setTextByWidget( BaseDialog*, const rtl::OUString&, const rtl::OUString& );


}

#endif // _BASEDLG_HXX_
















