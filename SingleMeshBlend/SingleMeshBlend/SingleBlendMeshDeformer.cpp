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
#include <maya/MItGeometry.h>
#include <maya/MEvaluationNode.h>
#include <maya/MThreadPool.h>

MString SingleBlendMeshDeformer::typeName{ "SingleBlendMesh" };
MTypeId SingleBlendMeshDeformer::typeId{ 0x0d12309 };

MObject SingleBlendMeshDeformer::blendMesh;
MObject SingleBlendMeshDeformer::blendWeight;
MObject SingleBlendMeshDeformer::numTasks;

SingleBlendMeshDeformer::SingleBlendMeshDeformer()
	:isInitialized{ false },
	 blendVertexPositions{}
{
	MThreadPool::init();
}

SingleBlendMeshDeformer::~SingleBlendMeshDeformer()
{
	MThreadPool::release();
}

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

	numTasks = nAttr.create("numTasks", "ntk", MFnNumericData::kInt, 32, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	CHECK_MSTATUS(nAttr.setChannelBox(true));
	CHECK_MSTATUS(nAttr.setMin(1));
	CHECK_MSTATUS(addAttribute(numTasks));

	attributeAffects(blendMesh, outputGeom);
	attributeAffects(blendWeight, outputGeom);

	MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer SingleBlendMesh weights");

	return MStatus::kSuccess;
}

MStatus SingleBlendMeshDeformer::preEvaluation(const  MDGContext& context, const MEvaluationNode& evaluationNode) {
	MStatus status{};

	if (!context.isNormal()) {
		return MStatus::kFailure;
	}

	// If the blendMesh has been changed we set isInitiliazed to false to cache it again
	if (evaluationNode.dirtyPlugExists(blendMesh, &status) && status) {
		isInitialized = false;
	}

	return MStatus::kSuccess;
}

MStatus SingleBlendMeshDeformer::deform(MDataBlock & block, MItGeometry & iterator, const MMatrix & matrix, unsigned int multiIndex)
{
	MPlug blendMeshPlug{ thisMObject(), blendMesh };
	if (!blendMeshPlug.isConnected()) {
		MGlobal::displayWarning(this->name() + ": blendMesh not connected. Please connect a mesh.");
		return MStatus::kInvalidParameter;
	}

	float envelopeValue{ block.inputValue(envelope).asFloat() };
	MObject blendMeshValue{ block.inputValue(blendMesh).asMesh() };
	double blendWeightValue{ block.inputValue(blendWeight).asDouble() };

	MFnMesh blendMeshFn{ blendMeshValue };
	if (!isInitialized) {
		CHECK_MSTATUS_AND_RETURN_IT( cacheBlendMeshVertexPositions(blendMeshFn) );
		isInitialized = true;
	}

	MPointArray vertexPositions{};
	CHECK_MSTATUS_AND_RETURN_IT( iterator.allPositions(vertexPositions) );
	
	//MPointArray operator[] has a non-negligible overhead. 
	//Accessing the memory directly with pointers bypass that overhead.
	unsigned int vertexCount{ vertexPositions.length() };
	MPoint* currentVertexPosition{ &vertexPositions[0] };
	MPoint* blendVertexPosition{ &blendVertexPositions[0] };
	for (unsigned int vertexIndex{ 0 }; vertexIndex < vertexCount; vertexIndex++) {
		float weight{ weightValue(block, multiIndex, vertexIndex) };
		MVector delta{ (*(blendVertexPosition + vertexIndex) - *(currentVertexPosition + vertexIndex)) * blendWeightValue * envelopeValue *  weight };
		MPoint newPosition{ delta + *(currentVertexPosition + vertexIndex) };

		*(currentVertexPosition + vertexIndex) = newPosition;
	}
	
	iterator.setAllPositions(vertexPositions);
	return MStatus::kSuccess;
}

ThreadData * SingleBlendMeshDeformer::createThreadData(int numTasks, TaskData * taskData)
{
	ThreadData* threadData = new ThreadData[numTasks];
	unsigned int vertexCount{ taskData->vertexPositions.length() };
	unsigned int taskLenght{ (vertexCount + numTasks - 1) / numTasks };

	unsigned int start{ 0 };
	unsigned int end{ taskLenght };

	int lastTask{ numTasks - 1 };
	for (int taskIndex{ 0 }; taskIndex < numTasks; taskIndex++) {
		if (taskIndex == lastTask) {
			end = vertexCount;
		}

		threadData[taskIndex].start = start;
		threadData[taskIndex].end = end;
		threadData[taskIndex].numTasks = numTasks;
		threadData[taskIndex].data = taskData;

		start += taskLenght;
		end += taskLenght;
	}

	return threadData;
}

void SingleBlendMeshDeformer::createTasks(void * data, MThreadRootTask * pRoot)
{
	ThreadData* threadData{ static_cast<ThreadData*>(data) };

	if (threadData) {
		int numTasks{ threadData->numTasks };
		for (int taskIndex{ 0 }; taskIndex < numTasks; taskIndex++) {
			MThreadPool::createTask(threadEvaluate, (void*)&threadData[taskIndex], pRoot);
		}
		MThreadPool::executeAndJoin(pRoot);
	}
}

MThreadRetVal SingleBlendMeshDeformer::threadEvaluate(void * pParam)
{
	MStatus status{};

	ThreadData* threadData{ (ThreadData*)pParam };
	TaskData* data{ threadData->data };

	unsigned int start{ threadData->start };
	unsigned int end{ threadData->end };

	MPointArray& vertexPositions{ data->vertexPositions };
	MPointArray& blendVertexPositions{ data->blendVertexPositions };

	float envelopeValue{ data->envelopeValue };
	double blendWeightValue{ data->blendWeightValue };

	unsigned int vertexCount{ vertexPositions.length() };
	MPoint* currentVertexPosition{ &vertexPositions[0] };
	MPoint* blendVertexPosition{ &blendVertexPositions[0] };
	for (unsigned int vertexIndex{ start }; vertexIndex < end; vertexIndex++) {
		MVector delta{ (*(blendVertexPosition + vertexIndex) - *(currentVertexPosition + vertexIndex)) * blendWeightValue * envelopeValue };
		MPoint newPosition{ delta + *(currentVertexPosition + vertexIndex) };

		*(currentVertexPosition + vertexIndex) = newPosition;
	}

	return MThreadRetVal();
}

MStatus SingleBlendMeshDeformer::cacheBlendMeshVertexPositions(const MFnMesh & blendMeshFn)
{
	MStatus status{};

	int vertexCount{ blendMeshFn.numVertices(&status) };
	CHECK_MSTATUS_AND_RETURN_IT(status);

	blendVertexPositions.setLength(vertexCount);
	CHECK_MSTATUS_AND_RETURN_IT( blendMeshFn.getPoints(blendVertexPositions) );

	return MStatus::kSuccess;
}
