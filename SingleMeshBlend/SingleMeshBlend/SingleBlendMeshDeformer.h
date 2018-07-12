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

//An helper struct that store the data needed by the threads.
//The data is shared to all threads
struct TaskData {
	MPointArray vertexPositions;
	MPointArray blendVertexPositions;

	float envelopeValue;
	double blendWeightValue;
	float weight;
};

//An helper struct that store the information needed
//for a specific thread computation
struct ThreadData {
	unsigned int start;
	unsigned int end;
	unsigned int numTasks;
	TaskData* pData;
};

class SingleBlendMeshDeformer : public MPxDeformerNode {
public:
	SingleBlendMeshDeformer();
	~SingleBlendMeshDeformer();

	static  void*   creator();
	static  MStatus initialize();
	virtual MStatus preEvaluation(const  MDGContext& context, const MEvaluationNode& evaluationNode) override;
	virtual MStatus deform(MDataBlock & block, MItGeometry & iterator, const MMatrix & matrix, unsigned int multiIndex) override;

	ThreadData*         createThreadData(int numTasks, TaskData* pTaskData);
	static void         createTasks(void* data, MThreadRootTask *pRoot);
	static MThreadRetVal threadEvaluate(void* pParam);

private:
	/// Caches the blendMesh positions into this->blendVertexPositions
	MStatus cacheBlendMeshVertexPositions(const MFnMesh& blendMeshFn);

public:
	static MString typeName;
	static MTypeId typeId;
	
	static MObject blendMesh;
	static MObject blendWeight;

	/// The number of thread tasks
	static MObject numTasks;

private:
	bool isInitialized;
	MPointArray blendVertexPositions;
};