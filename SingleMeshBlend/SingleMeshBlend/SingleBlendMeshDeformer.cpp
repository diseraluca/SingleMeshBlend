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

MString SingleBlendMeshDeformer::typeName{ "SingleBlendMesh" };
MTypeId SingleBlendMeshDeformer::typeId{ 0x0d12309 };

MObject SingleBlendMeshDeformer::blendMesh;
MObject SingleBlendMeshDeformer::blendWeight;
MObject SingleBlendMeshDeformer::rebind;
MObject SingleBlendMeshDeformer::numTasks;

SingleBlendMeshDeformer::SingleBlendMeshDeformer()
	:isInitialized{ false },
	 isThreadDataInitialized{ false },
	 lastTaskValue{ 0 },
     taskData{},
	 threadData{ nullptr }
{
	MThreadPool::init();
}

SingleBlendMeshDeformer::~SingleBlendMeshDeformer()
{
	delete[] threadData;
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

	rebind = nAttr.create("rebind", "rbd", MFnNumericData::kBoolean, false, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	CHECK_MSTATUS(nAttr.setKeyable(true));
	CHECK_MSTATUS(addAttribute(rebind));

	numTasks = nAttr.create("numTasks", "ntk", MFnNumericData::kInt, 32, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	CHECK_MSTATUS(nAttr.setChannelBox(true));
	CHECK_MSTATUS(nAttr.setMin(1));
	CHECK_MSTATUS(addAttribute(numTasks));

	CHECK_MSTATUS(attributeAffects(blendMesh, outputGeom));
	CHECK_MSTATUS(attributeAffects(blendWeight, outputGeom));
	CHECK_MSTATUS(attributeAffects(rebind, outputGeom));

	MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer SingleBlendMesh weights");

	return MStatus::kSuccess;
}

/*
MStatus SingleBlendMeshDeformer::preEvaluation(const  MDGContext& context, const MEvaluationNode& evaluationNode) {
	MStatus status{};

	if (!context.isNormal()) {
		return MStatus::kFailure;
	}

	// If the blendMesh has been changed we set isInitiliazed to false to cache it again
	if (evaluationNode.dirtyPlugExists(blendMesh, &status) && status) {
		isInitialized = false;
	}

	// If numTask was changed we have to create the ThreadData again
	if (evaluationNode.dirtyPlugExists(blendMesh, &status) && status) {
		isThreadDataInitialized = false;
	}

	return MStatus::kSuccess;
}
*/

MStatus SingleBlendMeshDeformer::deform(MDataBlock & block, MItGeometry & iterator, const MMatrix & matrix, unsigned int multiIndex)
{
	CHECK_MSTATUS_AND_RETURN_IT(iterator.allPositions(taskData.vertexPositions));

	bool rebindValue{ block.inputValue(rebind).asBool() };

	if (!isInitialized || rebindValue) {
		// If blendMesh is not connected we get out
		MPlug blendMeshPlug{ thisMObject(), blendMesh };
		if (!blendMeshPlug.isConnected()) {
			MGlobal::displayWarning(this->name() + ": blendMesh not connected. Please connect a mesh.");
			return MStatus::kInvalidParameter;
		}

		MObject blendMeshValue{ block.inputValue(blendMesh).asMesh() };
		MFnMesh blendMeshFn{ blendMeshValue };

		CHECK_MSTATUS_AND_RETURN_IT( cacheBlendMeshVertexPositionsAndDeltas(blendMeshFn, taskData.vertexPositions) );
		isInitialized = true;
		isThreadDataInitialized = false;
	}

	float envelopeValue{ block.inputValue(envelope).asFloat() };
	double blendWeightValue{ block.inputValue(blendWeight).asDouble() };

	// Setting the relevant attribute values on taskData so that the threads can access them
	taskData.envelopeValue = envelopeValue;
	taskData.blendWeightValue = blendWeightValue;

	// Initialize thead data
	int numTasksValue{ block.inputValue(numTasks).asInt() };
	if (!isThreadDataInitialized || ( lastTaskValue != numTasksValue )) {
		//If it isn't the first time we create the data we delete the previous data
		if (threadData) {
			delete[] threadData;
		}

		threadData = createThreadData(numTasksValue, &taskData);
		isThreadDataInitialized = true;
		lastTaskValue = numTasksValue;
	}

	MThreadPool::newParallelRegion(createTasks, (void*)threadData);

	iterator.setAllPositions(taskData.vertexPositions);

	return MStatus::kSuccess;
}

ThreadData * SingleBlendMeshDeformer::createThreadData(int numTasks, TaskData * taskData)
{
	ThreadData* threadData = new ThreadData[numTasks];
	unsigned int vertexCount{ taskData->vertexPositions.length() };
	unsigned int taskLenght{ (vertexCount + numTasks - 1) / numTasks };

	unsigned int start{ 0 };
	unsigned int end{ taskLenght };

	for (int taskIndex{ 0 }; taskIndex < numTasks; ++taskIndex) {

		threadData[taskIndex].start = start;
		threadData[taskIndex].end = end;
		threadData[taskIndex].numTasks = numTasks;
		threadData[taskIndex].data = taskData;

		start += taskLenght;
		end += taskLenght;
	}

	// We patch the last task to end at the last vertex
	threadData[numTasks - 1].end = vertexCount;

	return threadData;
}

void SingleBlendMeshDeformer::createTasks(void * data, MThreadRootTask * pRoot)
{
	ThreadData* threadData{ static_cast<ThreadData*>(data) };

	if (threadData) {
		unsigned int numTasks{ threadData->numTasks };
		for (unsigned int taskIndex{ 0 }; taskIndex < numTasks; taskIndex++) {
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

	//Operator [] on maya containers as  a non negligible overhead. By using pointer we can bypass it
	MPoint* currentVertexPosition{ &data->vertexPositions[0] };
	MVectorArray deltas{ data->deltas };

	float envelopeValue{ data->envelopeValue };
	double blendWeightValue{ data->blendWeightValue };

	unsigned int vertexCount{ data->vertexPositions.length() };
	for (unsigned int vertexIndex{ start }; vertexIndex < end; ++vertexIndex) {
		currentVertexPosition[vertexIndex] = (deltas[vertexIndex] * blendWeightValue * envelopeValue) + currentVertexPosition[vertexIndex];
	}

	return MThreadRetVal();
}

MStatus SingleBlendMeshDeformer::cacheBlendMeshVertexPositionsAndDeltas(const MFnMesh & blendMeshFn, const MPointArray& vertexPositions)
{
	MStatus status{};

	int vertexCount{ blendMeshFn.numVertices(&status) };
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Cache blend vertex Positions
	MPointArray blendVertexPositions{};
	blendVertexPositions.setLength(vertexCount);
	CHECK_MSTATUS_AND_RETURN_IT( blendMeshFn.getPoints(blendVertexPositions) );

	cacheDeltasValues(vertexPositions, blendVertexPositions, vertexCount);

	return MStatus::kSuccess;
}

MStatus SingleBlendMeshDeformer::cacheDeltasValues(const MPointArray & vertexPositions, const MPointArray & blendVertexPositions, int vertexCount)
{
	taskData.deltas.setLength(vertexCount);
	for (int vertexIndex{ 0 }; vertexIndex < vertexCount; ++vertexIndex) {
		taskData.deltas[vertexIndex] = blendVertexPositions[vertexIndex] - vertexPositions[vertexIndex];
	}

	return MStatus::kSuccess;
}
