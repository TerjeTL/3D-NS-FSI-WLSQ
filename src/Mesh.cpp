/*
 * Mesh.cpp
 *
 *  Created on: Oct 7, 2022
 *      Author: frederk
 */

#include "Mesh.h"

Mesh::Mesh(const ConfigSettings& params) :
NI{params.NI}, NJ{params.NJ}, NK{params.NK},
nNodesTotal{NI*NJ*NK},
dx{ -1 }, dy{ -1 }, dz{ -1 }, // <- Depend on boundary condition types
conservedVariables(NI, NJ, NK),
conservedVariablesOld(NI, NJ, NK),
primitiveVariables(NI, NJ, NK),
transportProperties(NI, NJ, NK),
RK4slopes(NI, NJ, NK),
nodeType(NI, NJ, NK)
{
}



// Construct the objects that define the boundary conditions (BCs)
void Mesh::setupBoundaries(const ConfigSettings &params)
{
	edgeBoundaries.push_back(std::make_unique<SymmetryBoundary>(AxisOrientationEnum::z, EdgeIndexEnum::min));
	edgeBoundaries.push_back(std::make_unique<SymmetryBoundary>(AxisOrientationEnum::z, EdgeIndexEnum::max));
	edgeBoundaries.push_back(std::make_unique<SymmetryBoundary>(AxisOrientationEnum::y, EdgeIndexEnum::min));
	edgeBoundaries.push_back(std::make_unique<SymmetryBoundary>(AxisOrientationEnum::y, EdgeIndexEnum::max));
	edgeBoundaries.push_back(std::make_unique<InletBoundary>(AxisOrientationEnum::x, EdgeIndexEnum::min, params.M_0));
	edgeBoundaries.push_back(std::make_unique<OutletBoundary>(AxisOrientationEnum::x, EdgeIndexEnum::max));

	dx = params.L_x / (NI-1);
	dy = params.L_y / (NJ-3);
	dz = params.L_z / (NK-3);
	positionOffset.x = 0;
	positionOffset.y = 1;
	positionOffset.z = 1;

//	Vector3_d cylinderCentroidPosition(params.L_x / 4, params.L_y / 2, 0);
//	immersedBoundaries.push_back(std::make_unique<CylinderBody>(cylinderCentroidPosition,
//																AxisOrientationEnum::z,
//																params.L_y/8));
	Vector3_d sphereCenterPoint(params.L_x/4, params.L_y/2, params.L_z/2);
	immersedBoundaries.push_back(std::make_unique<SphereBody>(sphereCenterPoint, params.L_y/10));
}

// Sanity check for the combination of boundary conditions.
void Mesh::assertBoundaryConditionCompliance()
{
	// Implement when creating general IBM.
}

void Mesh::categorizeNodes(const ConfigSettings& params)
{
	const IndexBoundingBox meshSize(NI-1, NJ-1, NK-1);
	const Vector3_u nMeshNodes(NI, NJ, NK);
	const Vector3_d gridSpacing(dx, dy, dz);
	nodeType.setAll(NodeTypeEnum::FluidInterior);

	IndexBoundingBox unclaimedNodes = meshSize;
	for(auto&& boundary : edgeBoundaries)
		boundary->identifyOwnedNodes(unclaimedNodes, nMeshNodes, nodeType);
	std::reverse( edgeBoundaries.begin(), edgeBoundaries.end() );

	for(auto&& boundary : immersedBoundaries)
		boundary->identifyRelatedNodes(params, gridSpacing, nMeshNodes, positionOffset, nodeType);

	for(size_t index1D{0}; index1D<nNodesTotal; ++index1D)
		if(nodeType(index1D) == NodeTypeEnum::FluidInterior)
			indexByType.fluidInterior.push_back(index1D);
		else if(nodeType(index1D) == NodeTypeEnum::FluidEdge)
			indexByType.fluidEdge.push_back(index1D);
		else if(nodeType(index1D) == NodeTypeEnum::SolidGhost)
//			 || nodeType(index1D) == NodeTypeEnum::FluidGhost)
			indexByType.ghost.push_back(index1D);
}

// Filters one variable field, i.e., one solution array, 'filterVariable' and stores the filtered
// result in 'variableTemporaryStorage'. Then, the arrays are swapped by move-semantics.
// Only filters if the modulo of time level plus one, by the filter interval is zero.
// This causes the first filtering to happen as late as possible.
// Currently, copies the edge-boundary nodes and filters all interior nodes, including ghost and
// solid. Consider copying also ghosts, and filter only active fluid nodes.
void Mesh::applyFilter_ifAppropriate(Array3D_d& filterVariable, Array3D_d& variableTemporaryStorage,
									uint filterInterval, ulong timeLevel)
{
	if(filterInterval > 0)
		if( (timeLevel+1) % filterInterval == 0 )
		{
			size_t iMax{NI-1}, jMax{NJ-1}, kMax{NK-1};
			// Copy boundary nodes:
			for(size_t i{0}; i<=iMax; ++i)
				for(size_t j{0}; j<=jMax; ++j)
				{
					variableTemporaryStorage(i,j,0   ) = filterVariable(i,j,0   );
					variableTemporaryStorage(i,j,kMax) = filterVariable(i,j,kMax);
				}
			for(size_t i{0}; i<=iMax; ++i)
				for(size_t k{1}; k<=kMax-1; ++k)
				{
					variableTemporaryStorage(i,0,   k) = filterVariable(i,0,   k);
					variableTemporaryStorage(i,jMax,k) = filterVariable(i,jMax,k);
				}
			for(size_t j{1}; j<=jMax-1; ++j)
				for(size_t k{1}; k<=kMax-1; ++k)
				{
					variableTemporaryStorage(0,   j,k) = filterVariable(0,   j,k);
					variableTemporaryStorage(iMax,j,k) = filterVariable(iMax,j,k);
				}
			// Apply filter to interior nodes:
			for(size_t i{1}; i<=iMax-1; ++i)
				for(size_t j{1}; j<=jMax-1; ++j)
					for(size_t k{1}; k<=kMax-1; ++k)
					{
						variableTemporaryStorage(i,j,k) = 1./2.  *   filterVariable(i,j,k)
														+ 1./12. * ( filterVariable(i+1,j,k) + filterVariable(i-1,j,k)
																   + filterVariable(i,j+1,k) + filterVariable(i,j-1,k)
																   + filterVariable(i,j,k+1) + filterVariable(i,j,k-1) );
					}
			filterVariable.dataSwap(variableTemporaryStorage);	// Swap the arrays using move-sematics (super-fast)
		}
}

// Swap the contents of all the arrays of conserved variables and the intermediate arrays, by move-semantics.
// This operation is super fast and needs no extra copy. Only the ownership of the data is changed.
void Mesh::swapConservedVariables()
{
	conservedVariables.rho  .dataSwap(conservedVariablesOld.rho  );
	conservedVariables.rho_u.dataSwap(conservedVariablesOld.rho_u);
	conservedVariables.rho_v.dataSwap(conservedVariablesOld.rho_v);
	conservedVariables.rho_w.dataSwap(conservedVariablesOld.rho_w);
	conservedVariables.rho_E.dataSwap(conservedVariablesOld.rho_E);
}

void Mesh::applyAllBoundaryConditions(double t, const ConfigSettings& params)
{
	const Vector3_u nMeshNodes(NI, NJ, NK);
	const Vector3_d gridSpacing(dx, dy, dz);
	AllFlowVariablesArrayGroup flowVariableReferences(conservedVariables, primitiveVariables, transportProperties);

	for(auto&& boundary : edgeBoundaries)
		boundary->applyBoundaryCondition(t, nMeshNodes, params, flowVariableReferences);

	for(auto&& boundary : immersedBoundaries)
		boundary->applyBoundaryCondition(nMeshNodes, gridSpacing, positionOffset, params, nodeType, flowVariableReferences);
}

// Compute the 2-norm of the difference between two arrys. Intended to monitor the change between two consecutive time levels.
// E.g. to check convergence of solution.
double Mesh::getNormOfChange(const Array3D_d& oldValue, const Array3D_d& newValue)
{
	double sumOfSquaredChanges = 0;
	for(size_t i : indexByType.fluidInterior)
		sumOfSquaredChanges += pow(oldValue(i)-newValue(i), 2);
	double normOfChange = sqrt( dx*dy*dz * sumOfSquaredChanges );
	return normOfChange;
}





