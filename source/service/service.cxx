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

#include "solver.hxx"
#include "tool/global.hxx"
#include "dialog.hxx"
#include "lpbuilder.hxx"
#include "xcalc.hxx"
#include "option.hxx"
#include "resmgr.hxx"
#include "solvemodel.hxx"
#include "numeric/matrix.hxx"

#include "cppuhelper/implementationentry.hxx"
#include "com/sun/star/lang/XComponent.hpp"
#include "com/sun/star/frame/XDispatch.hpp"

#include <memory>

using ::rtl::OUString;

namespace scsolver {

//--------------------------------------------------------------------------
// Component operations

static uno::Sequence< rtl::OUString > SAL_CALL getSupportedServiceNames_SolverImpl();
static rtl::OUString getImplementationName_SolverImpl();
static Reference< uno::XInterface > SAL_CALL create_SolverImpl(
    Reference< uno::XComponentContext > const & xContext )
    SAL_THROW( () );


SolverImpl::SolverImpl( Reference< uno::XComponentContext > const & xContext ) :
#ifndef SCSOLVER_UNO_COMPONENT
    m_pResMgr(NULL),
#endif    
    m_pDlg(NULL), 
    m_pCalc(new CalcInterface(xContext)),
    m_pOption(new OptionData),
    m_pStringResMgr(new StringResMgr(m_pCalc.get()))
{
}

SolverImpl::~SolverImpl()
{
}

//--------------------------------------------------------------------------
// UNO Component Interface Methods

void SolverImpl::initialize( const Sequence< Any >& /*aArgs*/ ) throw( Exception )
{
}

rtl::OUString SolverImpl::getImplementationName()
    throw( RuntimeException )
{
    return rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(IMPLEMENTATION_NAME) );
}

sal_Bool SolverImpl::supportsService( rtl::OUString const & serviceName )
    throw( RuntimeException )
{
    return serviceName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM(SERVICE_NAME) );
}

Sequence< rtl::OUString > SolverImpl::getSupportedServiceNames()
    throw( RuntimeException )
{
    return getSupportedServiceNames_SolverImpl();
}

Reference< frame::XDispatch > SAL_CALL SolverImpl::queryDispatch(
    const util::URL& aURL, const ::rtl::OUString& /*sTargetFrameName*/, sal_Int32 /*nSearchFlags*/ )
    throw ( RuntimeException )
{
    Reference< frame::XDispatch > xRet;
    if ( aURL.Protocol.compareToAscii("org.go-oo.comp.CalcSolver:") == 0 )
    {
        if ( aURL.Path.compareToAscii("execute") == 0 )
            xRet = this;
    }
    return xRet;
}

Sequence< Reference< frame::XDispatch > > SAL_CALL SolverImpl::queryDispatches(
    const Sequence< frame::DispatchDescriptor >& seqDescripts )
    throw ( RuntimeException )
{
    sal_Int32 nCount = seqDescripts.getLength();
    Sequence< Reference< frame::XDispatch > > lDispatcher( nCount );
    
    for ( sal_Int32 i = 0; i < nCount; ++i )
        lDispatcher[i] = queryDispatch( 
            seqDescripts[i].FeatureURL, seqDescripts[i].FrameName, seqDescripts[i].SearchFlags );
    
    return lDispatcher;
}

void SAL_CALL SolverImpl::dispatch( 
    const util::URL& aURL, const Sequence< beans::PropertyValue >& /*lArgs*/ )
    throw ( RuntimeException )
{
    if ( aURL.Protocol.compareToAscii( "org.go-oo.comp.CalcSolver:" ) == 0 )
    {
        if ( aURL.Path.compareToAscii( "execute" ) == 0 )
        {
            execute();
        }
    }
}

void SAL_CALL SolverImpl::addStatusListener( 
    const Reference< frame::XStatusListener >& /*xControl*/, const util::URL& /*aURL*/ )
    throw ( RuntimeException )
{
}

void SAL_CALL SolverImpl::removeStatusListener( 
    const Reference< frame::XStatusListener >& /*xControl*/, const util::URL& /*aURL*/ )
    throw ( RuntimeException )
{
}

void SAL_CALL SolverImpl::dispatchWithNotification(
    const util::URL& /*aURL*/, const Sequence< beans::PropertyValue >& /*lArgs*/,
    const Reference< frame::XDispatchResultListener >& /*xDRL*/ )
    throw ( RuntimeException )
{
}

SolverDialog* SolverImpl::getMainDialog()
{
	if ( m_pDlg.get() == NULL )
		m_pDlg.reset( new SolverDialog( this ) );

	return m_pDlg.get();
}

CalcInterface* SolverImpl::getCalcInterface() const
{
	return m_pCalc.get();
}

OptionData* SolverImpl::getOptionData() const
{
	return m_pOption.get();
}

void SolverImpl::setTitle( const ::rtl::OUString& /*aTitle*/ )
		throw (uno::RuntimeException)
{
}

sal_Int16 SolverImpl::execute()
		throw (::com::sun::star::uno::RuntimeException)
{
	getMainDialog()->setVisible( true );
	return 0;
}

sal_Bool SolverImpl::solveModel()
{	
	Debug("solveModel --------------------------------------------------------");

	::std::auto_ptr<SolveModel> p( new SolveModel( this ) );
	try
	{
		p->solve();
	}
	catch( const RuntimeError& e )
	{
		getMainDialog()->showMessage( e.getMessage() );
	}

	if ( p->isSolved() )
	{
		Debug( "solution available" );
		return true;
	}
	
	return false;
}

void SolverImpl::initLocale()
{
#ifndef SCSOLVER_UNO_COMPONENT
	rtl::OString aModName( "scsolver" );
	aModName += rtl::OString::valueOf( sal_Int32( SUPD ) );

	m_pResMgr.reset(ResMgr::CreateResMgr(aModName.getStr(), m_eLocale));
#endif
}

#ifndef SCSOLVER_UNO_COMPONENT
ResMgr* SolverImpl::getResMgr()
{
    if ( !m_pResMgr.get() )
        initLocale();
    return m_pResMgr.get();
}
#endif

OUString SolverImpl::getResStr( int resid )
{
#ifdef SCSOLVER_UNO_COMPONENT
    return m_pStringResMgr->getLocaleStr(resid);
#else
   	ResMgr *pResMgr = getResMgr();
	if ( pResMgr )
        return OUString( String( ResId( resid, *getResMgr() ) ) );
    else
        return OUString();
#endif
}

// XLocalizable

void SAL_CALL SolverImpl::setLocale( const lang::Locale& eLocale )
	throw(::com::sun::star::uno::RuntimeException)
{
	m_eLocale = eLocale;
	initLocale();
}

lang::Locale SAL_CALL SolverImpl::getLocale()
	throw(::com::sun::star::uno::RuntimeException)
{
	return m_eLocale;
}

//---------------------------------------------------------------------------
// Component operations

static Sequence< rtl::OUString > getSupportedServiceNames_SolverImpl()
{
    static Sequence < rtl::OUString > *pNames = 0;
    if( !pNames )
    {
        static Sequence< rtl::OUString > seqNames(1);
        seqNames.getArray()[0] = rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(SERVICE_NAME));
        pNames = &seqNames;
    }
    return *pNames;
}

static rtl::OUString getImplementationName_SolverImpl()
{
    static rtl::OUString *pImplName = 0;
    if( !pImplName )
    {
        static rtl::OUString implName(
            RTL_CONSTASCII_USTRINGPARAM(IMPLEMENTATION_NAME));
        pImplName = &implName;
    }
    return *pImplName;
}

static Reference< uno::XInterface > SAL_CALL create_SolverImpl(
    Reference< uno::XComponentContext > const & xContext )
    SAL_THROW( () )
{
    return static_cast< lang::XTypeProvider * >( new SolverImpl( xContext ) );
}


static struct ::cppu::ImplementationEntry s_component_entries [] =
{
    {
        create_SolverImpl, getImplementationName_SolverImpl,
        getSupportedServiceNames_SolverImpl, ::cppu::createSingleComponentFactory,
        0, 0
    },
    { 0, 0, 0, 0, 0, 0 }
};

   
}


//------------------------------------------------------------------------------
// Shared library symbol exports

extern "C"
{
    void SAL_DLLPUBLIC_EXPORT component_getImplementationEnvironment(
        sal_Char const ** ppEnvTypeName, uno_Environment ** /*ppEnv*/ )
    {
        *ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
    }
    
    sal_Bool SAL_DLLPUBLIC_EXPORT component_writeInfo(
        lang::XMultiServiceFactory * xMgr, registry::XRegistryKey * xRegistry )
    {
        return ::cppu::component_writeInfoHelper(
            xMgr, xRegistry, ::scsolver::s_component_entries );
    }

    void SAL_DLLPUBLIC_EXPORT *component_getFactory(
        sal_Char const * implName, lang::XMultiServiceFactory * xMgr,
        registry::XRegistryKey * xRegistry )
    {
        return ::cppu::component_getFactoryHelper(
            implName, xMgr, xRegistry, ::scsolver::s_component_entries );
    }
}
