/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=cpp:et:sw=4:ts=4:sts=4*/

#include <feel/feelmodels/heattransfer/heattransfer.hpp>

#include <feel/feelvf/vf.hpp>

//#include <feel/feelmodels/modelcore/stabilizationglsparameter.hpp>
#include <feel/feelmodels/modelvf/stabilizationglsparameter.hpp>

namespace Feel
{
namespace FeelModels
{


HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateLinearPDEStabilizationGLS( DataUpdateLinear & data ) const
{
    sparse_matrix_ptrtype& A = data.matrix();
    vector_ptrtype& F = data.rhs();
    bool buildCstPart = data.buildCstPart();
    if ( !this->fieldVelocityConvectionIsUsedAndOperational() || buildCstPart )
        return;

    auto mesh = this->mesh();
    auto Xh = this->spaceTemperature();
    auto const& u = this->fieldTemperature();
    auto const& v = this->fieldTemperature();

    auto bilinearForm_PatternCoupled = form2( _test=Xh,_trial=Xh,_matrix=A,
                                              _pattern=size_type(Pattern::COUPLED),
                                              _rowstart=this->rowStartInMatrix(),
                                              _colstart=this->colStartInMatrix() );
    auto myLinearForm = form1( _test=Xh, _vector=F,
                               _rowstart=this->rowStartInVector() );

    //if ( this->fieldVelocityConvectionIsUsedAndOperational() && !buildCstPart )
    //{
        auto kappa = idv(this->thermalProperties()->fieldThermalConductivity());
        auto thecoeff = idv(this->thermalProperties()->fieldRho())*idv(this->thermalProperties()->fieldHeatCapacity());
        auto uconv=thecoeff*idv(this->fieldVelocityConvection());
        auto rangeStabUsed = M_rangeMeshElements;
#if 0
        typedef StabilizationGLSParameter<mesh_type, nOrderTemperature> stab_gls_parameter_impl_type;
        auto stabGLSParam =  std::dynamic_pointer_cast<stab_gls_parameter_impl_type>( this->stabilizationGLSParameter() );
        stabGLSParam->updateTau(uconv, kappa, rangeStabUsed);
        auto tau = idv(stabGLSParam->fieldTau());
#elif 1
        auto tau = Feel::vf::FeelModels::stabilizationGLSParameterExpr( *this->stabilizationGLSParameter(),uconv, kappa );
#else
        static const uint16_type nStabGlsOrderPoly = (nOrderTemperature>1)? nOrderTemperature : 2;
        typedef StabilizationGLSParameter<mesh_type, nStabGlsOrderPoly> stab_gls_parameter_impl_type;
        auto tau =  std::dynamic_pointer_cast<stab_gls_parameter_impl_type>( this->stabilizationGLSParameter() )->tau( uconv, kappa, mpl::int_<0/*StabParamType*/>() );
        std::cout << "decltype(tau)::imorder=" << decltype(tau)::imorder << "\n";
        std::cout << "decltype(tau)::imIsPoly=" << decltype(tau)::imIsPoly << "\n";

#endif
        if ( nOrderTemperature <= 1 || this->stabilizationGLSType() == "supg"  )
        {
            auto stab_test = grad(u)*uconv;
            if (!this->isStationary())
            {
                auto stab_residual_bilinear = thecoeff*(idt(u)*this->timeStepBdfTemperature()->polyDerivCoefficient(0) + gradt(u)*idv(this->fieldVelocityConvection()) );
                bilinearForm_PatternCoupled +=
                    integrate( _range=M_rangeMeshElements,
                               _expr=tau*stab_residual_bilinear*stab_test,
                               _geomap=this->geomap() );
                auto rhsTimeStep = this->timeStepBdfTemperature()->polyDeriv();
                auto stab_residual_linear = thecoeff*idv( rhsTimeStep );
                myLinearForm +=
                    integrate( _range=M_rangeMeshElements,
                               _expr= val(tau*stab_residual_linear)*stab_test,
                               _geomap=this->geomap() );
            }
            else
            {
                auto stab_residual_bilinear = thecoeff*gradt(u)*idv(this->fieldVelocityConvection());
                bilinearForm_PatternCoupled +=
                    integrate( _range=M_rangeMeshElements,
                               _expr=tau*stab_residual_bilinear*stab_test,
                               _geomap=this->geomap() );
            }
            for( auto const& d : this->bodyForces() )
            {
                auto rangeBodyForceUsed = ( marker(d).empty() )? M_rangeMeshElements : markedelements(mesh,marker(d));
                myLinearForm +=
                    integrate( _range=rangeBodyForceUsed,
                               _expr=tau*expression(d)*stab_test,
                               _geomap=this->geomap() );
            }
        }
        else if ( ( this->stabilizationGLSType() == "gls" ) || ( this->stabilizationGLSType() == "unusual-gls" ) )
        {
            int stabCoeffDiffusion = (this->stabilizationGLSType() == "gls")? 1 : -1;
            auto stab_test = grad(u)*uconv + stabCoeffDiffusion*kappa*laplacian(u);
            if (!this->isStationary())
            {
                auto stab_residual_bilinear = thecoeff*(idt(u)*this->timeStepBdfTemperature()->polyDerivCoefficient(0) + gradt(u)*idv(this->fieldVelocityConvection()) ) - kappa*laplaciant(u);
                bilinearForm_PatternCoupled +=
                    integrate( _range=M_rangeMeshElements,
                               _expr=tau*stab_residual_bilinear*stab_test,
                               _geomap=this->geomap() );
                auto rhsTimeStep = this->timeStepBdfTemperature()->polyDeriv();
                auto stab_residual_linear = thecoeff*idv( rhsTimeStep );
                myLinearForm +=
                    integrate( _range=M_rangeMeshElements,
                               _expr= val(tau*stab_residual_linear)*stab_test,
                               _geomap=this->geomap() );
            }
            else
            {
                auto stab_residual_bilinear = thecoeff*gradt(u)*idv(this->fieldVelocityConvection()) - kappa*laplaciant(u);
                bilinearForm_PatternCoupled +=
                    integrate( _range=M_rangeMeshElements,
                               _expr=tau*stab_residual_bilinear*stab_test,
                               _geomap=this->geomap() );
            }
            for( auto const& d : this->bodyForces() )
            {
                auto rangeBodyForceUsed = ( marker(d).empty() )? M_rangeMeshElements : markedelements(mesh,marker(d));
                myLinearForm +=
                    integrate( _range=rangeBodyForceUsed,
                               _expr=tau*expression(d)*stab_test,
                               _geomap=this->geomap() );
            }
        }
        //}

}






HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateLinearPDE( DataUpdateLinear & data ) const
{
    sparse_matrix_ptrtype& A = data.matrix();
    vector_ptrtype& F = data.rhs();
    bool buildCstPart = data.buildCstPart();
    bool _doBCStrongDirichlet = data.doBCStrongDirichlet();

    std::string sc=(buildCstPart)?" (build cst part)":" (build non cst part)";
    this->log("HeatTransfer","updateLinearPDE", "start"+sc);
    boost::mpi::timer thetimer;

    auto mesh = this->mesh();
    auto Xh = this->spaceTemperature();
    auto const& u = this->fieldTemperature();
    auto const& v = this->fieldTemperature();

    auto bilinearForm_PatternCoupled = form2( _test=Xh,_trial=Xh,_matrix=A,
                                              _pattern=size_type(Pattern::COUPLED),
                                              _rowstart=this->rowStartInMatrix(),
                                              _colstart=this->colStartInMatrix() );
    auto myLinearForm = form1( _test=Xh, _vector=F,
                               _rowstart=this->rowStartInVector() );

    //--------------------------------------------------------------------------------------------------//

    if ( buildCstPart )
    {
        //double kappa = this->thermalProperties()->cstThermalConductivity();
        auto kappa = idv(this->thermalProperties()->fieldThermalConductivity());
        bilinearForm_PatternCoupled +=
            integrate( _range=M_rangeMeshElements,
                       _expr= kappa*inner(gradt(u),grad(v)),
                       _geomap=this->geomap() );
    }

    if ( this->fieldVelocityConvectionIsUsedAndOperational() && !buildCstPart )
    {
        //double thecoeff = this->thermalProperties()->cstRho()*this->thermalProperties()->cstHeatCapacity();
        auto thecoeff = idv(this->thermalProperties()->fieldRho())*idv(this->thermalProperties()->fieldHeatCapacity());
        bilinearForm_PatternCoupled +=
            integrate( _range=M_rangeMeshElements,
                       _expr= thecoeff*(gradt(u)*idv(this->fieldVelocityConvection()))*id(v),
                       _geomap=this->geomap() );


        //viscous dissipation
        if ( false/*true*/ )
        {
            double mu = 1.;
            auto defv = sym(gradv( this->fieldVelocityConvection() ) );
            auto defv2 = inner(defv,defv);

            if ( !this->fieldVelocityConvectionIsIncompressible() )
            {
#if 0
                bilinearForm_PatternCoupled +=
                    integrate( _range=M_rangeMeshElements,
                               _expr= thecoeff*(idt(u)*divv(this->fieldVelocityConvection()))*id(v),
                               _geomap=this->geomap() );
#endif
                myLinearForm +=
                    integrate( _range=M_rangeMeshElements,
                               _expr= 2*mu*defv2*id(v),
                               _geomap=this->geomap() );
            }
            else
            {
                auto incomp2 = pow( divv( this->fieldVelocityConvection() ),2 );
                myLinearForm +=
                    integrate( _range=M_rangeMeshElements,
                               _expr= 2*mu*(defv2-(1./3.)*incomp2)*id(v),
                               _geomap=this->geomap() );
            }
        }
    }

    if (!this->isStationary())
    {
        bool buildNonCstPart=!buildCstPart;
        bool BuildNonCstPart_Form2TransientTerm = buildNonCstPart;
        bool BuildNonCstPart_Form1TransientTerm = buildNonCstPart;
        if ( this->timeStepBase()->strategy()==TS_STRATEGY_DT_CONSTANT )
        {
            BuildNonCstPart_Form2TransientTerm = buildCstPart;
        }

        if (BuildNonCstPart_Form2TransientTerm)
        {
            //double thecoeff = this->thermalProperties()->cstRho()*this->thermalProperties()->cstHeatCapacity()*this->timeStepBdfTemperature()->polyDerivCoefficient(0);
            auto thecoeff = idv(this->thermalProperties()->fieldRho())*idv(this->thermalProperties()->fieldHeatCapacity())*this->timeStepBdfTemperature()->polyDerivCoefficient(0);
            bilinearForm_PatternCoupled +=
                integrate( _range=M_rangeMeshElements,
                           _expr= thecoeff*idt(u)*id(v),
                           _geomap=this->geomap() );
        }

        if (BuildNonCstPart_Form1TransientTerm)
        {
            //double thecoeff = this->thermalProperties()->cstRho()*this->thermalProperties()->cstHeatCapacity();
            auto thecoeff = idv(this->thermalProperties()->fieldRho())*idv(this->thermalProperties()->fieldHeatCapacity());
            auto rhsTimeStep = this->timeStepBdfTemperature()->polyDeriv();
            myLinearForm +=
                integrate( _range=M_rangeMeshElements,
                           _expr= thecoeff*idv(rhsTimeStep)*id(v),
                           _geomap=this->geomap() );
        }
    }

    //--------------------------------------------------------------------------------------------------//

    // update source term
    this->updateSourceTermLinearPDE(F, buildCstPart);

    // update stabilization gls
    if ( M_stabilizationGLS )
    {
        this->updateLinearPDEStabilizationGLS( data );
    }

    // update bc
    this->updateWeakBCLinearPDE(A,F,buildCstPart);
    if ( !buildCstPart && _doBCStrongDirichlet)
        this->updateBCStrongDirichletLinearPDE(A,F);


    double timeElapsed = thetimer.elapsed();
    this->log("HeatTransfer","updateLinearPDE",
              "finish in "+(boost::format("%1% s") % timeElapsed).str() );
}

HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateNewtonInitialGuess( vector_ptrtype& U ) const
{
    if ( M_bcDirichlet.empty() ) return;

    this->log("HeatTransfer","updateNewtonInitialGuess","start" );

    auto mesh = this->mesh();
    auto u = this->spaceTemperature()->element( U, this->rowStartInVector() );

    for( auto const& d : M_bcDirichlet )
    {
        u.on(_range=markedfaces(mesh, this->markerDirichletBCByNameId( "elimination",marker(d) ) ),
             _expr=expression(d) );
    }
    // synchronize temperature dof on interprocess
    auto itFindDofsWithValueImposed = M_dofsWithValueImposed.find("temperature");
    if ( itFindDofsWithValueImposed != M_dofsWithValueImposed.end() )
        sync( u, "=", itFindDofsWithValueImposed->second );

    this->log("HeatTransfer","updateNewtonInitialGuess","finish" );
}

HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateJacobian( DataUpdateJacobian & data ) const
{
    const vector_ptrtype& XVec = data.currentSolution();
    sparse_matrix_ptrtype& J = data.jacobian();
    vector_ptrtype& RBis = data.vectorUsedInStrongDirichlet();
    bool _BuildCstPart = data.buildCstPart();
    bool _doBCStrongDirichlet = data.doBCStrongDirichlet();

    bool buildNonCstPart = !_BuildCstPart;
    bool buildCstPart = _BuildCstPart;

    std::string sc=(buildCstPart)?" (build cst part)":" (build non cst part)";
    this->log("HeatTransfer","updateJacobian", "start"+sc);

    auto mesh = this->mesh();
    auto Xh = this->spaceTemperature();
    auto const& u = this->fieldTemperature();
    auto const& v = this->fieldTemperature();
#if 0
    auto const u = Xh->element(XVec, this->rowStartInVector());
#endif

    auto bilinearForm_PatternCoupled = form2( _test=Xh,_trial=Xh,_matrix=J,
                                              _pattern=size_type(Pattern::COUPLED),
                                              _rowstart=this->rowStartInMatrix(),
                                              _colstart=this->colStartInMatrix() );


    if ( buildCstPart )
    {
        //double kappa = this->thermalProperties()->cstThermalConductivity();
        auto kappa = idv(this->thermalProperties()->fieldThermalConductivity());
        bilinearForm_PatternCoupled +=
            integrate( _range=M_rangeMeshElements,
                       _expr= kappa*inner(gradt(u),grad(v)),
                       _geomap=this->geomap() );
    }

    if ( this->fieldVelocityConvectionIsUsedAndOperational() && !buildCstPart )
    {
        //double thecoeff = this->thermalProperties()->cstRho()*this->thermalProperties()->cstHeatCapacity();
        auto thecoeff = idv(this->thermalProperties()->fieldRho())*idv(this->thermalProperties()->fieldHeatCapacity());
        bilinearForm_PatternCoupled +=
            integrate( _range=M_rangeMeshElements,
                       _expr= thecoeff*(gradt(u)*idv(this->fieldVelocityConvection()))*id(v),
                       _geomap=this->geomap() );
    }

    if ( !this->isStationary() )
    {
        bool BuildNonCstPart_Form2TransientTerm = buildNonCstPart;
        if ( this->timeStepBase()->strategy()==TS_STRATEGY_DT_CONSTANT )
        {
            BuildNonCstPart_Form2TransientTerm = buildCstPart;
        }

        if (BuildNonCstPart_Form2TransientTerm)
        {
            //double thecoeff = this->thermalProperties()->cstRho()*this->thermalProperties()->cstHeatCapacity()*this->timeStepBdfTemperature()->polyDerivCoefficient(0);
            auto thecoeff = idv(this->thermalProperties()->fieldRho())*idv(this->thermalProperties()->fieldHeatCapacity())*this->timeStepBdfTemperature()->polyDerivCoefficient(0);
            bilinearForm_PatternCoupled +=
                integrate( _range=M_rangeMeshElements,
                           _expr= thecoeff*idt(u)*id(v),
                           _geomap=this->geomap() );
        }
    }


    this->updateBCRobinJacobian( J,buildCstPart );

    if ( buildNonCstPart && _doBCStrongDirichlet )
    {
        this->updateBCStrongDirichletJacobian( J,RBis );
    }

}
HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateResidual( DataUpdateResidual & data ) const
{
    const vector_ptrtype& XVec = data.currentSolution();
    vector_ptrtype& R = data.residual();
    bool _BuildCstPart = data.buildCstPart();
    bool UseJacobianLinearTerms = data.useJacobianLinearTerms();
    bool _doBCStrongDirichlet = data.doBCStrongDirichlet();

    bool buildNonCstPart = !_BuildCstPart;
    bool buildCstPart = _BuildCstPart;

    std::string sc=(buildCstPart)?" (build cst part)":" (build non cst part)";
    this->log("HeatTransfer","updateResidual", "start"+sc);

    auto mesh = this->mesh();
    auto Xh = this->spaceTemperature();
    auto const& v = this->fieldTemperature();
    auto const u = Xh->element(XVec, this->rowStartInVector());

    auto myLinearForm = form1( _test=Xh, _vector=R,
                               _rowstart=this->rowStartInVector() );

    if (!buildCstPart && !UseJacobianLinearTerms )
    {
        //double kappa = this->thermalProperties()->cstThermalConductivity();
        auto kappa = idv(this->thermalProperties()->fieldThermalConductivity());
        myLinearForm +=
            integrate( _range=M_rangeMeshElements,
                       _expr= kappa*inner(gradv(u),grad(v)),
                       _geomap=this->geomap() );
    }

    if ( this->fieldVelocityConvectionIsUsedAndOperational() )
    {
        if ( !buildCstPart )
        {
            //double thecoeff = this->thermalProperties()->cstRho()*this->thermalProperties()->cstHeatCapacity();
            auto thecoeff = idv(this->thermalProperties()->fieldRho())*idv(this->thermalProperties()->fieldHeatCapacity());
            myLinearForm +=
                integrate( _range=M_rangeMeshElements,
                           _expr= thecoeff*(gradv(u)*idv(this->fieldVelocityConvection()))*id(v),
                           _geomap=this->geomap() );
        }

        //viscous dissipation
        if ( false/*buildCstPart*/ )
        {
            double mu = 1.;
            auto defv = sym(gradv( this->fieldVelocityConvection() ) );
            auto defv2 = inner(defv,defv);

            if ( !this->fieldVelocityConvectionIsIncompressible() )
            {
                myLinearForm +=
                    integrate( _range=M_rangeMeshElements,
                               _expr= -2*mu*defv2*id(v),
                               _geomap=this->geomap() );
            }
            else
            {
                auto incomp2 = pow( divv( this->fieldVelocityConvection() ),2 );
                myLinearForm +=
                    integrate( _range=M_rangeMeshElements,
                               _expr= -2*mu*(defv2-(1./3.)*incomp2)*id(v),
                               _geomap=this->geomap() );
            }
        }

    }

    if (!this->isStationary())
    {
        bool Build_TransientTerm = !buildCstPart;
        if ( this->timeStepBase()->strategy()==TS_STRATEGY_DT_CONSTANT ) Build_TransientTerm=!buildCstPart && !UseJacobianLinearTerms;

        if (Build_TransientTerm)
        {
            //double thecoeff = this->thermalProperties()->cstRho()*this->thermalProperties()->cstHeatCapacity()*this->timeStepBdfTemperature()->polyDerivCoefficient(0);
            auto thecoeff = idv(this->thermalProperties()->fieldRho())*idv(this->thermalProperties()->fieldHeatCapacity())*this->timeStepBdfTemperature()->polyDerivCoefficient(0);
            myLinearForm +=
                integrate( _range=M_rangeMeshElements,
                           _expr= thecoeff*idv(u)*id(v),
                           _geomap=this->geomap() );
        }

        if (buildCstPart)
        {
            //double thecoeff = this->thermalProperties()->cstRho()*this->thermalProperties()->cstHeatCapacity();
            auto thecoeff = idv(this->thermalProperties()->fieldRho())*idv(this->thermalProperties()->fieldHeatCapacity());
            auto rhsTimeStep = this->timeStepBdfTemperature()->polyDeriv();
            myLinearForm +=
                integrate( _range=M_rangeMeshElements,
                           _expr= -thecoeff*idv(rhsTimeStep)*id(v),
                           _geomap=this->geomap() );
        }
    }

    this->updateSourceTermResidual( R,buildCstPart ) ;
    this->updateBCNeumannResidual( R,buildCstPart );
    this->updateBCRobinResidual( u,R,buildCstPart );

    if ( !buildCstPart && _doBCStrongDirichlet && this->hasMarkerDirichletBCelimination() )
    {
        R->close();
        this->updateBCDirichletStrongResidual( R );
    }

    this->log("HeatTransfer","updateResidual", "finish");
}

HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateBCDirichletStrongResidual( vector_ptrtype& R ) const
{
    if ( this->M_bcDirichlet.empty() ) return;

    this->log("HeatTransfer","updateBCDirichletStrongResidual","start" );

    auto mesh = this->mesh();
#if 0
    auto u = this->spaceTemperature()->element( R,this->rowStartInVector() );

    for( auto const& d : this->M_bcDirichlet )
    {
        u.on(_range=markedfaces(mesh,this->markerDirichletBCByNameId( "elimination",marker(d) ) ),
             _expr=cst(0.) );
    }
#else
    auto u = this->spaceTemperature()->element( R,this->rowStartInVector() );
    auto itFindDofsWithValueImposed = M_dofsWithValueImposed.find("temperature");
    auto const& dofsWithValueImposedTemperature = ( itFindDofsWithValueImposed != M_dofsWithValueImposed.end() )? itFindDofsWithValueImposed->second : std::set<size_type>();
    for ( size_type thedof : dofsWithValueImposedTemperature )
        u.set( thedof,0. );
    sync( u, "=", dofsWithValueImposedTemperature );
#endif
    this->log("HeatTransfer","updateBCDirichletStrongResidual","finish" );
}

HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateBCNeumannResidual( vector_ptrtype& R, bool buildCstPart ) const
{
    if ( this->M_bcNeumann.empty() ) return;

    if ( buildCstPart )
    {
        auto mesh = this->mesh();
        auto Xh = this->spaceTemperature();
        auto const& v = this->fieldTemperature();

        auto myLinearForm = form1( _test=Xh, _vector=R,
                                   _rowstart=this->rowStartInVector() );
        for( auto const& d : this->M_bcNeumann )
        {
            myLinearForm +=
                integrate( _range=markedfaces(this->mesh(),this->markerNeumannBC(NeumannBCShape::SCALAR,marker(d)) ),
                           _expr= -expression(d)*id(v),
                           _geomap=this->geomap() );
        }
    }

}

HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateBCRobinResidual( element_temperature_external_storage_type const& u, vector_ptrtype& R, bool buildCstPart ) const
{
    if ( this->M_bcRobin.empty() ) return;

    auto mesh = this->mesh();
    auto Xh = this->spaceTemperature();
    auto const& v = this->fieldTemperature();

    auto myLinearForm = form1( _test=Xh, _vector=R,
                               _rowstart=this->rowStartInVector() );
    for( auto const& d : this->M_bcRobin )
    {
        if ( !buildCstPart )
        {
            myLinearForm +=
                integrate( _range=markedfaces(mesh,this->markerRobinBC( marker(d) ) ),
                           _expr= expression1(d)*idv(u)*id(v),
                           _geomap=this->geomap() );
        }
        if ( buildCstPart )
        {
            myLinearForm +=
                integrate( _range=markedfaces(mesh,this->markerRobinBC( marker(d) ) ),
                           _expr= -expression1(d)*expression2(d)*id(v),
                           _geomap=this->geomap() );
        }
    }
}

HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateSourceTermResidual( vector_ptrtype& R, bool buildCstPart ) const
{
    if ( this->M_volumicForcesProperties.empty() ) return;

    if ( buildCstPart )
    {
        auto myLinearForm = form1( _test=this->spaceTemperature(), _vector=R,
                                   _rowstart=this->rowStartInVector() );
        auto const& v = this->fieldTemperature();

        for( auto const& d : this->M_volumicForcesProperties )
        {
            auto rangeBodyForceUsed = ( marker(d).empty() )? M_rangeMeshElements : markedelements(this->mesh(),marker(d));
            myLinearForm +=
                integrate( _range=rangeBodyForceUsed,
                           _expr= -expression(d)*id(v),
                           _geomap=this->geomap() );
        }
    }
}

HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateBCStrongDirichletJacobian(sparse_matrix_ptrtype& J,vector_ptrtype& RBis ) const
{
    if ( this->M_bcDirichlet.empty() ) return;

    this->log("HeatTransfer","updateBCStrongDirichletJacobian","start" );

    auto mesh = this->mesh();
    auto Xh = this->spaceTemperature();
    auto const& u = this->fieldTemperature();
    auto bilinearForm_PatternCoupled = form2( _test=Xh,_trial=Xh,_matrix=J,
                                              _pattern=size_type(Pattern::COUPLED),
                                              _rowstart=this->rowStartInMatrix(),
                                              _colstart=this->colStartInMatrix() );

    for( auto const& d : this->M_bcDirichlet )
    {
        bilinearForm_PatternCoupled +=
            on( _range=markedfaces(mesh, this->markerDirichletBCByNameId( "elimination",marker(d) ) ),
                _element=u,_rhs=RBis,_expr=cst(0.) );
    }

    this->log("HeatTransfer","updateBCStrongDirichletJacobian","finish" );

}

HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateBCRobinJacobian( sparse_matrix_ptrtype& J, bool buildCstPart ) const
{
    if ( this->M_bcRobin.empty() ) return;

    if ( !buildCstPart )
    {
        auto mesh = this->mesh();
        auto Xh = this->spaceTemperature();
        auto const& v = this->fieldTemperature();

        auto bilinearForm_PatternCoupled = form2( _test=Xh,_trial=Xh,_matrix=J,
                                                  _pattern=size_type(Pattern::COUPLED),
                                                  _rowstart=this->rowStartInMatrix(),
                                                  _colstart=this->colStartInMatrix() );
        for( auto const& d : this->M_bcRobin )
        {
            bilinearForm_PatternCoupled +=
                integrate( _range=markedfaces(mesh,this->markerRobinBC( marker(d) ) ),
                           _expr= expression1(d)*idt(v)*id(v),
                           _geomap=this->geomap() );
        }
    }
}

HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateBCStrongDirichletLinearPDE(sparse_matrix_ptrtype& A, vector_ptrtype& F) const
{
    if ( this->M_bcDirichlet.empty() ) return;

    this->log("HeatTransfer","updateBCStrongDirichletLinearPDE","start" );

    auto mesh = this->mesh();
    auto Xh = this->spaceTemperature();
    auto const& u = this->fieldTemperature();
    auto bilinearForm_PatternCoupled = form2( _test=Xh,_trial=Xh,_matrix=A,
                                              _pattern=size_type(Pattern::COUPLED),
                                              _rowstart=this->rowStartInMatrix(),
                                              _colstart=this->colStartInMatrix() );

    for( auto const& d : this->M_bcDirichlet )
    {
        bilinearForm_PatternCoupled +=
            on( _range=markedfaces(mesh, this->markerDirichletBCByNameId( "elimination",marker(d) ) ),
                _element=u,_rhs=F,_expr=expression(d) );
    }

    this->log("HeatTransfer","updateBCStrongDirichletLinearPDE","finish" );
}

HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateSourceTermLinearPDE( vector_ptrtype& F, bool buildCstPart ) const
{
    if ( this->M_overwritemethod_updateSourceTermLinearPDE != NULL )
    {
        this->M_overwritemethod_updateSourceTermLinearPDE(F,buildCstPart);
        return;
    }

    if ( this->M_volumicForcesProperties.empty() ) return;

    if ( !buildCstPart )
    {
        auto myLinearForm = form1( _test=this->spaceTemperature(), _vector=F,
                                   _rowstart=this->rowStartInVector() );
        auto const& v = this->fieldTemperature();

        for( auto const& d : this->M_volumicForcesProperties )
        {
            auto rangeBodyForceUsed = ( marker(d).empty() )? this->rangeMeshElements() : markedelements(this->mesh(),marker(d));
            myLinearForm +=
                integrate( _range=rangeBodyForceUsed,
                           _expr= expression(d)*id(v),
                           _geomap=this->geomap() );
        }
    }
}

HEATTRANSFER_CLASS_TEMPLATE_DECLARATIONS
void
HEATTRANSFER_CLASS_TEMPLATE_TYPE::updateWeakBCLinearPDE(sparse_matrix_ptrtype& A, vector_ptrtype& F,bool buildCstPart) const
{
    if ( this->M_bcNeumann.empty() && this->M_bcRobin.empty() ) return;

    if ( !buildCstPart )
    {
        auto mesh = this->mesh();
        auto Xh = this->spaceTemperature();
        auto const& v = this->fieldTemperature();

        auto myLinearForm = form1( _test=Xh, _vector=F,
                                   _rowstart=this->rowStartInVector() );
        for( auto const& d : this->M_bcNeumann )
        {
            myLinearForm +=
                integrate( _range=markedfaces(this->mesh(),this->markerNeumannBC(NeumannBCShape::SCALAR,marker(d)) ),
                           _expr= expression(d)*id(v),
                           _geomap=this->geomap() );
        }

        auto bilinearForm_PatternCoupled = form2( _test=Xh,_trial=Xh,_matrix=A,
                                                  _pattern=size_type(Pattern::COUPLED),
                                                  _rowstart=this->rowStartInMatrix(),
                                                  _colstart=this->colStartInMatrix() );
        for( auto const& d : this->M_bcRobin )
        {
            bilinearForm_PatternCoupled +=
                integrate( _range=markedfaces(mesh,this->markerRobinBC( marker(d) ) ),
                           _expr= expression1(d)*idt(v)*id(v),
                           _geomap=this->geomap() );
            myLinearForm +=
                integrate( _range=markedfaces(mesh,this->markerRobinBC( marker(d) ) ),
                           _expr= expression1(d)*expression2(d)*id(v),
                           _geomap=this->geomap() );
        }

    }
}




} // end namespace FeelModels
} // end namespace Feel