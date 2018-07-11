// Copyright 2018 Luca Di Sera
//		Contact: disera.luca@gmail.com
//				 https://github.com/diseraluca
//				 https://www.linkedin.com/in/luca-di-sera-200023167
//
// This code is licensed under the MIT License. 
// More informations can be found in the LICENSE file in the root folder of this repository
//
//
// File : SingleBlendMeshDeformer.cpp

#include "SingleBlendMeshDeformer.h"

MString SingleBlendMeshDeformer::typeName{ "SingleBlendMesh" };
MTypeId SingleBlendMeshDeformer::typeId{ 0x0d12309 };

MObject SingleBlendMeshDeformer::blendMesh;
MObject SingleBlendMeshDeformer::blendWeight;

void * SingleBlendMeshDeformer::creator()
{
	return new SingleBlendMeshDeformer();
}

MStatus SingleBlendMeshDeformer::initialize()
{
	return MStatus();
}

MStatus SingleBlendMeshDeformer::deform(MDataBlock & block, MItGeometry & iterator, const MMatrix & matrix, unsigned int multiIndex)
{
	return MStatus();
}
