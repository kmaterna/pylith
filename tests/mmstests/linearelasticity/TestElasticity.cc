// -*- C++ -*-
//
// ----------------------------------------------------------------------
//
// Brad T. Aagaard, U.S. Geological Survey
// Charles A. Williams, GNS Science
// Matthew G. Knepley, University at Buffalo
//
// This code was developed as part of the Computational Infrastructure
// for Geodynamics (http://geodynamics.org).
//
// Copyright (c) 2010-2022 University of California, Davis
//
// See LICENSE.md for license information.
//
// ----------------------------------------------------------------------
//

#include <portinfo>

#include "TestElasticity.hh" // Implementation of class methods

#include "pylith/problems/TimeDependent.hh" // USES TimeDependent
#include "pylith/materials/Elasticity.hh" // USES Elasticity

#include "pylith/materials/Query.hh" // USES Query

#include "pylith/topology/Mesh.hh" // USES Mesh
#include "pylith/topology/MeshOps.hh" // USES MeshOps::nondimensionalize()
#include "pylith/topology/Field.hh" // USES Field
#include "pylith/topology/VisitorMesh.hh" // USES VecVisitorMesh
#include "pylith/topology/FieldQuery.hh" // USES FieldQuery
#include "pylith/feassemble/AuxiliaryFactory.hh" // USES AuxiliaryFactory
#include "pylith/problems/SolutionFactory.hh" // USES SolutionFactory
#include "pylith/meshio/MeshIOAscii.hh" // USES MeshIOAscii
#include "pylith/meshio/MeshIOPetsc.hh" // USES MeshIOPetsc
#include "pylith/bc/DirichletUserFn.hh" // USES DirichletUserFn
#include "pylith/utils/error.hh" // USES PYLITH_METHOD_BEGIN/END
#include "pylith/utils/journals.hh" // pythia::journal

#include "spatialdata/spatialdb/UserFunctionDB.hh" // USES UserFunctionDB
#include "spatialdata/geocoords/CoordSys.hh" // USES CoordSys
#include "spatialdata/spatialdb/GravityField.hh" // USES GravityField
#include "spatialdata/units/Nondimensional.hh" // USES Nondimensional

// ---------------------------------------------------------------------------------------------------------------------
// Setup testing data.
void
pylith::mmstests::TestElasticity::setUp(void) {
    MMSTest::setUp();

    _material = new pylith::materials::Elasticity;CPPUNIT_ASSERT(_material);
    _bc = new pylith::bc::DirichletUserFn;CPPUNIT_ASSERT(_bc);
    _data = NULL;
} // setUp


// ---------------------------------------------------------------------------------------------------------------------
// Deallocate testing data.
void
pylith::mmstests::TestElasticity::tearDown(void) {
    delete _material;_material = NULL;
    delete _bc;_bc = NULL;
    delete _data;_data = NULL;

    MMSTest::tearDown();
} // tearDown


static bool
endsWith(const std::string& str,
         const std::string& suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}


// ---------------------------------------------------------------------------------------------------------------------
// Initialize objects for test.
void
pylith::mmstests::TestElasticity::_initialize(void) {
    PYLITH_METHOD_BEGIN;
    CPPUNIT_ASSERT(_mesh);

    PetscErrorCode err = 0;

    if (_data->meshFilename) {
        std::string name(_data->meshFilename);
        if (endsWith(name, ".mesh")) {
            pylith::meshio::MeshIOAscii iohandler;
            iohandler.setFilename(_data->meshFilename);
            iohandler.read(_mesh);CPPUNIT_ASSERT(_mesh);
        } else {
            if (_data->meshOptions) {
                err = PetscOptionsInsertString(NULL, _data->meshOptions);PYLITH_CHECK_ERROR(err);
            } // if
            pylith::meshio::MeshIOPetsc iohandler;
            iohandler.setFilename(_data->meshFilename);
            iohandler.read(_mesh);CPPUNIT_ASSERT(_mesh);
        } // if/else
    } // if/else

    CPPUNIT_ASSERT_MESSAGE("Test mesh does not contain any cells.",
                           pylith::topology::MeshOps::getNumCells(*_mesh) > 0);
    CPPUNIT_ASSERT_MESSAGE("Test mesh does not contain any vertices.",
                           pylith::topology::MeshOps::getNumVertices(*_mesh) > 0);

    // Set up coordinates.
    _mesh->setCoordSys(_data->cs);
    CPPUNIT_ASSERT(_data->normalizer);
    pylith::topology::MeshOps::nondimensionalize(_mesh, *_data->normalizer);

    // Set up material
    CPPUNIT_ASSERT(_material);
    _material->setAuxiliaryFieldDB(_data->auxDB);

    for (int i = 0; i < _data->numAuxSubfields; ++i) {
        const pylith::topology::FieldBase::Discretization& info = _data->auxDiscretizations[i];
        _material->setAuxiliarySubfieldDiscretization(_data->auxSubfields[i], info.basisOrder, info.quadOrder,
                                                      _data->spaceDim, pylith::topology::FieldBase::DEFAULT_BASIS,
                                                      info.feSpace, info.isBasisContinuous);
    } // for

    // Set up problem.
    CPPUNIT_ASSERT(_problem);
    CPPUNIT_ASSERT(_data->normalizer);
    _problem->setNormalizer(*_data->normalizer);
    _problem->setGravityField(_data->gravityField);
    pylith::materials::Material* materials[1] = { _material };
    _problem->setMaterials(materials, 1);
    pylith::bc::BoundaryCondition* bcs[1] = { _bc };
    _problem->setBoundaryConditions(bcs, 1);
    _problem->setStartTime(_data->t);
    _problem->setEndTime(_data->t+_data->dt);
    _problem->setInitialTimeStep(_data->dt);
    _problem->setFormulation(_data->formulation);

    // Set up solution field.
    CPPUNIT_ASSERT(!_solution);
    _solution = new pylith::topology::Field(*_mesh);CPPUNIT_ASSERT(_solution);
    _solution->setLabel("solution");
    pylith::problems::SolutionFactory factory(*_solution, *_data->normalizer);
    factory.addDisplacement(_data->solnDiscretizations[0]);
    if (pylith::problems::Physics::QUASISTATIC == _data->formulation) {
        CPPUNIT_ASSERT_EQUAL(1, _data->numSolnSubfields);
    } else {
        CPPUNIT_ASSERT_EQUAL(pylith::problems::Physics::DYNAMIC, _data->formulation);
        CPPUNIT_ASSERT_EQUAL(2, _data->numSolnSubfields);
        factory.addVelocity(_data->solnDiscretizations[1]);
    } // if/else
    _problem->setSolution(_solution);

    pylith::testing::MMSTest::_initialize();

    PYLITH_METHOD_END;
} // _initialize


// ---------------------------------------------------------------------------------------------------------------------
// Constructor
pylith::mmstests::TestElasticity_Data::TestElasticity_Data(void) :
    spaceDim(0),
    meshFilename(NULL),
    meshOptions(NULL),
    boundaryLabel(NULL),
    cs(NULL),
    gravityField(NULL),
    normalizer(new spatialdata::units::Nondimensional),

    t(0.0),
    dt(0.05),

    numSolnSubfields(0),
    solnDiscretizations(NULL),

    numAuxSubfields(0),
    auxSubfields(NULL),
    auxDiscretizations(NULL),
    auxDB(new spatialdata::spatialdb::UserFunctionDB),

    formulation(pylith::problems::Physics::QUASISTATIC) {
    CPPUNIT_ASSERT(normalizer);

    CPPUNIT_ASSERT(auxDB);
    auxDB->setDescription("auxiliary field spatial database");
} // constructor


// ---------------------------------------------------------------------------------------------------------------------
// Destructor
pylith::mmstests::TestElasticity_Data::~TestElasticity_Data(void) {
    delete cs;cs = NULL;
    delete gravityField;gravityField = NULL;
    delete normalizer;normalizer = NULL;
    delete auxDB;auxDB = NULL;
} // destructor


// End of file
