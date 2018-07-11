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

#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MGlobal.h>

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
	MStatus status{};

	MFnTypedAttribute   tAttr{};
	MFnNumericAttribute nAttr{};

	blendMesh = tAttr.create("blendMesh", "blm", MFnData::kMesh, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	CHECK_MSTATUS(addAttribute(blendMesh));

	blendWeight = nAttr.create("blendWeight", "blw", MFnNumericData::kDouble, 0.0, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	CHECK_MSTATUS(nAttr.setKeyable(true));
	CHECK_MSTATUS(nAttr.setMin(0.0));
	CHECK_MSTATUS(nAttr.setMax(1.0));
	CHECK_MSTATUS(addAttribute(blendWeight));

	attributeAffects(blendMesh, outputGeom);
	attributeAffects(blendWeight, outputGeom);

	MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer SingleBlendMesh weights");

	return MStatus::kSuccess;
}

MStatus SingleBlendMeshDeformer::deform(MDataBlock & block, MItGeometry & iterator, const MMatrix & matrix, unsigned int multiIndex)
{
	return MStatus();
}