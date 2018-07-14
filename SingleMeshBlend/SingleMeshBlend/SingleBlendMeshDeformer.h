// Copyright 2018 Luca Di Sera
//		Contact: disera.luca@gmail.com
//				 https://github.com/diseraluca
//				 www.linkedin.com/in/luca-di-sera-200023167
//
// This code is licensed under the MIT License. 
// More informations can be found in the LICENSE file in the root folder of this repository
//
//
// File : SingleBlendMeshDeformer.h
//
// The SingleBlendMeshDeformer class defines a custom maya deformer node that morph a mesh to another.
// For the morph to happen the deformer expects the meshes to have the same number of vertices and 
// the same vertext ID per vertex.

#pragma once

#include <maya/MPxDeformerNode.h>
#include <maya/MPointArray.h>
#include <maya/MFnMesh.h>
#include <maya/MThreadPool.h>

//An helper struct that store the data needed by the threads.
//The data is shared to all threads
struct TaskData {
	MPointArray resultPositions;
	MPointArray vertexPositions;
	MVectorArray deltas;

	float envelopeValue;
	double blendWeightValue;
};

//An helper struct that store the information needed
//for a specific thread computation
struct ThreadData {
	unsigned int start;
	unsigned int end;
	unsigned int numTasks;
	TaskData* data;
};

class SingleBlendMeshDeformer : public MPxDeformerNode {
public:
	SingleBlendMeshDeformer();
	~SingleBlendMeshDeformer();

	static  void*   creator();
	static  MStatus initialize();
	//virtual MStatus preEvaluation(const  MDGContext& context, const MEvaluationNode& evaluationNode) override;
	virtual MStatus deform(MDataBlock & block, MItGeometry & iterator, const MMatrix & matrix, unsigned int multiIndex) override;

	ThreadData*         createThreadData(int numTasks, TaskData* taskData);
	static void         createTasks(void* data, MThreadRootTask *pRoot);
	static MThreadRetVal threadEvaluate(void* pParam);

private:
	/// Caches the blendMesh positions into this->blendVertexPositions
	MStatus cacheBlendMeshVertexPositionsAndDeltas(const MFnMesh& blendMeshFn, const MPointArray& vertexPositions);

	/// Caches the deltas between the mesh and the blend mesh
	/// Warning: By caching the deltas the base mesh is expected to be non animated
	MStatus cacheDeltasValues(const MPointArray& vertexPositions, const MPointArray& blendVertexPositions, int vertexCount);

public:
	static MString typeName;
	static MTypeId typeId;
	
	static MObject blendMesh;
	static MObject blendWeight;
	static MObject rebind;

	/// The number of thread tasks
	static MObject numTasks;

private:
	bool isInitialized;
	bool isThreadDataInitialized;

	//The last number of tasks. If it is different to the current numTask we recreate the threadData
	int lastTaskValue;

	TaskData taskData;
	ThreadData* threadData;
};